#include "GreenGuardCore.h"

#include <math.h>
#include <string.h>

namespace greenguard {

Controller::Controller(const Config& config) : config_(config) {
  if (!validConfig(config_)) config_ = Config();
}

bool Controller::elapsed(uint32_t now, uint32_t since, uint32_t interval) {
  return static_cast<uint32_t>(now - since) >= interval;
}

float Controller::clampPosition(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 100.0f) return 100.0f;
  return value;
}

bool Controller::validConfig(const Config& config) {
  return config.wetConfirmMs >= 250 && config.wetConfirmMs <= 30000 &&
         config.dryConfirmMs >= 1000 && config.dryConfirmMs <= 600000 &&
         config.dryConfirmMs > config.wetConfirmMs &&
         config.directionDeadTimeMs >= 100 && config.directionDeadTimeMs <= 5000 &&
         config.inferredFullTravelMs >= 1000 && config.inferredFullTravelMs <= 300000 &&
         config.motorMaxRuntimeMs >= config.inferredFullTravelMs &&
         config.motorMaxRuntimeMs <= 360000;
}

bool Controller::validSnapshot(const Snapshot& snapshot) {
  const bool modeValid = snapshot.mode == Mode::AUTO || snapshot.mode == Mode::MANUAL;
  const bool shieldValid = snapshot.shield == ShieldState::UNKNOWN || snapshot.shield == ShieldState::RETRACTED ||
                           snapshot.shield == ShieldState::DEPLOYED || snapshot.shield == ShieldState::STOPPED_PARTIAL;
  const bool confidenceValid = snapshot.confidence == PositionConfidence::UNKNOWN ||
                               snapshot.confidence == PositionConfidence::ESTIMATED ||
                               snapshot.confidence == PositionConfidence::USER_CALIBRATED ||
                               snapshot.confidence == PositionConfidence::LIMIT_CONFIRMED;
  const bool faultValid = snapshot.fault == Fault::NONE || snapshot.fault == Fault::MOTOR_TIMEOUT ||
                          snapshot.fault == Fault::LIMIT_CONFLICT || snapshot.fault == Fault::PERSISTENCE_INVALID ||
                          snapshot.fault == Fault::STORAGE_ERROR;
  const bool estimateValid = !snapshot.estimateAvailable ||
                             (isfinite(snapshot.estimatedPositionPct) && snapshot.estimatedPositionPct >= 0.0f &&
                              snapshot.estimatedPositionPct <= 100.0f);
  return modeValid && shieldValid && confidenceValid && faultValid && estimateValid;
}

void Controller::begin(const Snapshot& restored, const Inputs& inputs, bool persistenceValid) {
  mode_ = Mode::MANUAL;
  rain_ = RainState::UNKNOWN;
  shield_ = ShieldState::UNKNOWN;
  confidence_ = PositionConfidence::UNKNOWN;
  motion_ = MotionState::STOPPED;
  fault_ = Fault::NONE;
  stopLatched_ = false;
  estimateAvailable_ = false;
  estimatedPositionPct_ = 0.0f;
  rainEventCount_ = 0;
  pendingTarget_ = ShieldState::UNKNOWN;
  movementDurationMs_ = 0;
  motionRequestId_ = 0;
  lastRequestId_ = 0;
  lastCommand_ = Command::INVALID;
  lastCommandResult_ = CommandResult::IDLE;
  commandPhase_ = CommandPhase::NONE;
  rainCandidateWet_ = inputs.rainWet;
  rainCandidateSince_ = inputs.nowMs;

  if (!persistenceValid || !validSnapshot(restored)) {
    fault_ = Fault::PERSISTENCE_INVALID;
    stopLatched_ = true;
    return;
  }

  mode_ = restored.mode;
  shield_ = restored.shield;
  confidence_ = restored.confidence;
  estimateAvailable_ = restored.estimateAvailable;
  estimatedPositionPct_ = clampPosition(restored.estimatedPositionPct);
  stopLatched_ = restored.stopLatched;
  fault_ = restored.fault;
  rainEventCount_ = restored.rainEventCount;

  if (restored.movementActive) {
    shield_ = ShieldState::UNKNOWN;
    confidence_ = PositionConfidence::UNKNOWN;
    estimateAvailable_ = false;
    stopLatched_ = true;
  }

  if (config_.useLimitSwitches) {
    if (inputs.retractedLimit && inputs.deployedLimit) {
      setFault(Fault::LIMIT_CONFLICT, inputs.nowMs);
    } else if (inputs.retractedLimit) {
      shield_ = ShieldState::RETRACTED;
      confidence_ = PositionConfidence::LIMIT_CONFIRMED;
      estimatedPositionPct_ = 0.0f;
      estimateAvailable_ = true;
    } else if (inputs.deployedLimit) {
      shield_ = ShieldState::DEPLOYED;
      confidence_ = PositionConfidence::LIMIT_CONFIRMED;
      estimatedPositionPct_ = 100.0f;
      estimateAvailable_ = true;
    }
  }
}

