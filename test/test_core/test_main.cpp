#include <unity.h>

#include <initializer_list>
#include <stdio.h>

#include "ActuatorOutput.h"
#include "GreenGuardCore.h"

using namespace greenguard;

static unsigned assertionCount = 0;

#define GG_TRUE(value) do { ++assertionCount; TEST_ASSERT_TRUE(value); } while (0)
#define GG_FALSE(value) do { ++assertionCount; TEST_ASSERT_FALSE(value); } while (0)
#define GG_EQ(expected, actual) do { ++assertionCount; TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual)); } while (0)
#define GG_U32(expected, actual) do { ++assertionCount; TEST_ASSERT_EQUAL_UINT32((expected), (actual)); } while (0)
#define GG_FLOAT(expected, actual, tolerance) do { ++assertionCount; TEST_ASSERT_FLOAT_WITHIN((tolerance), (expected), (actual)); } while (0)

void setUp() {}
void tearDown() {}

Config config(bool limits = false) {
  Config value;
  value.wetConfirmMs = 300;
  value.dryConfirmMs = 1000;
  value.directionDeadTimeMs = 100;
  value.inferredFullTravelMs = 1000;
  value.motorMaxRuntimeMs = 1200;
  value.useLimitSwitches = limits;
  return value;
}

Snapshot at(ShieldState shield, float position, Mode mode = Mode::AUTO) {
  Snapshot snapshot;
  snapshot.mode = mode;
  snapshot.shield = shield;
  snapshot.estimatedPositionPct = position;
  snapshot.estimateAvailable = shield != ShieldState::UNKNOWN;
  snapshot.confidence = snapshot.estimateAvailable ? PositionConfidence::ESTIMATED : PositionConfidence::UNKNOWN;
  return snapshot;
}

Inputs input(uint32_t now, bool wet = false, bool retracted = false, bool deployed = false) {
  Inputs value;
  value.nowMs = now;
  value.rainWet = wet;
  value.retractedLimit = retracted;
  value.deployedLimit = deployed;
  return value;
}

void confirmDry(Controller& controller, uint32_t start = 0) { controller.tick(input(start + 1000, false)); }
void confirmWet(Controller& controller, uint32_t start = 0) { controller.tick(input(start + 300, true)); }
void enterManualDry(Controller& controller, uint32_t now = 1000) {
  confirmDry(controller, now - 1000);
  GG_TRUE(controller.command(Command::SET_MANUAL, 1, input(now, false)).accepted);
}

void test_config_validation() {
  Config value = config();
  GG_TRUE(Controller::validConfig(value));
  value.dryConfirmMs = value.wetConfirmMs;
  GG_FALSE(Controller::validConfig(value));
  value = config(); value.motorMaxRuntimeMs = 999;
  GG_FALSE(Controller::validConfig(value));
}

void test_startup_dry_waits_for_confirmation() {
  Controller controller(config());
  controller.begin(Snapshot(), input(0, false));
  controller.tick(input(999, false));
  GG_EQ(RainState::UNKNOWN, controller.rain());
  GG_EQ(MotionState::STOPPED, controller.motion());
  controller.tick(input(1000, false));
  GG_EQ(RainState::DRY, controller.rain());
  GG_EQ(ShieldState::UNKNOWN, controller.shield());
}

void test_startup_wet_deploys_from_unknown() {
  Controller controller(config());
  controller.begin(Snapshot(), input(0, true));
  controller.tick(input(300, true));
  GG_EQ(RainState::WET, controller.rain());
  GG_EQ(MotionState::REVERSAL_DEAD_TIME, controller.motion());
  controller.tick(input(400, true));
  GG_EQ(MotionState::DEPLOYING, controller.motion());
}

void test_auto_loop_does_not_restart_pending_dead_time() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0, true));
  controller.tick(input(300, true));
  for (uint32_t now = 301; now < 400; ++now) controller.tick(input(now, true));
  GG_EQ(MotionState::REVERSAL_DEAD_TIME, controller.motion());
  controller.tick(input(400, true));
  GG_EQ(MotionState::DEPLOYING, controller.motion());
}

