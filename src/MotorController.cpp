#include "MotorController.h"
#include "config.h"
#include <math.h>
// R_EN and L_EN are tied permanently to the BTS7960 5 V logic supply.
// The motor is stopped exclusively by forcing both PWM inputs LOW.
void MotorController::outputsOff(){analogWrite(PIN_MOTOR_RPWM,0);analogWrite(PIN_MOTOR_LPWM,0);}
bool MotorController::begin(){analogWriteRange(1023);pinMode(PIN_MOTOR_RPWM,OUTPUT);pinMode(PIN_MOTOR_LPWM,OUTPUT);outputsOff();return true;}
uint32_t MotorController::elapsed()const{return state_==MotorState::MOTOR_STOPPED?0:(uint32_t)(millis()-started_);} uint32_t MotorController::remaining()const{uint32_t e=elapsed();return e>=duration_?0:duration_-e;}
void MotorController::activate(MotorState d){outputsOff();pending_=MotorState::MOTOR_STOPPED;state_=d;lastDirection_=d;started_=millis();if(DEMO_MODE)return;bool physicalOpen=(d==MotorState::MOTOR_OPENING)^cfg_.motorDirectionReversed;analogWrite(physicalOpen?PIN_MOTOR_RPWM:PIN_MOTOR_LPWM,cfg_.motorPwmSpeed);}
bool MotorController::prepare(float cur,float target,MotorState d,uint32_t duration){if(error_!=ErrorCode::NO_ERROR)return false;startPosition_=cur;targetPosition_=target;duration_=duration;complete_=timedOut_=false;return requestDirection(d);}
bool MotorController::openToTarget(float c,float t){return prepare(c,t,MotorState::MOTOR_OPENING,(uint32_t)(cfg_.fullTravelTimeMs*fabsf(t-c)/100.0f));}
bool MotorController::closeToTarget(float c,float t){return prepare(c,t,MotorState::MOTOR_CLOSING,(uint32_t)(cfg_.fullTravelTimeMs*fabsf(t-c)/100.0f));}
bool MotorController::startUnknown(MotorState d){return prepare(d==MotorState::MOTOR_OPENING?0:100,d==MotorState::MOTOR_OPENING?100:0,d,cfg_.fullTravelTimeMs);}
bool MotorController::requestDirection(MotorState d){if(d!=MotorState::MOTOR_OPENING&&d!=MotorState::MOTOR_CLOSING)return false;if(state_==d)return true;if((state_==MotorState::MOTOR_OPENING||state_==MotorState::MOTOR_CLOSING)&&state_!=d){outputsOff();pending_=d;state_=MotorState::MOTOR_REVERSING;reverseStarted_=millis();Serial.println(F("Motor: direction reversal delay"));return true;}activate(d);return true;}
void MotorController::stop(bool){if(state_==MotorState::MOTOR_OPENING||state_==MotorState::MOTOR_CLOSING)lastElapsed_=millis()-started_;outputsOff();state_=MotorState::MOTOR_STOPPED;pending_=MotorState::MOTOR_STOPPED;}
void MotorController::emergencyStop(ErrorCode e){stop(false);error_=e;timedOut_=true;}
void MotorController::update(){uint32_t now=millis();if(state_==MotorState::MOTOR_REVERSING){if((uint32_t)(now-reverseStarted_)>=MOTOR_REVERSE_DELAY_MS)activate(pending_);return;}if(state_==MotorState::MOTOR_OPENING||state_==MotorState::MOTOR_CLOSING){uint32_t e=now-started_;if(e>duration_+cfg_.motorTimeoutMarginMs){emergencyStop(ErrorCode::MOTOR_TIMEOUT);return;}if(e>=duration_){stop(true);complete_=true;}}}
bool MotorController::completionPending(){bool v=complete_;complete_=false;return v;}bool MotorController::timeoutPending(){bool v=timedOut_;timedOut_=false;return v;}