bool Controller::configure(const Config& config) {
  if (!validConfig(config) || motion_ != MotionState::STOPPED) return false;
  config_ = config;
  return true;
}

void Controller::raiseFault(Fault fault, uint32_t nowMs) {
  if (fault != Fault::NONE) setFault(fault, nowMs);
}

Drive Controller::drive() const {
  if (motion_ == MotionState::DEPLOYING) return Drive::DEPLOY;
  if (motion_ == MotionState::RETRACTING) return Drive::RETRACT;
  return Drive::OFF;
}

uint32_t Controller::movementElapsedMs(uint32_t nowMs) const {
  if (motion_ != MotionState::DEPLOYING && motion_ != MotionState::RETRACTING) return 0;
  return static_cast<uint32_t>(nowMs - motionStartedAt_);
}

void Controller::updateRain(const Inputs& inputs) {
  if (inputs.rainWet != rainCandidateWet_) {
    rainCandidateWet_ = inputs.rainWet;
    rainCandidateSince_ = inputs.nowMs;
    return;
  }
  const uint32_t required = rainCandidateWet_ ? config_.wetConfirmMs : config_.dryConfirmMs;
  if (!elapsed(inputs.nowMs, rainCandidateSince_, required)) return;
  const RainState next = rainCandidateWet_ ? RainState::WET : RainState::DRY;
  if (next == rain_) return;
  if (next == RainState::WET) ++rainEventCount_;
  rain_ = next;
}

bool Controller::targetLimitActive(ShieldState target, const Inputs& inputs) const {
  if (!config_.useLimitSwitches) return false;
  return (target == ShieldState::DEPLOYED && inputs.deployedLimit) ||
         (target == ShieldState::RETRACTED && inputs.retractedLimit);
}

void Controller::updateEstimate(uint32_t nowMs) {
  if (motion_ != MotionState::DEPLOYING && motion_ != MotionState::RETRACTING) return;
  const uint32_t run = static_cast<uint32_t>(nowMs - motionStartedAt_);
  float fraction = movementDurationMs_ == 0 ? 1.0f : static_cast<float>(run) / movementDurationMs_;
  if (fraction > 1.0f) fraction = 1.0f;
  estimatedPositionPct_ = clampPosition(motionStartPositionPct_ +
      (motionTargetPositionPct_ - motionStartPositionPct_) * fraction);
  estimateAvailable_ = true;
  confidence_ = PositionConfidence::ESTIMATED;
}

void Controller::startPendingMove(uint32_t nowMs) {
  if (pendingTarget_ != ShieldState::DEPLOYED && pendingTarget_ != ShieldState::RETRACTED) return;
  motion_ = pendingTarget_ == ShieldState::DEPLOYED ? MotionState::DEPLOYING : MotionState::RETRACTING;
  motionStartedAt_ = nowMs;
  shield_ = ShieldState::STOPPED_PARTIAL;
  if (motionRequestId_ != 0 && motionRequestId_ == lastRequestId_) commandPhase_ = CommandPhase::STARTED;
}

void Controller::requestMove(ShieldState target, const Inputs& inputs, uint32_t requestId) {
  if (target != ShieldState::DEPLOYED && target != ShieldState::RETRACTED) return;
  updateEstimate(inputs.nowMs);

  if (targetLimitActive(target, inputs)) {
    stopAt(target, PositionConfidence::LIMIT_CONFIRMED);
    if (requestId != 0 && requestId == lastRequestId_) commandPhase_ = CommandPhase::COMPLETED;
    return;
  }
  if (motion_ == MotionState::STOPPED && shield_ == target) {
    if (requestId != 0 && requestId == lastRequestId_) commandPhase_ = CommandPhase::COMPLETED;
    return;
  }
  if ((motion_ == MotionState::DEPLOYING && target == ShieldState::DEPLOYED) ||
      (motion_ == MotionState::RETRACTING && target == ShieldState::RETRACTED)) {
    motionRequestId_ = requestId;
    if (requestId != 0 && requestId == lastRequestId_) commandPhase_ = CommandPhase::STARTED;
    return;
  }
  if (motion_ == MotionState::REVERSAL_DEAD_TIME && pendingTarget_ == target) {
    if (requestId != 0) motionRequestId_ = requestId;
    return;
  }

  pendingTarget_ = target;
  motionRequestId_ = requestId;
  motionStartPositionPct_ = estimateAvailable_ ? estimatedPositionPct_ : (target == ShieldState::DEPLOYED ? 0.0f : 100.0f);
  motionTargetPositionPct_ = target == ShieldState::DEPLOYED ? 100.0f : 0.0f;
  const float distance = fabsf(motionTargetPositionPct_ - motionStartPositionPct_);
  movementDurationMs_ = estimateAvailable_
      ? static_cast<uint32_t>(static_cast<float>(config_.inferredFullTravelMs) * distance / 100.0f)
      : config_.inferredFullTravelMs;
  if (movementDurationMs_ == 0) movementDurationMs_ = 1;
  motion_ = MotionState::REVERSAL_DEAD_TIME;
  deadTimeStartedAt_ = inputs.nowMs;
  shield_ = ShieldState::STOPPED_PARTIAL;
  confidence_ = estimateAvailable_ ? PositionConfidence::ESTIMATED : PositionConfidence::UNKNOWN;
}

