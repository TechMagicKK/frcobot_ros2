# High-rate (~100 Hz) motion-completion signals, sampled directly from the SDK
# realtime state package (FRRobot::GetRobotRealTimeState) by the fairino_hardware
# command server. Provided for consumers that need finer timing than the ~10 Hz
# /nonrt_state_data stream, e.g. detecting when a queued/blended move has finished.

uint8 motion_done       # 1 = commanded motion complete. Pulses briefly to 1 at
                        #     blended-segment boundaries while the arm is still moving,
                        #     so it cannot be trusted alone.
int32 mc_queue_len      # Controller motion-queue length. Only rises above 0 when commands
                        #     back up (long trajectories); stays 0 on short moves, so it is
                        #     not a reliable "still moving" indicator on its own.
float32 joint_speed_max # Largest absolute joint speed across all 6 joints (deg/s), from
                        #     actual_qd. The physical "is the arm moving" signal: > 0 for
                        #     ANY joint motion (including pure wrist reorientation and while
                        #     rounding a blended corner) and ~0 only at true rest.
