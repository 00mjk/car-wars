#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include "imgui/imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include "../Systems/Content/ContentManager.h"
#include "EntityManager.h"

const glm::vec3 Transform::FORWARD = glm::vec3(0, 0, -1);
const glm::vec3 Transform::RIGHT = glm::vec3(1, 0, 0);
const glm::vec3 Transform::UP = glm::vec3(0, 1, 0);

float Transform::radius = 0;

Transform::Transform() : Transform(nullptr, glm::vec3(), glm::vec3(1.f), glm::quat()) {}

Transform::Transform(nlohmann::json data) : parent(nullptr) {
	SetPosition(ContentManager::JsonToVec3(data["Position"], glm::vec3()));
    SetScale(ContentManager::JsonToVec3(data["Scale"], glm::vec3(1.f)));
    if (!data["Rotate"].is_null()) {
        const glm::vec3 rot = ContentManager::JsonToVec3(data["Rotate"]);
        SetRotationEulerAngles(glm::vec3(glm::radians(rot.x), glm::radians(rot.y), glm::radians(rot.z)));
    }
}

Transform::Transform(physx::PxTransform t) : Transform(nullptr, FromPx(t.p), glm::vec3(1.f), FromPx(t.q)) {}

Transform::Transform(Transform *pParent, glm::vec3 pPosition, glm::vec3 pScale, glm::vec3 pEulerRotation) : parent(pParent) {
	SetPosition(pPosition);
	SetScale(pScale);
	SetRotationEulerAngles(pEulerRotation);
}

Transform::Transform(Transform *pParent, glm::vec3 pPosition, glm::vec3 pScale, glm::quat pRotation) : parent(pParent) {
	SetPosition(pPosition);
	SetScale(pScale);
	SetRotation(pRotation);
}

void Transform::Update() {
	SetPosition(position);
	SetScale(scale);
	SetRotation(rotation);
}

bool Transform::RenderDebugGui() {
    bool changed = false;
    if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.01f)) {
        changed = true;
        SetPosition(position);
    }
    if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.05f)) {
        changed = true;
        SetScale(scale);
    }
    return changed;
}

glm::vec3 Transform::GetLocalPosition() const {
	return position;
}

glm::vec3 Transform::GetCylinderPosition() {
	return FromCylinder(GetGlobalPosition());
}

glm::vec3 Transform::GetLocalScale() const {
	return scale;
}

glm::quat Transform::GetLocalRotation() const {
	return rotation;
}

glm::vec3 Transform::GetGlobalPosition() {
	return GetTransformationMatrix() * glm::vec4(0.f, 0.f, 0.f, 1.f);
}

glm::vec3 Transform::GetGlobalScale() {
	glm::vec3 globalScale = glm::vec3(1.f);
	Transform* transform = this;
	do {
		globalScale = transform->GetLocalScale() * globalScale;
		transform = transform->parent;
	} while (transform != nullptr);
	return globalScale;
}

glm::vec3 Transform::GetLocalDirection(glm::vec3 globalDirection) {
	return glm::inverse(GetTransformationMatrix()) * glm::vec4(globalDirection, 0.f);
}

glm::vec3 Transform::GetGlobalDirection(glm::vec3 localDirection) {
    return GetTransformationMatrix() * glm::vec4(localDirection, 0.f);
}

glm::vec3 Transform::GetForward() {
    return GetGlobalDirection(FORWARD);
}

glm::vec3 Transform::GetRight() {
	return GetGlobalDirection(RIGHT);
}

glm::vec3 Transform::GetUp() {
	return GetGlobalDirection(UP);
}

void Transform::UpdateTransformationMatrix() {
	transformationMatrix = translationMatrix * rotationMatrix * scalingMatrix;
}

void Transform::SetPosition(glm::vec3 pPosition) {
	position = pPosition;
	translationMatrix = glm::translate(glm::mat4(), position);
	UpdateTransformationMatrix();
}

