#pragma once
#include "types.h"
class RainSensor {
 public:
  void begin(const AppConfig&); void configure(const AppConfig&); void update();
  uint8_t digitalLevel()const{return digitalLevel_;} bool inputActive()const{return inputActive_;}
  RainStableState candidate()const{return candidate_;} RainStableState stable()const{return stable_;}
  bool consumeStableChange(RainStableState& state);
  void setDemoState(RainStableState s);
 private:
  AppConfig cfg_; uint8_t digitalLevel_=HIGH; bool inputActive_=false;
  uint32_t lastRead_=0,candidateSince_=0; RainStableState candidate_=RainStableState::UNKNOWN,stable_=RainStableState::UNKNOWN; bool changed_=false;
};
