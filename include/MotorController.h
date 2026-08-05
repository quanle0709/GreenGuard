#pragma once
#include "types.h"
class MotorController {
 public:
  bool begin(); void configure(const AppConfig& c){cfg_=c;} void update();
  bool openToTarget(float current,float target); bool closeToTarget(float current,float target);
  bool startUnknown(MotorState direction); bool requestDirection(MotorState direction);
  void stop(bool normalStop); void emergencyStop(ErrorCode error); void clearError(){error_=ErrorCode::NO_ERROR;}
  MotorState state()const{return state_;} ErrorCode error()const{return error_;}
  uint32_t elapsed()const; uint32_t targetDuration()const{return duration_;} uint32_t remaining()const;
  uint32_t lastElapsed()const{return lastElapsed_;} MotorState lastDirection()const{return lastDirection_;}
  float startPosition()const{return startPosition_;} float targetPosition()const{return targetPosition_;}
  bool completionPending(); bool timeoutPending(); bool isMoving()const{return state_!=MotorState::MOTOR_STOPPED;}
 private:
  void outputsOff(); void activate(MotorState); bool prepare(float,float,MotorState,uint32_t);
  AppConfig cfg_; MotorState state_=MotorState::MOTOR_STOPPED,pending_=MotorState::MOTOR_STOPPED;
  ErrorCode error_=ErrorCode::NO_ERROR; uint32_t started_=0,duration_=0,reverseStarted_=0; float startPosition_=0,targetPosition_=0;
  bool complete_=false,timedOut_=false; uint32_t lastElapsed_=0; MotorState lastDirection_=MotorState::MOTOR_STOPPED;
};
