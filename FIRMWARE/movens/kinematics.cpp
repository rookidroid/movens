#include "kinematics.h"

#include <math.h>

#include "movens_config.h"

static const float PI_F = 3.14159265358979f;
static const float D2R  = PI_F / 180.0f;
static const float R2D  = 180.0f / PI_F;

struct Vec3 { float x, y, z; };

static Vec3  operator+(Vec3 a, Vec3 b)  { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3  operator-(Vec3 a, Vec3 b)  { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3  operator*(float k, Vec3 a) { return {k * a.x, k * a.y, k * a.z}; }
static float dot(Vec3 a, Vec3 b)        { return a.x * b.x + a.y * b.y + a.z * b.z; }

/* Frame of the forearm for base yaw t1 and forearm pitch phi = J2 + J3 (rad):
     er  horizontal "forward" direction of the arm plane
     n   axis of J2 / J3 / J5 (at J4 = 0); + rotation about it bends forward
     f   forearm direction (elbow -> wrist)
     b   n x f: the direction f moves toward under a + rotation */
struct ArmFrame { Vec3 er, n, f, b; };

static ArmFrame armFrame(float t1, float phi) {
  const Vec3 ez = {0, 0, 1};
  ArmFrame F;
  F.er = {cosf(t1), sinf(t1), 0};
  F.n  = {-sinf(t1), cosf(t1), 0};
  F.f  = sinf(phi) * F.er + cosf(phi) * ez;
  F.b  = cosf(phi) * F.er - sinf(phi) * ez;
  return F;
}

void forwardKinematics(const float deg[KIN_JOINTS], Pose& out) {
  const float t1 = deg[0] * D2R, t2 = deg[1] * D2R, t3 = deg[2] * D2R;
  const float t4 = deg[3] * D2R, t5 = deg[4] * D2R;
  const Vec3 ez = {0, 0, 1};

  ArmFrame F = armFrame(t1, t2 + t3);
  Vec3 shoulder = KIN_A1 * F.er + KIN_D1 * ez;
  Vec3 elbow    = shoulder + KIN_A2 * (sinf(t2) * F.er + cosf(t2) * ez);
  Vec3 wrist    = elbow + KIN_D4 * F.f + KIN_A3 * F.b;
  Vec3 t        = cosf(t5) * F.f + sinf(t5) * (cosf(t4) * F.b + sinf(t4) * F.n);
  Vec3 tool     = wrist + KIN_D6 * t;

  out.x     = tool.x;
  out.y     = tool.y;
  out.z     = tool.z;
  out.pitch = atan2f(t.z, sqrtf(t.x * t.x + t.y * t.y)) * R2D;
  out.yaw   = atan2f(t.y, t.x) * R2D;
}

// x + k*360 closest to cur that lies within [lo, hi]; NAN if none does
static float fitAngle(float x, float cur, float lo, float hi) {
  const float tol = 1e-3f;
  float d = remainderf(x - cur, 360.0f);  // [-180, 180]
  float best = NAN;
  for (int k = -1; k <= 1; k++) {
    float v = cur + d + 360.0f * k;
    if (v < lo - tol || v > hi + tol) continue;
    if (isnan(best) || fabsf(v - cur) < fabsf(best - cur)) best = v;
  }
  return best;
}

IkStatus inverseKinematics(const Pose& target, bool planar,
                           const float current[KIN_JOINTS],
                           const float lo[KIN_JOINTS], const float hi[KIN_JOINTS],
                           float out[KIN_JOINTS]) {
  const float eps = 1e-4f;

  // Approach vector; planar keeps it in the vertical plane through the target
  float yaw = target.yaw * D2R;
  if (planar) {
    yaw = (fabsf(target.x) > eps || fabsf(target.y) > eps)
              ? atan2f(target.y, target.x) : current[0] * D2R;
  }
  const float p = target.pitch * D2R;
  const Vec3 a = {cosf(p) * cosf(yaw), cosf(p) * sinf(yaw), sinf(p)};

  // Wrist centre (J5 axis)
  const Vec3 w = Vec3{target.x, target.y, target.z} - KIN_D6 * a;
  const float r = sqrtf(w.x * w.x + w.y * w.y);
  const float t1Base = r > eps ? atan2f(w.y, w.x) : current[0] * D2R;

  const float L     = sqrtf(KIN_D4 * KIN_D4 + KIN_A3 * KIN_A3);
  const float delta = atan2f(KIN_A3, KIN_D4);

  bool  reachable = false;
  float bestCost  = INFINITY;

  for (int back = 0; back < 2; back++) {         // facing the wrist, or reaching over the base
    const float t1 = t1Base + (back ? PI_F : 0.0f);
    const float rho = w.x * cosf(t1) + w.y * sinf(t1) - KIN_A1;
    const float h   = w.z - KIN_D1;

    float c = (rho * rho + h * h - KIN_A2 * KIN_A2 - L * L) / (2.0f * KIN_A2 * L);
    if (fabsf(c) > 1.0f + eps) continue;
    c = fmaxf(-1.0f, fminf(1.0f, c));
    reachable = true;

    for (int elbow = 0; elbow < 2; elbow++) {     // q = J3 + delta, both signs
      const float q  = elbow ? -acosf(c) : acosf(c);
      const float t3 = q - delta;
      const float t2 = atan2f(rho, h) - atan2f(L * sinf(q), KIN_A2 + L * cosf(q));

      // Wrist: a = cos J5 f + sin J5 (cos J4 b + sin J4 n)
      ArmFrame F = armFrame(t1, t2 + t3);
      const float af = dot(a, F.f), ab = dot(a, F.b), an = dot(a, F.n);
      const float s  = sqrtf(ab * ab + an * an);
      const float t4 = s > eps ? atan2f(an, ab) : current[3] * D2R;  // singular: keep J4
      const float t5 = atan2f(s, af);

      for (int flip = 0; flip < 2; flip++) {       // (J4, J5) or (J4 + 180, -J5)
        const float cand[KIN_JOINTS] = {
          t1 * R2D, t2 * R2D, t3 * R2D,
          (t4 + (flip ? PI_F : 0.0f)) * R2D,
          (flip ? -t5 : t5) * R2D,
        };

        float sol[KIN_JOINTS];
        float cost = 0;
        bool  ok   = true;
        for (int j = 0; j < KIN_JOINTS && ok; j++) {
          sol[j] = fitAngle(cand[j], current[j], lo[j], hi[j]);
          ok = !isnan(sol[j]);
          if (ok) cost += (sol[j] - current[j]) * (sol[j] - current[j]);
        }
        if (!ok || cost >= bestCost) continue;

        bestCost = cost;
        for (int j = 0; j < KIN_JOINTS; j++) out[j] = sol[j];
      }
    }
  }

  if (!reachable) return IK_UNREACHABLE;
  return isinf(bestCost) ? IK_LIMITS : IK_OK;
}
