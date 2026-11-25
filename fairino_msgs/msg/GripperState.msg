# Gripper feedback as the controller reports it, sampled from the SDK realtime state
# package (FRRobot::GetRobotRealTimeState) by the fairino_hardware command server and
# published on "gripper_state_raw" at ~10 Hz, also while a blocking move is running.
# Raw controller units: nothing here is converted. A consumer that needs the opening
# in metres maps position_pct with its own calibration (techmagic_fairino_commander
# does, and republishes it as techmagic_arm_commander_interfaces/GripperState).

builtin_interfaces/Time stamp   # When the driver sampled the state (node clock).
uint8 position_pct              # Current position [% of the gripper's travel, 0-100], as MoveGripper takes it.
int8 speed_pct                  # Current speed [%].
int8 current_pct                # Current motor current [%].
uint16 fault                    # Gripper fault word, 0 = no fault.
uint16 active                   # Gripper activation status word.
uint8 motion_done               # 1 = the last gripper motion has finished.
