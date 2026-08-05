#include "PersistentStorage.h"
#include <LittleFS.h>

bool PersistentStorage::begin(){ return LittleFS.begin(); }
bool PersistentStorage::validateConfig(const AppConfig& c){
 if(c.rainConfirmationMs<500||c.rainConfirmationMs>30000||c.dryConfirmationMs<3000||c.dryConfirmationMs>300000) return false;
 if(c.fullTravelTimeMs<5000||c.fullTravelTimeMs>60000||c.motorPwmSpeed>1023||c.motorTimeoutMarginMs<2000||c.motorTimeoutMarginMs>15000) return false;
 return true;
}
bool PersistentStorage::atomicWrite(const char* path,const JsonDocument& doc){
 String tmp=String(path)+".tmp"; File f=LittleFS.open(tmp,"w"); if(!f)return false;
 size_t n=serializeJson(doc,f); f.flush(); f.close(); if(!n){LittleFS.remove(tmp);return false;}
 JsonDocument check; f=LittleFS.open(tmp,"r"); bool ok=f&&!deserializeJson(check,f); f.close(); if(!ok){LittleFS.remove(tmp);return false;}
 LittleFS.remove(path); return LittleFS.rename(tmp,path);
}
bool PersistentStorage::loadConfig(AppConfig& c){
 File f=LittleFS.open("/config.json","r"); if(!f)return false; JsonDocument d; if(deserializeJson(d,f)){f.close();return false;} f.close();
 AppConfig n; n.rainDigitalActiveLow=d["rainDigitalActiveLow"]|true;
 n.rainConfirmationMs=d["rainConfirmationMs"]|3000; n.dryConfirmationMs=d["dryConfirmationMs"]|30000; n.fullTravelTimeMs=d["fullTravelTimeMs"]|30000;
 n.motorPwmSpeed=d["motorPwmSpeed"]|750; n.motorDirectionReversed=d["motorDirectionReversed"]|false; n.motorTimeoutMarginMs=d["motorTimeoutMarginMs"]|5000;
 if(!validateConfig(n)) return false;
 c=n;
 return true;
}
bool PersistentStorage::saveConfig(const AppConfig& c){ JsonDocument d; d["rainDigitalActiveLow"]=c.rainDigitalActiveLow;d["rainConfirmationMs"]=c.rainConfirmationMs;d["dryConfirmationMs"]=c.dryConfirmationMs;d["fullTravelTimeMs"]=c.fullTravelTimeMs;d["motorPwmSpeed"]=c.motorPwmSpeed;d["motorDirectionReversed"]=c.motorDirectionReversed;d["motorTimeoutMarginMs"]=c.motorTimeoutMarginMs;return atomicWrite("/config.json",d); }
bool PersistentStorage::loadState(PersistentState& s){ File f=LittleFS.open("/state.json","r");if(!f)return false;JsonDocument d;if(deserializeJson(d,f)){f.close();return false;}f.close(); if(!d["estimatedPosition"].is<float>()&&!d["estimatedPosition"].is<int>())return false;s.estimatedPosition=constrain(d["estimatedPosition"].as<float>(),0.0f,100.0f);s.positionKnown=d["positionKnown"]|false;s.movementWasActive=d["movementWasActive"]|false;s.rainEventCount=d["rainEventCount"]|0;return true; }
bool PersistentStorage::saveState(const PersistentState& s){JsonDocument d;d["estimatedPosition"]=s.estimatedPosition;d["positionKnown"]=s.positionKnown;d["movementWasActive"]=s.movementWasActive;d["rainEventCount"]=s.rainEventCount;return atomicWrite("/state.json",d);}
