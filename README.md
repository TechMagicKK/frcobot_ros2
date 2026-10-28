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

**Per-call MoveJ blend time** — `MoveJ` takes an optional 9th argument, `blendT`, after the four external
axes: `MoveJ(CART1,speed,tool,user,e1,e2,e3,e4,blendT)`. `-1` blocks until the move ends, `0`–`500` (ms) queues
it non-blocking. It overrides the `MoveJ_blendT` parameter for that call only; calls without it are unchanged.

**Stop service** — `fairino_stop_motion_service` (`std_srvs/srv/Trigger`) stops the arm even while a blocking
`MoveJ`/`MoveL` is in flight. The command service handles one call at a time, so `StopMotion()` sent through it
only runs after a blocking move has finished. The stop service runs on its own callback group and sends
`StopMotion` to the controller on its own XML-RPC connection. `StopMotion()` on the command service is unchanged.

```bash
ros2 service call /fairino_stop_motion_service std_srvs/srv/Trigger
```

## Gripper and arm motion in parallel

The controller runs commands in the order they are queued. A gripper move overlaps an arm move only when the
**arm move is queued non-blocking first and `MoveGripper` is sent after it with `block = 1`**:

```bash
ros2 service call /fairino_remote_command_service fairino_msgs/srv/RemoteCmdInterface \
  "cmd_str: MoveL(CART1,30,1,1,0)"                    # blend 0 = non-blocking
ros2 service call /fairino_remote_command_service fairino_msgs/srv/RemoteCmdInterface \
  "cmd_str: MoveGripper(1,80,50,1,5000,1)"            # index,pos,vel,force,max_time,block=1
ros2 service call /fairino_remote_command_service fairino_msgs/srv/RemoteCmdInterface \
  "cmd_str: MoveL(CART1,30,1,1,-1)"                   # same target, blocking: returns when arm AND gripper are done
```

Measured on an FR5 (controller V3.8.4.6):

- The gripper runs in parallel only with the segment it is sent right after. A motion queued **after**
  `MoveGripper` waits until the gripper has finished, with `block` 0 or 1.
- `MoveGripper` before a motion, a blocking motion before `MoveGripper`, or `MoveGripper` from a Lua
  `NewAuxThread` all run one after the other.
- With `block = 0` this driver waits for the gripper itself (see above), so the call blocks.

`techmagic_fairino_commander`'s `move_tool_with_gripper` action uses this pattern.

## Installation

Clone this repository and check out the `mrobo2` branch:

```bash
git clone git@github.com:TechMagicKK/frcobot_ros2.git
cd frcobot_ros2
git checkout mrobo2
```

  The `mrobo2` branch is derived from the `tags/V3.0.0_RobotV3.8.4` tag of the [FAIR-INNOVATION repository](https://github.com/FAIR-INNOVATION/frcobot_ros2.git) as shown below:

  ```bash
  git clone https://github.com/FAIR-INNOVATION/frcobot_ros2.git
  cd frcobot_ros2
  git checkout tags/V3.0.0_RobotV3.8.4
  ```

  > **Note:** You do not need to clone `FAIR-INNOVATION/frcobot_ros2.git` — this is shown here for reference only.