void test_startup_known_deployed_dry_retracts() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100), input(0, false));
  controller.tick(input(1000, false));
  GG_EQ(MotionState::REVERSAL_DEAD_TIME, controller.motion());
}

void test_unknown_dry_never_blindly_retracts() {
  Controller controller(config());
  controller.begin(Snapshot(), input(0, false));
  controller.tick(input(5000, false));
  GG_EQ(RainState::DRY, controller.rain());
  GG_EQ(MotionState::STOPPED, controller.motion());
  GG_FALSE(controller.estimateAvailable());
}

void test_corrupt_persistence_faults_safe() {
  Controller controller(config());
  Snapshot corrupt;
  corrupt.mode = static_cast<Mode>(99);
  controller.begin(corrupt, input(0), true);
  GG_EQ(Fault::PERSISTENCE_INVALID, controller.fault());
  GG_EQ(MotionState::STOPPED, controller.motion());
  GG_TRUE(controller.stopLatched());
}

void test_explicit_invalid_persistence_faults_safe() {
  Controller controller(config());
  controller.begin(Snapshot(), input(0), false);
  GG_EQ(Fault::PERSISTENCE_INVALID, controller.fault());
  GG_EQ(Mode::MANUAL, controller.mode());
}

void test_reboot_during_motion_becomes_unknown() {
  Snapshot snapshot = at(ShieldState::STOPPED_PARTIAL, 42);
  snapshot.movementActive = true;
  Controller controller(config());
  controller.begin(snapshot, input(0));
  GG_EQ(ShieldState::UNKNOWN, controller.shield());
  GG_EQ(PositionConfidence::UNKNOWN, controller.confidence());
  GG_FALSE(controller.estimateAvailable());
  GG_TRUE(controller.stopLatched());
}

void test_sustained_rain_completes_inferred_deploy() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0, true));
  controller.tick(input(300, true));
  controller.tick(input(400, true));
  controller.tick(input(1400, true));
  GG_EQ(ShieldState::DEPLOYED, controller.shield());
  GG_EQ(PositionConfidence::ESTIMATED, controller.confidence());
  GG_EQ(MotionState::STOPPED, controller.motion());
  GG_FLOAT(100, controller.estimatedPositionPct(), .01f);
}

void test_short_wet_noise_does_not_move() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0, false));
  controller.tick(input(1000, false));
  controller.tick(input(1100, true));
  controller.tick(input(1399, true));
  controller.tick(input(1400, false));
  GG_EQ(RainState::DRY, controller.rain());
  GG_EQ(MotionState::STOPPED, controller.motion());
}

void test_rapid_wet_dry_noise_does_not_move() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0, false));
  for (uint32_t now = 100; now <= 900; now += 100) controller.tick(input(now, (now / 100) % 2));
  GG_EQ(RainState::UNKNOWN, controller.rain());
  GG_EQ(MotionState::STOPPED, controller.motion());
}

void test_brief_dry_period_is_cancelled_by_rain() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100), input(0, true));
  controller.tick(input(300, true));
  controller.tick(input(400, false));
  controller.tick(input(1399, false));
  controller.tick(input(1400, true));
  GG_EQ(RainState::WET, controller.rain());
  GG_EQ(MotionState::STOPPED, controller.motion());
}

void test_sustained_dry_retracts() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100), input(0, false));
  controller.tick(input(1000, false));
  controller.tick(input(1100, false));
  controller.tick(input(2100, false));
  GG_EQ(ShieldState::RETRACTED, controller.shield());
  GG_EQ(MotionState::STOPPED, controller.motion());
}

void test_manual_command_rejected_in_auto() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0));
  CommandResponse response = controller.command(Command::DEPLOY, 10, input(0));
  GG_FALSE(response.accepted);
  GG_EQ(CommandResult::REJECTED_MODE, response.result);
  GG_EQ(CommandPhase::REJECTED, response.phase);
}

