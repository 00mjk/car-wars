#pragma once
#include <string>
#include <foundation/PxTransform.h>

class WeaponComponent;
class Event;
class Entity;

enum ComponentType {
	ComponentType_Mesh,
	ComponentType_Camera,
	ComponentType_PointLight,
	ComponentType_DirectionLight,
	ComponentType_SpotLight,
	ComponentType_Rigidbody,
	ComponentType_RigidStatic,
	ComponentType_RigidDynamic,
	ComponentType_Vehicle,
	ComponentType_Weapons,
	ComponentType_MachineGun,
	ComponentType_RailGun,
	ComponentType_RocketLauncher,
	ComponentType_AI,
    ComponentType_PowerUp,
    ComponentType_HealthPowerUp,
    ComponentType_DefencePowerUp,
    ComponentType_DamagePowerUp,
	ComponentType_GUI,
	ComponentType_Missile,
	ComponentType_Line,
	ComponentType_PowerUpSpawner,
	ComponentType_SuicideWeapon,
	ComponentType_Billboard,
	ComponentType_ParticleEmitter
};

class Component {
public:
    virtual ~Component() = default;
    Component();
    Component(const Component& component) = default;
    Component& operator=(const Component& component) = default;
	
	bool enabled;
	
    static std::string GetTypeName(ComponentType type);

	virtual ComponentType GetType() = 0;
	virtual void HandleEvent(Event *event) = 0;

    virtual void RenderDebugGui();

    virtual void UpdateFromPhysics(physx::PxTransform t);

	virtual void TakeDamage(WeaponComponent* damager, float damage);

	virtual void SetEntity(Entity *_entity);
	Entity* GetEntity() const;
protected:
	Entity *entity;
};
