#pragma once
#include "PersistentStorage.h"
#include "RainSensor.h"
#include "MotorController.h"
class GreenGuardController {
 public:
  GreenGuardController(PersistentStorage& s,RainSensor& r,MotorController& m):storage_(s),rain_(r),motor_(m){}
  void begin(const PersistentState&,const AppConfig&); void update();
  bool command(const String&,bool confirmed,String& message,int& status);
  bool applyConfig(const AppConfig&); bool clearError();
  const AppConfig& config()const{return cfg_;} const PersistentState& state()const{return state_;}
  OperatingMode mode()const{return mode_;} CurtainState curtainState()const; ErrorCode error()const;
  float displayedPosition()const{return currentPosition();}
 private:
  bool startTo(float target,bool unknown=false); void stopAndSave(bool manual); void save(); void evaluateAuto(); float currentPosition()const;
  PersistentStorage& storage_; RainSensor& rain_; MotorController& motor_; AppConfig cfg_; PersistentState state_;
  OperatingMode mode_=OperatingMode::AUTO; RainStableState lastRain_=RainStableState::UNKNOWN;
};