void test_manual_deploy_lifecycle() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0));
  enterManualDry(controller);
  CommandResponse response = controller.command(Command::DEPLOY, 2, input(1000));
  GG_TRUE(response.accepted);
  GG_EQ(CommandPhase::ACCEPTED, response.phase);
  controller.tick(input(1100));
  GG_EQ(CommandPhase::STARTED, controller.commandPhase());
  controller.tick(input(2100));
  GG_EQ(CommandPhase::COMPLETED, controller.commandPhase());
  GG_EQ(ShieldState::DEPLOYED, controller.shield());
}

void test_manual_retract_when_dry() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100), input(0));
  enterManualDry(controller);
  GG_TRUE(controller.command(Command::RETRACT, 2, input(1000)).accepted);
  controller.tick(input(1100));
  GG_EQ(MotionState::RETRACTING, controller.motion());
}

void test_manual_retract_blocked_when_wet() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100, Mode::MANUAL), input(0, true));
  confirmWet(controller);
  CommandResponse response = controller.command(Command::RETRACT, 2, input(300, true));
  GG_FALSE(response.accepted);
  GG_EQ(CommandResult::REJECTED_RAIN, response.result);
}

void test_stop_while_deploying() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  confirmDry(controller);
  controller.command(Command::DEPLOY, 2, input(1000));
  controller.tick(input(1100));
  CommandResponse response = controller.command(Command::STOP, 3, input(1400));
  GG_TRUE(response.accepted);
  GG_EQ(CommandPhase::STOPPED, response.phase);
  GG_EQ(MotionState::STOPPED, controller.motion());
  GG_EQ(ShieldState::STOPPED_PARTIAL, controller.shield());
  GG_TRUE(controller.stopLatched());
}

void test_stop_while_retracting() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100, Mode::MANUAL), input(0));
  confirmDry(controller);
  controller.command(Command::RETRACT, 2, input(1000));
  controller.tick(input(1100));
  controller.command(Command::STOP, 3, input(1500));
  GG_EQ(MotionState::STOPPED, controller.motion());
  GG_FLOAT(60, controller.estimatedPositionPct(), 1.0f);
}

void test_direction_reversal_enforces_dead_time() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  confirmDry(controller);
  controller.command(Command::DEPLOY, 2, input(1000));
  controller.tick(input(1100));
  controller.tick(input(1300));
  controller.command(Command::RETRACT, 3, input(1300));
  GG_EQ(MotionState::REVERSAL_DEAD_TIME, controller.motion());
  GG_EQ(Drive::OFF, controller.drive());
  controller.tick(input(1399));
  GG_EQ(Drive::OFF, controller.drive());
  controller.tick(input(1400));
  GG_EQ(MotionState::RETRACTING, controller.motion());
}

void test_actuator_outputs_are_mutually_exclusive() {
  ActuatorOutput deploy = actuatorOutput(Drive::DEPLOY, false, true, true, 800);
  ActuatorOutput retract = actuatorOutput(Drive::RETRACT, false, true, true, 800);
  GG_U32(800, deploy.rpwm);
  GG_U32(0, deploy.lpwm);
  GG_U32(0, retract.rpwm);
  GG_U32(800, retract.lpwm);
  GG_FALSE(deploy.rpwm && deploy.lpwm);
  GG_FALSE(retract.rpwm && retract.lpwm);
}

void test_dry_run_never_energizes_outputs() {
  for (Drive drive : {Drive::OFF, Drive::DEPLOY, Drive::RETRACT}) {
    ActuatorOutput output = actuatorOutput(drive, true, true, true, 1023);
    GG_U32(0, output.rpwm);
    GG_U32(0, output.lpwm);
    GG_FALSE(output.enableHigh);
  }
}

void test_enable_stays_low_when_not_firmware_controlled() {
  ActuatorOutput output = actuatorOutput(Drive::DEPLOY, false, false, true, 500);
  GG_FALSE(output.enableHigh);
  GG_U32(500, output.rpwm);
}

