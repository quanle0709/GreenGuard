#pragma once
#include <Arduino.h>
#include "config.h"

enum class CurtainState { UNKNOWN, OPEN, CLOSED, PARTIALLY_OPEN, OPENING, CLOSING, STOPPED, ERROR };
enum class OperatingMode { AUTO, MANUAL };
enum class MotorState { MOTOR_STOPPED, MOTOR_OPENING, MOTOR_CLOSING, MOTOR_REVERSING };
enum class RainStableState { UNKNOWN, DRY, WET };
enum class ErrorCode { NO_ERROR, MOTOR_TIMEOUT, FILESYSTEM_ERROR, INVALID_CONFIGURATION, POSITION_UNKNOWN, STORAGE_ERROR };

inline const char* toString(CurtainState v) { switch(v){case CurtainState::OPEN:return "OPEN";case CurtainState::CLOSED:return "CLOSED";case CurtainState::PARTIALLY_OPEN:return "PARTIALLY_OPEN";case CurtainState::OPENING:return "OPENING";case CurtainState::CLOSING:return "CLOSING";case CurtainState::STOPPED:return "STOPPED";case CurtainState::ERROR:return "ERROR";default:return "UNKNOWN";} }
inline const char* toString(OperatingMode v) { return v==OperatingMode::AUTO?"AUTO":"MANUAL"; }
inline const char* toString(MotorState v) { switch(v){case MotorState::MOTOR_OPENING:return "MOTOR_OPENING";case MotorState::MOTOR_CLOSING:return "MOTOR_CLOSING";case MotorState::MOTOR_REVERSING:return "MOTOR_REVERSING";default:return "MOTOR_STOPPED";} }
inline const char* toString(RainStableState v) { return v==RainStableState::WET?"WET":v==RainStableState::DRY?"DRY":"UNKNOWN"; }
inline const char* toString(ErrorCode v) { switch(v){case ErrorCode::MOTOR_TIMEOUT:return "MOTOR_TIMEOUT";case ErrorCode::FILESYSTEM_ERROR:return "FILESYSTEM_ERROR";case ErrorCode::INVALID_CONFIGURATION:return "INVALID_CONFIGURATION";case ErrorCode::POSITION_UNKNOWN:return "POSITION_UNKNOWN";case ErrorCode::STORAGE_ERROR:return "STORAGE_ERROR";default:return "NO_ERROR";} }

struct AppConfig {
  int wetThreshold=500, dryThreshold=650;
  bool rainValueIncreasesWhenWet=false, motorDirectionReversed=false;
  uint32_t rainConfirmationMs=3000, dryConfirmationMs=30000;
  uint32_t fullTravelTimeMs=DEFAULT_FULL_TRAVEL_TIME_MS;
  uint16_t motorPwmSpeed=750;
  uint32_t motorTimeoutMarginMs=DEFAULT_MOTOR_TIMEOUT_MARGIN_MS;
};
struct PersistentState { float estimatedPosition=0; bool positionKnown=false; bool movementWasActive=false; uint32_t rainEventCount=0; };