void Controller::stopAt(ShieldState target, PositionConfidence confidence) {
  motion_ = MotionState::STOPPED;
  pendingTarget_ = ShieldState::UNKNOWN;
  shield_ = target;
  confidence_ = confidence;
  estimatedPositionPct_ = target == ShieldState::DEPLOYED ? 100.0f : 0.0f;
  estimateAvailable_ = true;
  if (motionRequestId_ != 0 && motionRequestId_ == lastRequestId_) commandPhase_ = CommandPhase::COMPLETED;
  motionRequestId_ = 0;
}

void Controller::stopPartial(uint32_t nowMs) {
  updateEstimate(nowMs);
  if (motion_ != MotionState::STOPPED) shield_ = ShieldState::STOPPED_PARTIAL;
  motion_ = MotionState::STOPPED;
  pendingTarget_ = ShieldState::UNKNOWN;
  motionRequestId_ = 0;
  if (!estimateAvailable_) confidence_ = PositionConfidence::UNKNOWN;
}

void Controller::setFault(Fault fault, uint32_t nowMs) {
  updateEstimate(nowMs);
  if (motion_ != MotionState::STOPPED) shield_ = ShieldState::STOPPED_PARTIAL;
  motion_ = MotionState::STOPPED;
  pendingTarget_ = ShieldState::UNKNOWN;
  fault_ = fault;
  stopLatched_ = true;
  if (motionRequestId_ != 0 && motionRequestId_ == lastRequestId_) commandPhase_ = CommandPhase::FAULT;
  motionRequestId_ = 0;
}

void Controller::updateMotion(const Inputs& inputs) {
  if (fault_ != Fault::NONE) return;
  if (config_.useLimitSwitches && inputs.retractedLimit && inputs.deployedLimit) {
    setFault(Fault::LIMIT_CONFLICT, inputs.nowMs);
    return;
  }
  if (motion_ == MotionState::REVERSAL_DEAD_TIME) {
    if (targetLimitActive(pendingTarget_, inputs)) {
      stopAt(pendingTarget_, PositionConfidence::LIMIT_CONFIRMED);
    } else if (elapsed(inputs.nowMs, deadTimeStartedAt_, config_.directionDeadTimeMs)) {
      startPendingMove(inputs.nowMs);
    }
    return;
  }
  if (motion_ != MotionState::DEPLOYING && motion_ != MotionState::RETRACTING) return;

  updateEstimate(inputs.nowMs);
  const ShieldState target = motion_ == MotionState::DEPLOYING ? ShieldState::DEPLOYED : ShieldState::RETRACTED;
  if (targetLimitActive(target, inputs)) {
    stopAt(target, PositionConfidence::LIMIT_CONFIRMED);
    return;
  }
  const uint32_t run = static_cast<uint32_t>(inputs.nowMs - motionStartedAt_);
  if (run >= config_.motorMaxRuntimeMs) {
    setFault(Fault::MOTOR_TIMEOUT, inputs.nowMs);
  } else if (!config_.useLimitSwitches && run >= movementDurationMs_) {
    stopAt(target, PositionConfidence::ESTIMATED);
  }
}

