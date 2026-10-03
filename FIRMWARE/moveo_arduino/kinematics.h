#pragma once

/* ─────────────────────────────────────────────
   Forward / inverse kinematics  (pure math, no Arduino deps)
   Geometry and joint conventions: see moveo_config.h.
   Angles in degrees, lengths in mm.

   Tool approach vector a = (cos p cos y, cos p sin y, sin p)
     pitch p: -90 = pointing straight down
     yaw   y: azimuth of the approach direction
   Roll about a is not controllable (5 DOF).
   ───────────────────────────────────────────── */

#define KIN_JOINTS 5

struct Pose {
  float x, y, z;  // tool point (mm)
  float pitch;    // approach elevation (deg)
  float yaw;      // approach azimuth (deg)
};

enum IkStatus {
  IK_OK,
  IK_UNREACHABLE,  // no solution for this pose
  IK_LIMITS        // solutions exist, but all are outside the joint limits
};

void forwardKinematics(const float deg[KIN_JOINTS], Pose& out);

// planar: ignore target.yaw and approach within the arm's vertical plane (J4 = 0).
// current: present joint angles; the closest valid solution is chosen and
//          J1/J4 are kept where the pose leaves them undetermined.
// lo / hi: joint limits (use -INFINITY / INFINITY for an unlimited joint).
IkStatus inverseKinematics(const Pose& target, bool planar,
                           const float current[KIN_JOINTS],
                           const float lo[KIN_JOINTS], const float hi[KIN_JOINTS],
                           float out[KIN_JOINTS]);
