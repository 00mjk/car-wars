#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "PxPhysicsAPI.h"
#include <json/json.hpp>

class GuiComponent;

class Transform {
public:
	Transform();
	Transform(nlohmann::json data);
	Transform(physx::PxTransform transform);
	Transform(glm::vec3 _position, glm::vec3 _scale = glm::vec3(1.f), glm::quat _rotation=glm::quat());
	Transform(Transform *parent, glm::vec3 pPosition, glm::vec3 pScale, glm::vec3 pEulerRotation);
	Transform(Transform *pParent, glm::vec3 pPosition, glm::vec3 pScale, glm::quat pRotation);

	void Update();

	static float radius;

	Transform *parent;

	static const glm::vec3 FORWARD;
	static const glm::vec3 RIGHT;
	static const glm::vec3 UP;

    bool RenderDebugGui(float positionIncrement=0.01f, float scaleIncrement=0.05f);

	// Getters for basic data
	glm::vec3 GetLocalPosition() const;
	glm::vec3 GetLocalScale() const;
	glm::quat GetLocalRotation() const;

	glm::vec3 GetGlobalPosition();
	glm::vec3 GetGlobalScale();

	glm::vec3 GetLocalDirection(glm::vec3 globalDirection);
    glm::vec3 GetGlobalDirection(glm::vec3 localDirection);
	glm::vec3 GetForward();
	glm::vec3 GetRight();
	glm::vec3 GetUp();

    glm::vec3 GetEulerAngles() const;

	// Setters for basic data
	void SetPosition(glm::vec3 pPosition);
	void SetScale(glm::vec3 pScale);
	void SetRotation(glm::quat pRotation);
	void SetRotationEulerAngles(glm::vec3 eulerAngles);
	void SetRotationAxisAngles(glm::vec3 axis, float radians);
    void LookAt(glm::vec3 position);
    void LookInDirection(glm::vec3 direction);

	// Operators for basic data
	void Translate(glm::vec3 offset);
	void Scale(float scaleFactor);
	void Scale(glm::vec3 scaleFactor);
	void Rotate(glm::vec3 axis, float radians);
	void Rotate(glm::quat quaternion);

	// Getters for output data
	glm::mat4 GetTranslationMatrix();
	glm::mat4 GetScalingMatrix();
	glm::mat4 GetRotationMatrix();
	glm::mat4 GetLocalTransformationMatrix();
	glm::mat4 GetTransformationMatrix();
    glm::mat4 GetGuiTransformationMatrix(glm::vec2 anchorPoint, glm::vec2 scaledPosition, glm::vec2 scaledScale, glm::vec2 viewportPosition, glm::vec2 viewportScale, glm::vec2 windowScale);

	static glm::vec4 FromPx(physx::PxVec4 v);
	static glm::vec3 FromPx(physx::PxVec3 v);
	static glm::vec2 FromPx(physx::PxVec2 v);
	static glm::quat FromPx(physx::PxQuat q);
	static Transform FromPx(physx::PxTransform t);

    static physx::PxVec4 ToPx(glm::vec4 v);
    static physx::PxVec3 ToPx(glm::vec3 v);
    static physx::PxVec2 ToPx(glm::vec2 v);
    static physx::PxQuat ToPx(glm::quat q);
    static physx::PxTransform ToPx(Transform t);

    static glm::vec3 Project(glm::vec3 v, glm::vec3 n);
    static glm::vec3 ProjectVectorOnPlane(glm::vec3 v, glm::vec3 n);

private:
	// Basic data
	glm::vec3 position;
	glm::vec3 scale;
	glm::quat rotation;
	
	// Output data for lazy-loading purposes
	glm::mat4 translationMatrix;
	glm::mat4 scalingMatrix;
	glm::mat4 rotationMatrix;
	glm::mat4 transformationMatrix;

	void UpdateTransformationMatrix();
};