void Controller::runAutomatic(const Inputs& inputs) {
  if (mode_ != Mode::AUTO || stopLatched_ || fault_ != Fault::NONE) return;
  if (rain_ == RainState::WET) {
    if (shield_ != ShieldState::DEPLOYED || motion_ == MotionState::RETRACTING ||
        (motion_ == MotionState::REVERSAL_DEAD_TIME && pendingTarget_ == ShieldState::RETRACTED)) {
      requestMove(ShieldState::DEPLOYED, inputs, 0);
    }
  } else if (rain_ == RainState::DRY && estimateAvailable_ && shield_ != ShieldState::RETRACTED) {
    requestMove(ShieldState::RETRACTED, inputs, 0);
  }
}

void Controller::tick(const Inputs& inputs) {
  updateRain(inputs);

  const bool retracting = motion_ == MotionState::RETRACTING ||
      (motion_ == MotionState::REVERSAL_DEAD_TIME && pendingTarget_ == ShieldState::RETRACTED);
  if (rain_ == RainState::WET && retracting) {
    if (motionRequestId_ != 0 && motionRequestId_ == lastRequestId_) commandPhase_ = CommandPhase::STOPPED;
    requestMove(ShieldState::DEPLOYED, inputs, 0);
  }

  updateMotion(inputs);
  runAutomatic(inputs);
}

CommandResponse Controller::reject(CommandResult result) {
  lastCommandResult_ = result;
  commandPhase_ = CommandPhase::REJECTED;
  return {result, commandPhase_, false};
}

CommandResponse Controller::complete(CommandResult result) {
  lastCommandResult_ = result;
  commandPhase_ = CommandPhase::COMPLETED;
  return {result, commandPhase_, true};
}

CommandResponse Controller::command(Command commandValue, uint32_t requestId, const Inputs& inputs) {
  if (requestId == 0 || commandValue == Command::INVALID) {
    if (requestId != 0) {
      lastRequestId_ = requestId;
      lastCommand_ = commandValue;
    }
    return reject(CommandResult::INVALID);
  }
  if (requestId == lastRequestId_) return {CommandResult::DUPLICATE, commandPhase_, false};

  lastRequestId_ = requestId;
  lastCommand_ = commandValue;
  lastCommandResult_ = CommandResult::ACCEPTED;
  commandPhase_ = CommandPhase::ACCEPTED;

  if (commandValue == Command::STOP) {
    stopPartial(inputs.nowMs);
    mode_ = Mode::MANUAL;
    stopLatched_ = true;
    commandPhase_ = CommandPhase::STOPPED;
    return {CommandResult::ACCEPTED, commandPhase_, true};
  }
  if (commandValue == Command::RESET_FAULT) {
    if (config_.useLimitSwitches && inputs.retractedLimit && inputs.deployedLimit) return reject(CommandResult::REJECTED_FAULT);
    stopPartial(inputs.nowMs);
    fault_ = Fault::NONE;
    mode_ = Mode::MANUAL;
    stopLatched_ = true;
    return complete();
  }
  if (fault_ != Fault::NONE) return reject(CommandResult::REJECTED_FAULT);

  if (commandValue == Command::SET_AUTO) {
    mode_ = Mode::AUTO;
    stopLatched_ = false;
    complete();
    runAutomatic(inputs);
    return {lastCommandResult_, commandPhase_, true};
  }
  if (commandValue == Command::SET_MANUAL) {
    stopPartial(inputs.nowMs);
    mode_ = Mode::MANUAL;
    stopLatched_ = false;
    return complete();
  }
  if (commandValue == Command::CALIBRATE_DEPLOYED || commandValue == Command::CALIBRATE_RETRACTED) {
    if (motion_ != MotionState::STOPPED || mode_ != Mode::MANUAL) return reject(CommandResult::REJECTED_STATE);
    stopLatched_ = false;
    stopAt(commandValue == Command::CALIBRATE_DEPLOYED ? ShieldState::DEPLOYED : ShieldState::RETRACTED,
           PositionConfidence::USER_CALIBRATED);
    return complete();
  }
  if (mode_ != Mode::MANUAL) return reject(CommandResult::REJECTED_MODE);
  if (commandValue == Command::RETRACT && rain_ != RainState::DRY) return reject(CommandResult::REJECTED_RAIN);
  if (commandValue != Command::DEPLOY && commandValue != Command::RETRACT) return reject(CommandResult::INVALID);

  stopLatched_ = false;
  requestMove(commandValue == Command::DEPLOY ? ShieldState::DEPLOYED : ShieldState::RETRACTED, inputs, requestId);
  return {CommandResult::ACCEPTED, commandPhase_, true};
}

Snapshot Controller::snapshot() const {
  Snapshot result;
  result.mode = mode_;
  result.shield = shield_;
  result.confidence = confidence_;
  result.estimatedPositionPct = estimatedPositionPct_;
  result.estimateAvailable = estimateAvailable_;
  result.stopLatched = stopLatched_;
  result.fault = fault_;
  result.movementActive = motion_ != MotionState::STOPPED;
  result.rainEventCount = rainEventCount_;
  return result;
}

