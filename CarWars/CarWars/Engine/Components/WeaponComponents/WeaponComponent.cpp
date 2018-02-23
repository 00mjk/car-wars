#include "WeaponComponent.h"

void WeaponComponent::SetTargetRotation(float _horizontalAngle, float _verticalAngle) {
	targetHorizontalAngle = _horizontalAngle * -1.0f;
	targetVerticalAngle = _verticalAngle * -1.0f;

	horizontalAngle = glm::mix(horizontalAngle, targetHorizontalAngle, 0.05f);
	verticalAngle = glm::mix(verticalAngle, targetVerticalAngle, 0.05f);
}

ComponentType WeaponComponent::GetType() {
	return ComponentType_Weapons;
}

void WeaponComponent::HandleEvent(Event *event) {
	return;
}

void WeaponComponent::RenderDebugGui() {
    Component::RenderDebugGui();
}