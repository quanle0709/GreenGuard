#pragma once

#include <stdint.h>

namespace greenguard {

enum class Mode : uint8_t { AUTO = 0, MANUAL = 1 };
enum class RainState : uint8_t { UNKNOWN = 0, DRY = 1, WET = 2 };
enum class ShieldState : uint8_t { UNKNOWN = 0, RETRACTED = 1, DEPLOYED = 2, STOPPED_PARTIAL = 3 };
enum class PositionConfidence : uint8_t { UNKNOWN = 0, ESTIMATED = 1, USER_CALIBRATED = 2, LIMIT_CONFIRMED = 3 };
enum class MotionState : uint8_t { STOPPED = 0, DEPLOYING = 1, RETRACTING = 2, REVERSAL_DEAD_TIME = 3 };
enum class Fault : uint8_t { NONE = 0, MOTOR_TIMEOUT = 1, LIMIT_CONFLICT = 2, PERSISTENCE_INVALID = 3, STORAGE_ERROR = 4 };
enum class Command : uint8_t { SET_AUTO, SET_MANUAL, DEPLOY, RETRACT, STOP, RESET_FAULT, CALIBRATE_DEPLOYED, CALIBRATE_RETRACTED, INVALID };
enum class CommandResult : uint8_t { IDLE = 0, ACCEPTED = 1, REJECTED_MODE = 2, REJECTED_RAIN = 3, REJECTED_FAULT = 4, DUPLICATE = 5, INVALID = 6, REJECTED_STATE = 7 };
enum class CommandPhase : uint8_t { NONE = 0, ACCEPTED = 1, STARTED = 2, COMPLETED = 3, STOPPED = 4, REJECTED = 5, FAULT = 6 };
enum class Drive : uint8_t { OFF = 0, DEPLOY = 1, RETRACT = 2 };

struct Config {
  uint32_t wetConfirmMs = 3000;
  uint32_t dryConfirmMs = 120000;
  uint32_t directionDeadTimeMs = 300;
  uint32_t inferredFullTravelMs = 30000;
  uint32_t motorMaxRuntimeMs = 35000;
  bool useLimitSwitches = false;
};

struct Inputs {
  uint32_t nowMs = 0;
  bool rainWet = false;
  bool retractedLimit = false;
  bool deployedLimit = false;
};

struct Snapshot {
  Mode mode = Mode::AUTO;
  ShieldState shield = ShieldState::UNKNOWN;
  PositionConfidence confidence = PositionConfidence::UNKNOWN;
  float estimatedPositionPct = 0.0f;
  bool estimateAvailable = false;
  bool stopLatched = false;
  Fault fault = Fault::NONE;
  bool movementActive = false;
  uint32_t rainEventCount = 0;
};

struct CommandResponse {
  CommandResult result = CommandResult::IDLE;
  CommandPhase phase = CommandPhase::NONE;
  bool accepted = false;
};

class Controller {
 public:
  explicit Controller(const Config& config = Config());

  static bool validConfig(const Config& config);
  void begin(const Snapshot& restored, const Inputs& inputs, bool persistenceValid = true);
  void tick(const Inputs& inputs);
  CommandResponse command(Command command, uint32_t requestId, const Inputs& inputs);
  bool configure(const Config& config);
  void raiseFault(Fault fault, uint32_t nowMs);

  Mode mode() const { return mode_; }
  RainState rain() const { return rain_; }
  ShieldState shield() const { return shield_; }
  PositionConfidence confidence() const { return confidence_; }
  MotionState motion() const { return motion_; }
  Fault fault() const { return fault_; }
  Drive drive() const;
  bool stopLatched() const { return stopLatched_; }
  bool estimateAvailable() const { return estimateAvailable_; }
  float estimatedPositionPct() const { return estimatedPositionPct_; }
  uint32_t rainEventCount() const { return rainEventCount_; }
  uint32_t lastRequestId() const { return lastRequestId_; }
  Command lastCommand() const { return lastCommand_; }
  CommandResult lastCommandResult() const { return lastCommandResult_; }
  CommandPhase commandPhase() const { return commandPhase_; }
  uint32_t movementElapsedMs(uint32_t nowMs) const;
  uint32_t movementDurationMs() const { return movementDurationMs_; }
  Snapshot snapshot() const;

 private:
  static bool elapsed(uint32_t now, uint32_t since, uint32_t interval);
  static float clampPosition(float value);
  static bool validSnapshot(const Snapshot& snapshot);
  void updateRain(const Inputs& inputs);
  void updateMotion(const Inputs& inputs);
  void runAutomatic(const Inputs& inputs);
  void requestMove(ShieldState target, const Inputs& inputs, uint32_t requestId);
  void startPendingMove(uint32_t nowMs);
  void updateEstimate(uint32_t nowMs);
  void stopAt(ShieldState target, PositionConfidence confidence);
  void stopPartial(uint32_t nowMs);
  void setFault(Fault fault, uint32_t nowMs);
  CommandResponse reject(CommandResult result);
  CommandResponse complete(CommandResult result = CommandResult::ACCEPTED);
  bool targetLimitActive(ShieldState target, const Inputs& inputs) const;

  Config config_;
  Mode mode_ = Mode::AUTO;
  RainState rain_ = RainState::UNKNOWN;
  ShieldState shield_ = ShieldState::UNKNOWN;
  PositionConfidence confidence_ = PositionConfidence::UNKNOWN;
  MotionState motion_ = MotionState::STOPPED;
  Fault fault_ = Fault::NONE;
  bool stopLatched_ = false;
  bool estimateAvailable_ = false;
  float estimatedPositionPct_ = 0.0f;
  uint32_t rainEventCount_ = 0;

  bool rainCandidateWet_ = false;
  uint32_t rainCandidateSince_ = 0;
  uint32_t motionStartedAt_ = 0;
  uint32_t deadTimeStartedAt_ = 0;
  uint32_t movementDurationMs_ = 0;
  float motionStartPositionPct_ = 0.0f;
  float motionTargetPositionPct_ = 0.0f;
  ShieldState pendingTarget_ = ShieldState::UNKNOWN;
  uint32_t motionRequestId_ = 0;

  uint32_t lastRequestId_ = 0;
  Command lastCommand_ = Command::INVALID;
  CommandResult lastCommandResult_ = CommandResult::IDLE;
  CommandPhase commandPhase_ = CommandPhase::NONE;
};

const char* toString(Mode value);
const char* toString(RainState value);
const char* toString(ShieldState value);
const char* toString(PositionConfidence value);
const char* toString(MotionState value);
const char* toString(Fault value);
const char* toString(Command value);
const char* toString(CommandResult value);
const char* toString(CommandPhase value);
Command commandFromString(const char* value);

}  // namespace greenguard
