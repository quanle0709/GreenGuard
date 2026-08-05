#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <ThingSpeak.h>
#include "config.h"
#include "secrets.h"
#include "GreenGuardController.h"

PersistentStorage storage; RainSensor rainSensor; MotorController motorController;
GreenGuardController controller(storage,rainSensor,motorController); ESP8266WebServer server(80); WiFiClient cloudClient;
uint32_t lastWifiAttempt=0,lastUpload=0,lastUploadSuccess=0; int thingSpeakStatus=0; bool mdnsStarted=false,fsReady=false;

void sendJson(JsonDocument& d,int code=200){String out;serializeJson(d,out);server.send(code,"application/json; charset=utf-8",out);}
bool boolArg(const String& s){return s=="true"||s=="1"||s=="on";}
bool parseBody(JsonDocument& d){if(server.hasArg("plain")&&server.arg("plain").length()){return !deserializeJson(d,server.arg("plain"));}for(uint8_t i=0;i<server.args();i++)d[server.argName(i)]=server.arg(i);return true;}
void serveFile(const char* path,const char* type){File f=LittleFS.open(path,"r");if(!f){server.send(404,"text/plain","Not found");return;}server.streamFile(f,type);f.close();}
void statusRoute(){JsonDocument d;const auto&s=controller.state();uint32_t ago=lastUploadSuccess?(millis()-lastUploadSuccess)/1000:0;
 d["deviceName"]=DEVICE_NAME;d["demoMode"]=DEMO_MODE;d["uptimeSeconds"]=millis()/1000;d["mode"]=toString(controller.mode());d["curtainState"]=toString(controller.curtainState());d["estimatedPosition"]=controller.displayedPosition();d["positionKnown"]=s.positionKnown;d["motorState"]=toString(motorController.state());d["errorCode"]=toString(controller.error());d["rainRaw"]=rainSensor.raw();d["rainFiltered"]=rainSensor.filtered();d["rainDetected"]=rainSensor.stable()==RainStableState::WET;d["rainStableState"]=toString(rainSensor.stable());d["rainEventCount"]=s.rainEventCount;d["movementElapsedMs"]=motorController.elapsed();d["movementTargetMs"]=motorController.targetDuration();d["movementRemainingMs"]=motorController.remaining();d["fullTravelTimeMs"]=controller.config().fullTravelTimeMs;d["motorPwmSpeed"]=controller.config().motorPwmSpeed;d["wifiConnected"]=WiFi.status()==WL_CONNECTED;d["wifiRssi"]=WiFi.status()==WL_CONNECTED?WiFi.RSSI():0;d["ipAddress"]=WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():"";d["thingSpeakLastStatus"]=thingSpeakStatus;d["thingSpeakLastUploadSecondsAgo"]=ago;sendJson(d);}
void configGet(){const auto&c=controller.config();JsonDocument d;d["wetThreshold"]=c.wetThreshold;d["dryThreshold"]=c.dryThreshold;d["rainValueIncreasesWhenWet"]=c.rainValueIncreasesWhenWet;d["rainConfirmationMs"]=c.rainConfirmationMs;d["dryConfirmationMs"]=c.dryConfirmationMs;d["fullTravelTimeMs"]=c.fullTravelTimeMs;d["motorPwmSpeed"]=c.motorPwmSpeed;d["motorDirectionReversed"]=c.motorDirectionReversed;d["motorTimeoutMarginMs"]=c.motorTimeoutMarginMs;sendJson(d);}
void controlPost(){JsonDocument in,out;if(!parseBody(in)){out["success"]=false;out["message"]="JSON không hợp lệ";sendJson(out,400);return;}String cmd=in["command"]|"";bool confirmed=in["confirmed"].is<bool>()?in["confirmed"].as<bool>():boolArg(in["confirmed"]|"");String msg;int code;bool ok=controller.command(cmd,confirmed,msg,code);out["success"]=ok;out["message"]=msg;out["mode"]=toString(controller.mode());out["curtainState"]=toString(controller.curtainState());sendJson(out,code);}
void configPost(){JsonDocument in,out;if(!parseBody(in)){out["success"]=false;out["message"]="JSON không hợp lệ";sendJson(out,400);return;}AppConfig c=controller.config();
#define GETNUM(k,field,type) if(in[k].is<type>())c.field=in[k].as<type>();else if(in[k].is<const char*>())c.field=(type)String(in[k].as<const char*>()).toInt()
 GETNUM("wetThreshold",wetThreshold,int);GETNUM("dryThreshold",dryThreshold,int);GETNUM("rainConfirmationMs",rainConfirmationMs,uint32_t);GETNUM("dryConfirmationMs",dryConfirmationMs,uint32_t);GETNUM("fullTravelTimeMs",fullTravelTimeMs,uint32_t);GETNUM("motorPwmSpeed",motorPwmSpeed,uint16_t);GETNUM("motorTimeoutMarginMs",motorTimeoutMarginMs,uint32_t);
#undef GETNUM
 if(!in["rainValueIncreasesWhenWet"].isNull()) {
   c.rainValueIncreasesWhenWet=in["rainValueIncreasesWhenWet"].is<bool>()?in["rainValueIncreasesWhenWet"].as<bool>():boolArg(in["rainValueIncreasesWhenWet"]|"");
 }
 if(!in["motorDirectionReversed"].isNull()) {
   c.motorDirectionReversed=in["motorDirectionReversed"].is<bool>()?in["motorDirectionReversed"].as<bool>():boolArg(in["motorDirectionReversed"]|"");
 }
 bool ok=controller.applyConfig(c);
 out["success"]=ok;out["message"]=ok?"Đã lưu cấu hình":"Cấu hình không hợp lệ hoặc không thể lưu";sendJson(out,ok?200:400);}
