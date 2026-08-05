#pragma once
#include "types.h"
class RainSensor {
 public:
  void begin(const AppConfig&); void configure(const AppConfig&); void update();
  int raw()const{return raw_;} float filtered()const{return filtered_;}
  RainStableState candidate()const{return candidate_;} RainStableState stable()const{return stable_;}
  bool consumeStableChange(RainStableState& state);
  void setDemoState(RainStableState s);
 private:
  AppConfig cfg_; int samples_[10]={0},raw_=0; uint8_t index_=0,count_=0; float filtered_=0;
  uint32_t lastRead_=0,candidateSince_=0; RainStableState candidate_=RainStableState::UNKNOWN,stable_=RainStableState::UNKNOWN; bool changed_=false;
};