void test_motor_timeout_with_limit_switches() {
  Config value = config(true);
  Controller controller(value);
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  confirmDry(controller);
  controller.command(Command::DEPLOY, 2, input(1000));
  controller.tick(input(1100));
  controller.tick(input(2300));
  GG_EQ(Fault::MOTOR_TIMEOUT, controller.fault());
  GG_EQ(MotionState::STOPPED, controller.motion());
  GG_EQ(CommandPhase::FAULT, controller.commandPhase());
}

void test_fault_lockout() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  controller.raiseFault(Fault::STORAGE_ERROR, 0);
  CommandResponse response = controller.command(Command::DEPLOY, 2, input(0));
  GG_FALSE(response.accepted);
  GG_EQ(CommandResult::REJECTED_FAULT, response.result);
  GG_EQ(Drive::OFF, controller.drive());
}

void test_fault_reset_stays_stopped_manual() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0));
  controller.raiseFault(Fault::STORAGE_ERROR, 0);
  CommandResponse response = controller.command(Command::RESET_FAULT, 2, input(0));
  GG_TRUE(response.accepted);
  GG_EQ(Fault::NONE, controller.fault());
  GG_EQ(Mode::MANUAL, controller.mode());
  GG_TRUE(controller.stopLatched());
}

void test_auto_to_manual_stops_motion() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0, true));
  confirmWet(controller);
  controller.tick(input(400, true));
  GG_EQ(MotionState::DEPLOYING, controller.motion());
  controller.command(Command::SET_MANUAL, 3, input(500, true));
  GG_EQ(Mode::MANUAL, controller.mode());
  GG_EQ(MotionState::STOPPED, controller.motion());
}

void test_manual_to_auto_runs_dry_policy() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100, Mode::MANUAL), input(0, false));
  confirmDry(controller);
  controller.command(Command::SET_AUTO, 2, input(1000, false));
  GG_EQ(Mode::AUTO, controller.mode());
  GG_EQ(MotionState::REVERSAL_DEAD_TIME, controller.motion());
}

void test_repeated_same_direction_is_idempotent() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  confirmDry(controller);
  controller.command(Command::DEPLOY, 2, input(1000));
  controller.tick(input(1100));
  CommandResponse response = controller.command(Command::DEPLOY, 3, input(1200));
  GG_TRUE(response.accepted);
  GG_EQ(CommandPhase::STARTED, response.phase);
  GG_EQ(MotionState::DEPLOYING, controller.motion());
}

void test_duplicate_request_id_is_not_executed_twice() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  confirmDry(controller);
  controller.command(Command::DEPLOY, 42, input(1000));
  CommandResponse duplicate = controller.command(Command::RETRACT, 42, input(1000));
  GG_FALSE(duplicate.accepted);
  GG_EQ(CommandResult::DUPLICATE, duplicate.result);
  GG_EQ(MotionState::REVERSAL_DEAD_TIME, controller.motion());
}

void test_conflicting_manual_command_in_auto_is_rejected() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100), input(0, false));
  confirmDry(controller);
  CommandResponse response = controller.command(Command::RETRACT, 9, input(1000));
  GG_EQ(CommandResult::REJECTED_MODE, response.result);
}

void test_invalid_and_zero_id_commands_rejected() {
  Controller controller(config());
  controller.begin(Snapshot(), input(0));
  GG_EQ(CommandResult::INVALID, controller.command(Command::INVALID, 1, input(0)).result);
  GG_EQ(CommandResult::INVALID, controller.command(Command::STOP, 0, input(0)).result);
  GG_EQ(Command::INVALID, commandFromString("ESP32_MAGIC"));
}

void test_millis_wraparound_rain_confirmation() {
  Controller controller(config());
  const uint32_t start = 0xfffffff0u;
  controller.begin(Snapshot(), input(start, true));
  controller.tick(input(start + 299u, true));
  GG_EQ(RainState::UNKNOWN, controller.rain());
  controller.tick(input(start + 300u, true));
  GG_EQ(RainState::WET, controller.rain());
}

