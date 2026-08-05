#include "RainSensor.h"
#include "config.h"
void RainSensor::begin(const AppConfig& c){cfg_=c;pinMode(PIN_RAIN_DIGITAL,INPUT);}
void RainSensor::configure(const AppConfig& c){cfg_=c;candidate_=RainStableState::UNKNOWN;candidateSince_=millis();}
void RainSensor::update(){uint32_t now=millis();if((uint32_t)(now-lastRead_)<SENSOR_INTERVAL_MS)return;lastRead_=now;digitalLevel_=digitalRead(PIN_RAIN_DIGITAL);inputActive_=cfg_.rainDigitalActiveLow?digitalLevel_==LOW:digitalLevel_==HIGH;
 RainStableState next=inputActive_?RainStableState::WET:RainStableState::DRY;
 if(next!=candidate_){candidate_=next;candidateSince_=now;} if(candidate_!=RainStableState::UNKNOWN&&candidate_!=stable_){uint32_t need=candidate_==RainStableState::WET?cfg_.rainConfirmationMs:cfg_.dryConfirmationMs;if((uint32_t)(now-candidateSince_)>=need){stable_=candidate_;changed_=true;}}
}
bool RainSensor::consumeStableChange(RainStableState& s){if(!changed_)return false;changed_=false;s=stable_;return true;}
void RainSensor::setDemoState(RainStableState s){if(!DEMO_MODE)return;candidate_=stable_=s;changed_=true;candidateSince_=millis();}
