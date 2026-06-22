# frcobot_ros2

This is the ROS 2 API project for the Fairino robot (software version must be greater than V3.7.1). It provides a series of functions based on the Fair SDK API that have been simplified, allowing users to call them through service messages.

## Why this fork

The upstream implementation hardcoded several motion parameters, making runtime tuning impossible without recompilation.

**Gripper control** — `MoveGripper` hardcoded `max_time = 30000` ms and `block = 1`, making it impossible to tune gripper timeout or run in non-blocking mode from the caller side.

This fork refactors `MoveGripper` in [fairino_hardware/src/command_server.cpp](https://github.com/FAIR-INNOVATION/frcobot_ros2/blob/7c683cf63f2c94587a5197c67fd0a80dc886b7ad/fairino_hardware/src/command_server.cpp#L946-L965) to:

- Accept `max_time` and `block` as dynamic parameters passed by the client.
- When `block = 0` (non-blocking), poll `GetGripperMotionDone` up to `max_time` ms with 50 ms intervals so the caller still gets reliable completion detection without blocking the robot controller thread.
- Surface gripper faults via `RCLCPP_WARN` (ROS 2 logger) if `fault != 0` is detected during the poll.

This is required for applications that need fine-grained control over gripper behavior — for example, coordinating gripper motion with arm trajectories where a fixed 30-second blocking wait is unacceptable.

**Motion blending** — The upstream `MoveL` command always used a fixed blend radius read from the Fairino parameter server (`MoveL_blendR`), requiring a separate `SetParameters` service call before each motion to change it.

This fork extends `MoveL` in `fairino_hardware/src/command_server.cpp` to accept an optional inline blend radius as a 5th argument. When provided, it overrides the parameter server value for that call only; when omitted, the parameter server value is used as before (fully backward compatible).

The `MoveTool` action interface (`techmagic_arm_commander_interfaces/action/MoveTool.action`) is extended with a `blend_radius` field (metres), which the `techmagic_fairino_commander` passes inline to `MoveL`. This lets callers vary blend radius per motion goal — useful for distinguishing transit moves (large radius, smooth arc) from precision approach moves (zero or small radius, sharp stop) without touching the parameter server.

## Known limitations

### Gripper cannot move in parallel with arm motion

Issuing `MoveGripper` while `MoveL` is still executing does not run both in parallel. To reproduce, run the following commands in three separate terminals:

```bash
# Terminal 1 — slow the robot down so the effect is clearly visible
ros2 service call /fairino_remote_command_service \
  fairino_msgs/srv/RemoteCmdInterface \
  "cmd_str: SetSpeed(5)"

# Terminal 1 — define a target point far enough to give a long travel time
ros2 service call /fairino_remote_command_service \
  fairino_msgs/srv/RemoteCmdInterface \
  "cmd_str: CARTPoint(1,200,-450,350,180,0,-135)"
```

```bash
# Terminal 2 — start the arm motion (returns immediately at the SDK level)
ros2 service call /fairino_remote_command_service \
  fairino_msgs/srv/RemoteCmdInterface \
  "cmd_str: MoveL(CART1,5,0,0)"
```

```bash
# Terminal 3 — immediately send the gripper command while the arm is still moving;
#              observe that it does not start until the arm stops
ros2 service call /fairino_remote_command_service \
  fairino_msgs/srv/RemoteCmdInterface \
  "cmd_str: MoveGripper(1,10)"
```

  > **Note:** This limitation exists in the original [FAIR-INNOVATION/frcobot_ros2](https://github.com/FAIR-INNOVATION/frcobot_ros2) as well.

## Installation

Clone this repository and check out the `mrobo2` branch:

```bash
git clone git@github.com:TechMagicKK/frcobot_ros2.git
cd frcobot_ros2
git checkout mrobo2
```

  The `mrobo2` branch is derived from the `tags/V3.0.0_RobotV3.8.0` tag of the [FAIR-INNOVATION repository](https://github.com/FAIR-INNOVATION/frcobot_ros2.git) as shown below:

  ```bash
  git clone https://github.com/FAIR-INNOVATION/frcobot_ros2.git
  cd frcobot_ros2
  git checkout tags/V3.0.0_RobotV3.8.0
  ```

  > **Note:** You do not need to clone `FAIR-INNOVATION/frcobot_ros2.git` — this is shown here for reference only.
