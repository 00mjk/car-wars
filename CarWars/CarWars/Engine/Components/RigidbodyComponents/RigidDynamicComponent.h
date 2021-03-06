#pragma once

#include "RigidbodyComponent.h"
#include <PxRigidDynamic.h>

class RigidDynamicComponent : public RigidbodyComponent {
public:
    RigidDynamicComponent(float _mass=100.f, glm::vec3 _momentOfIntertia=glm::vec3(0.f), glm::vec3 _centerOfMassOffset=glm::vec3(0.f));
    RigidDynamicComponent(nlohmann::json data);
	~RigidDynamicComponent();

    void SetMass(float _mass);
    void SetMomentOfInertia(glm::vec3 _momentOfInertia);
    void SetCenterOfMassOffset(glm::vec3 _centerOfMassOffset);

    ComponentType GetType() override;
    void HandleEvent(Event *event) override;

    void RenderDebugGui() override;

    physx::PxRigidDynamic *actor;

protected:
    virtual void InitializeRigidbody() override;

    float mass;
    glm::vec3 momentOfInertia;
    glm::vec3 centerOfMassOffset;
};