void Transform::SetScale(glm::vec3 pScale) {
	scale = pScale;
	scalingMatrix = glm::scale(scale);
	UpdateTransformationMatrix();
}

void Transform::SetRotation(glm::quat pRotation) {
	rotation = pRotation;
	rotationMatrix = glm::toMat4(rotation);
	UpdateTransformationMatrix();
}

void Transform::SetRotationEulerAngles(glm::vec3 eulerAngles) {
	SetRotation(glm::quat(eulerAngles));
}

void Transform::SetRotationAxisAngles(glm::vec3 axis, float radians) {
	SetRotation(glm::angleAxis(radians, axis));
}

void Transform::Translate(glm::vec3 offset) {
	SetPosition(position + offset);
}

void Transform::Scale(float scaleFactor) {
	SetScale(scale * scaleFactor);
}

void Transform::Scale(glm::vec3 scaleFactor) {
	SetScale(scale * scaleFactor);
}

void Transform::Rotate(glm::vec3 axis, float radians) {
	SetRotation(glm::rotate(rotation, radians, axis));
}

void Transform::Rotate(glm::quat quaternion) {
	SetRotation(quaternion * rotation);
}

glm::mat4 Transform::GetTranslationMatrix() {
	return translationMatrix;
}

glm::mat4 Transform::GetScalingMatrix() {
	return scalingMatrix;
}

glm::mat4 Transform::GetRotationMatrix() {
	return rotationMatrix;
}

glm::mat4 Transform::GetLocalTransformationMatrix() {
	return transformationMatrix;
}

glm::mat4 Transform::GetTransformationMatrix() {
	glm::mat4 matrix(1.f);
	Transform* transform = this;
	do {
		matrix = transform->GetLocalTransformationMatrix() * matrix;
		transform = transform->parent;
	} while (transform != nullptr);
	return matrix;
}

// returns the world location of a point in the cylinder co-ordinates
// since the x coordinate will wrap transforming to the cylinder and then back may result in diffrent locations
glm::vec3 Transform::ToCylinder(glm::vec3 point) {
	float theta = point.x / radius;
	float r = radius - point.y;
	
	point.x = r * cos(theta);
	point.y = r * sin(theta);

	return point;
}

// returns the location on rectangular part of the unwrapped cylinder 
// x coordinates: [-radius*pi,radius*pi]
// y coordinates: [...,radius]
// z coordinates: [...,...]
glm::vec3 Transform::FromCylinder(glm::vec3 point) {
	float r = sqrt(point.x*point.x + point.y*point.y);
	float theta = atan2(point.y, point.x);

	point.x = theta * radius - M_PI * radius;
	point.y = radius - r;

	return point;
}

glm::vec4 Transform::FromPx(physx::PxVec4 v) {
	return glm::vec4(v.x, v.y, v.z, v.w);
}

glm::vec3 Transform::FromPx(physx::PxVec3 v) {
	return glm::vec3(v.x, v.y, v.z);
}

glm::vec2 Transform::FromPx(physx::PxVec2 v) {
	return glm::vec2(v.x, v.y);
}

glm::quat Transform::FromPx(physx::PxQuat q) {
	return glm::quat(q.w, q.x, q.y, q.z);
}

Transform Transform::FromPx(physx::PxTransform t) {
    return Transform(t);
}

physx::PxVec4 Transform::ToPx(glm::vec4 v) {
    return physx::PxVec4(v.x, v.y, v.z, v.w);
}

physx::PxVec3 Transform::ToPx(glm::vec3 v) {
    return physx::PxVec3(v.x, v.y, v.z);
}

physx::PxVec2 Transform::ToPx(glm::vec2 v) {
    return physx::PxVec2(v.x, v.y);
}

physx::PxQuat Transform::ToPx(glm::quat q) {
    return physx::PxQuat(q.x, q.y, q.z, q.w);
}

physx::PxTransform Transform::ToPx(Transform t) {
    return physx::PxTransform(ToPx(t.GetGlobalPosition()), ToPx(t.GetLocalRotation()));
}
