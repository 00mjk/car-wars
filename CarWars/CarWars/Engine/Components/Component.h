#pragma once
#include <string>
#include <foundation/PxTransform.h>

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
	ComponentType_RocketLauncher
};

class Component {
public:
	Component();
	
	bool enabled;
	
    static std::string GetTypeName(ComponentType type);

	virtual ComponentType GetType() = 0;
	virtual void HandleEvent(Event *event) = 0;

    virtual void RenderDebugGui();

    virtual void UpdateFromPhysics(physx::PxTransform t);

	virtual void SetEntity(Entity *_entity);
	Entity* GetEntity() const;
protected:
	Entity *entity;
};
