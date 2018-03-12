#pragma once

#include "../Component.h"

class MissileComponent : public Component {
public:
	MissileComponent();

	void Initialize(Entity* _entity, glm::vec3 positon);

	float GetSpeed();
	float GetDamage();
	Entity* GetOwner();

	ComponentType GetType() override;
	void HandleEvent(Event *event) override;

private:
	float missileSpeed = 200.0f;
	float explosionRadius = 2.0f;
	float damage;
	Entity* owner;
};