void test_millis_wraparound_motion_completion() {
  Controller controller(config());
  const uint32_t start = 0xfffff000u;
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(start, false));
  controller.tick(input(start + 1000u, false));
  controller.command(Command::DEPLOY, 2, input(start + 1000u, false));
  controller.tick(input(start + 1100u, false));
  controller.tick(input(start + 2100u, false));
  GG_EQ(ShieldState::DEPLOYED, controller.shield());
}

void test_disabled_limits_are_ignored() {
  Controller controller(config(false));
  controller.begin(at(ShieldState::RETRACTED, 0), input(0, false, true, true));
  GG_EQ(Fault::NONE, controller.fault());
  GG_EQ(ShieldState::RETRACTED, controller.shield());
}

void test_retracted_limit_confirms_position() {
  Controller controller(config(true));
  controller.begin(Snapshot(), input(0, false, true, false));
  GG_EQ(ShieldState::RETRACTED, controller.shield());
  GG_EQ(PositionConfidence::LIMIT_CONFIRMED, controller.confidence());
  GG_FLOAT(0, controller.estimatedPositionPct(), .01f);
}

void test_deployed_limit_confirms_position() {
  Controller controller(config(true));
  controller.begin(Snapshot(), input(0, false, false, true));
  GG_EQ(ShieldState::DEPLOYED, controller.shield());
  GG_EQ(PositionConfidence::LIMIT_CONFIRMED, controller.confidence());
}

void test_both_limits_active_faults() {
  Controller controller(config(true));
  controller.begin(Snapshot(), input(0, false, true, true));
  GG_EQ(Fault::LIMIT_CONFLICT, controller.fault());
  GG_EQ(Drive::OFF, controller.drive());
  GG_TRUE(controller.stopLatched());
}

void test_rain_reverses_manual_retraction() {
  Controller controller(config());
  controller.begin(at(ShieldState::DEPLOYED, 100, Mode::MANUAL), input(0, false));
  confirmDry(controller);
  controller.command(Command::RETRACT, 2, input(1000, false));
  controller.tick(input(1100, false));
  controller.tick(input(1200, true));
  controller.tick(input(1500, true));
  GG_EQ(MotionState::REVERSAL_DEAD_TIME, controller.motion());
  GG_EQ(Drive::OFF, controller.drive());
  controller.tick(input(1600, true));
  GG_EQ(MotionState::DEPLOYING, controller.motion());
}

void test_stop_latch_persists_across_reboot() {
  Controller first(config());
  first.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  first.command(Command::STOP, 2, input(0));
  Controller rebooted(config());
  rebooted.begin(first.snapshot(), input(0, true));
  rebooted.tick(input(500, true));
  GG_TRUE(rebooted.stopLatched());
  GG_EQ(MotionState::STOPPED, rebooted.motion());
}

void test_calibration_requires_manual_and_stopped() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0));
  GG_EQ(CommandResult::REJECTED_STATE, controller.command(Command::CALIBRATE_DEPLOYED, 2, input(0)).result);
  controller.command(Command::SET_MANUAL, 3, input(0));
  GG_TRUE(controller.command(Command::CALIBRATE_DEPLOYED, 4, input(0)).accepted);
  GG_EQ(PositionConfidence::USER_CALIBRATED, controller.confidence());
  GG_EQ(ShieldState::DEPLOYED, controller.shield());
}

void test_stop_produces_partial_estimate() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  confirmDry(controller);
  controller.command(Command::DEPLOY, 2, input(1000));
  controller.tick(input(1100));
  controller.command(Command::STOP, 3, input(1600));
  GG_EQ(ShieldState::STOPPED_PARTIAL, controller.shield());
  GG_EQ(PositionConfidence::ESTIMATED, controller.confidence());
  GG_FLOAT(50, controller.estimatedPositionPct(), 1.0f);
}