void setupRoutes(){server.on("/",HTTP_GET,[]{serveFile("/index.html","text/html; charset=utf-8");});server.on("/style.css",HTTP_GET,[]{serveFile("/style.css","text/css");});server.on("/app.js",HTTP_GET,[]{serveFile("/app.js","application/javascript");});server.on("/api/status",HTTP_GET,statusRoute);server.on("/api/control",HTTP_POST,controlPost);server.on("/api/config",HTTP_GET,configGet);server.on("/api/config",HTTP_POST,configPost);server.on("/api/error/clear",HTTP_POST,[]{controller.clearError();JsonDocument d;d["success"]=true;d["errorCode"]=toString(controller.error());d["positionKnown"]=controller.state().positionKnown;sendJson(d);});server.on("/health",HTTP_GET,[]{server.send(200,"text/plain","OK");});
 if(DEMO_MODE){server.on("/api/demo/rain",HTTP_POST,[]{String v=server.arg("state");rainSensor.setDemoState(v=="wet"?RainStableState::WET:RainStableState::DRY);JsonDocument d;d["success"]=true;sendJson(d);});}
 server.onNotFound([]{JsonDocument d;d["success"]=false;d["message"]="Không tìm thấy";sendJson(d,404);});server.begin();}
void updateWiFi(){uint32_t now=millis();static bool was=false;bool connected=WiFi.status()==WL_CONNECTED;if(connected&&!was){Serial.print(F("Wi-Fi connected, IP: "));Serial.println(WiFi.localIP());if(!mdnsStarted){mdnsStarted=MDNS.begin(MDNS_NAME);if(mdnsStarted)Serial.println(F("mDNS: http://greenguard.local"));}}if(!connected&&was)Serial.println(F("Wi-Fi disconnected; local automation continues"));was=connected;if(!connected&&(uint32_t)(now-lastWifiAttempt)>=WIFI_RETRY_MS){lastWifiAttempt=now;WiFi.begin(WIFI_SSID,WIFI_PASSWORD);}}
void updateThingSpeak(){uint32_t now=millis();if(WiFi.status()!=WL_CONNECTED||(uint32_t)(now-lastUpload)<THINGSPEAK_INTERVAL_MS||motorController.state()==MotorState::MOTOR_REVERSING)return;lastUpload=now;ThingSpeak.setField(1,(float)rainSensor.filtered());ThingSpeak.setField(2,rainSensor.stable()==RainStableState::WET?1:0);ThingSpeak.setField(3,controller.state().estimatedPosition);ThingSpeak.setField(4,controller.mode()==OperatingMode::AUTO?0:1);ThingSpeak.setField(5,(long)controller.state().rainEventCount);ThingSpeak.setField(6,(int)motorController.state());ThingSpeak.setField(7,WiFi.RSSI());ThingSpeak.setField(8,(int)controller.error());thingSpeakStatus=ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID,THINGSPEAK_WRITE_API_KEY);if(thingSpeakStatus==200)lastUploadSuccess=now;Serial.printf("ThingSpeak status: %d\n",thingSpeakStatus);}
void setup(){motorController.begin();Serial.begin(115200);Serial.println();Serial.println(F("GreenGuard boot"));Serial.print(F("Reset reason: "));Serial.println(ESP.getResetReason());fsReady=storage.begin();Serial.println(fsReady?F("LittleFS mounted"):F("LittleFS mount failed"));AppConfig cfg;PersistentState state;if(fsReady){if(!storage.loadConfig(cfg)){storage.saveConfig(cfg);Serial.println(F("Default configuration loaded"));}else Serial.println(F("Configuration loaded"));if(!storage.loadState(state)){state=PersistentState();storage.saveState(state);Serial.println(F("Safe default state loaded"));}else Serial.println(F("Position state loaded"));}rainSensor.begin(cfg);controller.begin(state,cfg);WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);lastWifiAttempt=millis();ThingSpeak.begin(cloudClient);setupRoutes();}
void loop(){rainSensor.update();motorController.update();controller.update();server.handleClient();updateWiFi();updateThingSpeak();if(mdnsStarted)MDNS.update();yield();}
