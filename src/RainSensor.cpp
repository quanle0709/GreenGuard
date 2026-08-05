#include "RainSensor.h"
#include "config.h"
void RainSensor::begin(const AppConfig& c){cfg_=c;pinMode(PIN_RAIN_ANALOG,INPUT);}
void RainSensor::configure(const AppConfig& c){cfg_=c;candidate_=RainStableState::UNKNOWN;candidateSince_=millis();}
void RainSensor::update(){uint32_t now=millis();if((uint32_t)(now-lastRead_)<SENSOR_INTERVAL_MS)return;lastRead_=now;raw_=analogRead(PIN_RAIN_ANALOG);samples_[index_++]=raw_;index_%=10;if(count_<10)count_++;long sum=0;for(uint8_t i=0;i<count_;i++)sum+=samples_[i];filtered_=(float)sum/count_;
 RainStableState next=candidate_; if(cfg_.rainValueIncreasesWhenWet){if(filtered_>=cfg_.wetThreshold)next=RainStableState::WET;else if(filtered_<=cfg_.dryThreshold)next=RainStableState::DRY;}else{if(filtered_<=cfg_.wetThreshold)next=RainStableState::WET;else if(filtered_>=cfg_.dryThreshold)next=RainStableState::DRY;}
 if(next!=candidate_){candidate_=next;candidateSince_=now;} if(candidate_!=RainStableState::UNKNOWN&&candidate_!=stable_){uint32_t need=candidate_==RainStableState::WET?cfg_.rainConfirmationMs:cfg_.dryConfirmationMs;if((uint32_t)(now-candidateSince_)>=need){stable_=candidate_;changed_=true;}}
}
bool RainSensor::consumeStableChange(RainStableState& s){if(!changed_)return false;changed_=false;s=stable_;return true;}
void RainSensor::setDemoState(RainStableState s){if(!DEMO_MODE)return;candidate_=stable_=s;changed_=true;candidateSince_=millis();}