const char* toString(Mode value) { return value == Mode::AUTO ? "AUTO" : "MANUAL"; }
const char* toString(RainState value) {
  if (value == RainState::DRY) return "DRY";
  if (value == RainState::WET) return "WET";
  return "UNKNOWN";
}
const char* toString(ShieldState value) {
  if (value == ShieldState::RETRACTED) return "RETRACTED";
  if (value == ShieldState::DEPLOYED) return "DEPLOYED";
  if (value == ShieldState::STOPPED_PARTIAL) return "STOPPED_PARTIAL";
  return "UNKNOWN";
}
const char* toString(PositionConfidence value) {
  if (value == PositionConfidence::ESTIMATED) return "ESTIMATED";
  if (value == PositionConfidence::USER_CALIBRATED) return "USER_CALIBRATED";
  if (value == PositionConfidence::LIMIT_CONFIRMED) return "LIMIT_CONFIRMED";
  return "UNKNOWN";
}
const char* toString(MotionState value) {
  if (value == MotionState::DEPLOYING) return "DEPLOYING";
  if (value == MotionState::RETRACTING) return "RETRACTING";
  if (value == MotionState::REVERSAL_DEAD_TIME) return "REVERSAL_DEAD_TIME";
  return "STOPPED";
}
const char* toString(Fault value) {
  if (value == Fault::MOTOR_TIMEOUT) return "MOTOR_TIMEOUT";
  if (value == Fault::LIMIT_CONFLICT) return "LIMIT_CONFLICT";
  if (value == Fault::PERSISTENCE_INVALID) return "PERSISTENCE_INVALID";
  if (value == Fault::STORAGE_ERROR) return "STORAGE_ERROR";
  return "NONE";
}
const char* toString(Command value) {
  if (value == Command::SET_AUTO) return "SET_AUTO";
  if (value == Command::SET_MANUAL) return "SET_MANUAL";
  if (value == Command::DEPLOY) return "DEPLOY";
  if (value == Command::RETRACT) return "RETRACT";
  if (value == Command::STOP) return "STOP";
  if (value == Command::RESET_FAULT) return "RESET_FAULT";
  if (value == Command::CALIBRATE_DEPLOYED) return "CALIBRATE_DEPLOYED";
  if (value == Command::CALIBRATE_RETRACTED) return "CALIBRATE_RETRACTED";
  return "INVALID";
}
const char* toString(CommandResult value) {
  if (value == CommandResult::ACCEPTED) return "ACCEPTED";
  if (value == CommandResult::REJECTED_MODE) return "REJECTED_MODE";
  if (value == CommandResult::REJECTED_RAIN) return "REJECTED_RAIN";
  if (value == CommandResult::REJECTED_FAULT) return "REJECTED_FAULT";
  if (value == CommandResult::DUPLICATE) return "DUPLICATE";
  if (value == CommandResult::INVALID) return "INVALID";
  if (value == CommandResult::REJECTED_STATE) return "REJECTED_STATE";
  return "IDLE";
}
const char* toString(CommandPhase value) {
  if (value == CommandPhase::ACCEPTED) return "ACCEPTED";
  if (value == CommandPhase::STARTED) return "STARTED";
  if (value == CommandPhase::COMPLETED) return "COMPLETED";
  if (value == CommandPhase::STOPPED) return "STOPPED";
  if (value == CommandPhase::REJECTED) return "REJECTED";
  if (value == CommandPhase::FAULT) return "FAULT";
  return "NONE";
}

Command commandFromString(const char* value) {
  if (!value) return Command::INVALID;
  if (strcmp(value, "SET_AUTO") == 0) return Command::SET_AUTO;
  if (strcmp(value, "SET_MANUAL") == 0) return Command::SET_MANUAL;
  if (strcmp(value, "DEPLOY") == 0) return Command::DEPLOY;
  if (strcmp(value, "RETRACT") == 0) return Command::RETRACT;
  if (strcmp(value, "STOP") == 0) return Command::STOP;
  if (strcmp(value, "RESET_FAULT") == 0) return Command::RESET_FAULT;
  if (strcmp(value, "CALIBRATE_DEPLOYED") == 0) return Command::CALIBRATE_DEPLOYED;
  if (strcmp(value, "CALIBRATE_RETRACTED") == 0) return Command::CALIBRATE_RETRACTED;
  return Command::INVALID;
}

}  // namespace greenguard
