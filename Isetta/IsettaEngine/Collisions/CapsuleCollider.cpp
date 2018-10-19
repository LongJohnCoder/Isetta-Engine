/*
 * Copyright (c) 2018 Isetta
 */
#include "Collisions/CapsuleCollider.h"
#include "Collisions/BoxCollider.h"
#include "Collisions/CollisionsModule.h"
#include "Collisions/SphereCollider.h"

#include "Collisions/Ray.h"
#include "Core/Debug/DebugDraw.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Vector4.h"
#include "Scene/Transform.h"

namespace Isetta {
void CapsuleCollider::Update() {
  Math::Matrix4 scale;
  Math::Matrix4 rotation;
  GetWorldCapsule(&rotation, &scale);
  DebugDraw::WireCapsule(
      Math::Matrix4::Translate(GetTransform()->GetWorldPos() + center) * scale *
          rotation,
      radius, height, debugColor);

  Math::Vector3 dir =
      (Math::Vector3)(rotation * scale *
                      Math::Matrix4::Scale(Math::Vector3{height - 2 * radius}) *
                      Math::Vector4{0, 1, 0, 0});
  /* Math::Vector3 P0 = GetTransform()->GetWorldPos() + center - dir;
   Math::Vector3 P1 = GetTransform()->GetWorldPos() + center + dir;*/
  // DebugDraw::Line(P0, P1, Color::blue);
}
bool CapsuleCollider::RaycastSphere(const Math::Vector3& center, float radius,
                                    const Ray& ray, RaycastHit* const hitInfo,
                                    float maxDistance) {
  Math::Vector3 to = ray.GetOrigin() - center;
  float b = Math::Vector3::Dot(to, ray.GetDirection());
  float c = Math::Vector3::Dot(to, to) - radius * radius;
  if (c > 0.0f && b > 0.0f) {
    return false;
  }
  float discrim = b * b - c;
  if (discrim < 0.0f) {
    return false;
  }
  discrim = Math::Util::Sqrt(discrim);
  float t = -b - discrim;
  if (t < 0.f) t = -b + discrim;
  Math::Vector3 pt = ray.GetPoint(t);
  RaycastHitCtor(hitInfo, t, pt, pt - center);
  return true;
}
float CapsuleCollider::GetWorldCapsule(Math::Matrix4* rotation,
                                       Math::Matrix4* scale) const {
  Math::Matrix4& rot = *rotation;
  rot = (Math::Matrix4)GetTransform()->GetWorldRot();
  float max;
  switch (direction) {
    case Direction::X_AXIS:
      rot *= Math::Matrix4::zRot90;
      // rot = rot * Math::Matrix4::zRot90;
      max = Math::Util::Max(GetTransform()->GetWorldScale().y,
                            GetTransform()->GetWorldScale().z);
      *scale = Math::Matrix4::Scale(
          Math::Vector3{GetTransform()->GetWorldScale().x, max, max});
      break;
    case Direction::Y_AXIS:
      max = Math::Util::Max(GetTransform()->GetWorldScale().x,
                            GetTransform()->GetWorldScale().z);
      *scale = Math::Matrix4::Scale(
          Math::Vector3{max, GetTransform()->GetWorldScale().y, max});
      break;
    case Direction::Z_AXIS:
      *rotation *= Math::Matrix4::xRot90;
      max = Math::Util::Max(GetTransform()->GetWorldScale().x,
                            GetTransform()->GetWorldScale().y);
      *scale = Math::Matrix4::Scale(
          Math::Vector3{max, max, GetTransform()->GetWorldScale().z});
      break;
  }
  return max;
}
bool CapsuleCollider::Raycast(const Ray& ray, RaycastHit* const hitInfo,
                              float maxDistance) {
  Math::Matrix4 rot, scale;
  float radiusScale = GetWorldCapsule(&rot, &scale);
  Math::Vector3 dir = (Math::Vector3)(
      rot * scale * Math::Matrix4::Scale(Math::Vector3{height - 2 * radius}) *
      Math::Vector4{0, 1, 0, 0});
  Math::Vector3 p0 = GetWorldCenter() - dir;
  Math::Vector3 p1 = GetWorldCenter() + dir;

  Math::Vector3 to = p1 - p0;
  Math::Vector3 o = ray.GetOrigin() - p0;

  float toDir = Math::Vector3::Dot(to, ray.GetDirection());
  float toO = Math::Vector3::Dot(to, o);
  float toDot = Math::Vector3::Dot(to, to);

  float m = toDir / toDot;
  float n = toO / toDot;

  Math::Vector3 q = ray.GetDirection() - (to * m);
  Math::Vector3 r = o - (to * n);

  float a = Math::Vector3::Dot(q, q);
  float b = 2.0f * Math::Vector3::Dot(q, r);
  float c = Math::Vector3::Dot(r, r) - Math::Util::Square(radius * radiusScale);

  if (a == 0.0f) {
    RaycastHit aHit, bHit;
    if (!RaycastSphere(p0, radius * radiusScale, ray, &aHit, maxDistance) ||
        !RaycastSphere(p0, radius * radiusScale, ray, &aHit, maxDistance))
      return false;
    if (aHit.GetDistance() < bHit.GetDistance())
      *hitInfo = aHit;
    else
      *hitInfo = bHit;
    return true;
  }

  float discrim = b * b - 4.f * a * c;
  if (discrim < 0.f) return false;
  float sqrtDiscrim = Math::Util::Sqrt(discrim);
  float denom = 0.5f / a;
  float tmin = -(b + sqrtDiscrim) * denom;
  float tmax = (-b + sqrtDiscrim) * denom;
  if (tmin > tmax) std::swap(tmin, tmax);

  float tkMin = tmin * m + n;
  // TODO(Jacob) if point is inside capsule && inside sphere poles then it will
  // end on pole
  if (tkMin < 0.f) {
    return RaycastSphere(p0, radius * radiusScale, ray, hitInfo, maxDistance);
  } else if (tkMin > 1.f) {
    return RaycastSphere(p1, radius * radiusScale, ray, hitInfo, maxDistance);
  } else {
    Math::Vector3 pt = ray.GetPoint(tmin);
    RaycastHitCtor(hitInfo, tmin, pt, pt - (p0 + to * tkMin));
    // DebugDraw::Point(pt, Color::cyan, 10, 10);
    // DebugDraw::Point(p0 + to * tkMin, Color::cyan, 10, 10);
    return true;
  }

  float discrim = b * b - 4.f * a * c;
  if (discrim < 0.f) return false;
  float sqrtDiscrim = Math::Util::Sqrt(discrim);
  float denom = 0.5f / a;
  float tmin = -(b + sqrtDiscrim) * denom;
  float tmax = (-b + sqrtDiscrim) * denom;
  if (tmin > tmax) std::swap(tmin, tmax);

  float tkMin = tmin * m + n;
  RaycastHit hitMin;
  if (tkMin < 0.f) {
    // if (!RaycastSphere(p0, radius * radiusScale, ray, &hitMin, maxDistance))
    //  return false;
    return RaycastSphere(p0, radius * radiusScale, ray, hitInfo, maxDistance);
  } else if (tkMin > 1.f) {
    // if (!RaycastSphere(p1, radius * radiusScale, ray, &hitMin, maxDistance))
    //  return false;
    return RaycastSphere(p1, radius * radiusScale, ray, hitInfo, maxDistance);
  } else {
    Math::Vector3 pt = ray.GetPoint(tmin);
    // RaycastHitCtor(&hitMin, tmin, pt, pt - (p0 + to * tkMin));
    RaycastHitCtor(hitInfo, tmin, pt, pt - (p0 + to * tkMin));
    return true;
  }

  // float tkMax = tmax * m + n;
  // RaycastHit hitMax;
  // if (tkMax < 0.f) {
  //  if (!RaycastSphere(p1, radius * radiusScale, ray, &hitMax, maxDistance))
  //    return false;
  //} else if (tkMin > 1.f) {
  //  if (!RaycastSphere(p0, radius * radiusScale, ray, &hitMax, maxDistance))
  //    return false;
  //} else {
  //  Math::Vector3 pt = ray.GetPoint(tmax);
  //  RaycastHitCtor(&hitMin, tmin, pt, pt - (p1 + to * tkMax));
  //}

  return false;
}

INTERSECTION_TEST(CapsuleCollider)
}  // namespace Isetta