void test_snapshot_marks_active_motion() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0, Mode::MANUAL), input(0));
  confirmDry(controller);
  controller.command(Command::DEPLOY, 2, input(1000));
  GG_TRUE(controller.snapshot().movementActive);
  controller.command(Command::STOP, 3, input(1000));
  GG_FALSE(controller.snapshot().movementActive);
}

void test_offline_automation_has_no_network_dependency() {
  Controller controller(config());
  controller.begin(at(ShieldState::RETRACTED, 0), input(0, true));
  controller.tick(input(300, true));
  controller.tick(input(400, true));
  GG_EQ(Drive::DEPLOY, controller.drive());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_config_validation);
  RUN_TEST(test_startup_dry_waits_for_confirmation);
  RUN_TEST(test_startup_wet_deploys_from_unknown);
  RUN_TEST(test_auto_loop_does_not_restart_pending_dead_time);
  RUN_TEST(test_startup_known_deployed_dry_retracts);
  RUN_TEST(test_unknown_dry_never_blindly_retracts);
  RUN_TEST(test_corrupt_persistence_faults_safe);
  RUN_TEST(test_explicit_invalid_persistence_faults_safe);
  RUN_TEST(test_reboot_during_motion_becomes_unknown);
  RUN_TEST(test_sustained_rain_completes_inferred_deploy);
  RUN_TEST(test_short_wet_noise_does_not_move);
  RUN_TEST(test_rapid_wet_dry_noise_does_not_move);
  RUN_TEST(test_brief_dry_period_is_cancelled_by_rain);
  RUN_TEST(test_sustained_dry_retracts);
  RUN_TEST(test_manual_command_rejected_in_auto);
  RUN_TEST(test_manual_deploy_lifecycle);
  RUN_TEST(test_manual_retract_when_dry);
  RUN_TEST(test_manual_retract_blocked_when_wet);
  RUN_TEST(test_stop_while_deploying);
  RUN_TEST(test_stop_while_retracting);
  RUN_TEST(test_direction_reversal_enforces_dead_time);
  RUN_TEST(test_actuator_outputs_are_mutually_exclusive);
  RUN_TEST(test_dry_run_never_energizes_outputs);
  RUN_TEST(test_enable_stays_low_when_not_firmware_controlled);
  RUN_TEST(test_motor_timeout_with_limit_switches);
  RUN_TEST(test_fault_lockout);
  RUN_TEST(test_fault_reset_stays_stopped_manual);
  RUN_TEST(test_auto_to_manual_stops_motion);
  RUN_TEST(test_manual_to_auto_runs_dry_policy);
  RUN_TEST(test_repeated_same_direction_is_idempotent);
  RUN_TEST(test_duplicate_request_id_is_not_executed_twice);
  RUN_TEST(test_conflicting_manual_command_in_auto_is_rejected);
  RUN_TEST(test_invalid_and_zero_id_commands_rejected);
  RUN_TEST(test_millis_wraparound_rain_confirmation);
  RUN_TEST(test_millis_wraparound_motion_completion);
  RUN_TEST(test_disabled_limits_are_ignored);
  RUN_TEST(test_retracted_limit_confirms_position);
  RUN_TEST(test_deployed_limit_confirms_position);
  RUN_TEST(test_both_limits_active_faults);
  RUN_TEST(test_rain_reverses_manual_retraction);
  RUN_TEST(test_stop_latch_persists_across_reboot);
  RUN_TEST(test_calibration_requires_manual_and_stopped);
  RUN_TEST(test_stop_produces_partial_estimate);
  RUN_TEST(test_snapshot_marks_active_motion);
  RUN_TEST(test_offline_automation_has_no_network_dependency);
  const int failures = UNITY_END();
  printf("GreenGuard assertion count: %u\n", assertionCount);
  if (FILE* report = fopen(".pio/native-assertions.txt", "w")) {
    fprintf(report, "%u\n", assertionCount);
    fclose(report);
  }
  return failures;
}
