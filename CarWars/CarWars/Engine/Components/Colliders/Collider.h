#pragma once

#include <string>
#include <PxShape.h>
#include <PxActor.h>
#include <PxMaterial.h>
#include <PxFiltering.h>
#include <json/json.hpp>
#include "../../Entities/Transform.h"
#include "../../Systems/Content/Mesh.h"

enum ColliderType {
    Collider_Box,
    Collider_ConvexMesh,
    Collider_TriangleMesh
};

class Collider {
public:
    Collider(std::string _collisionGroup, physx::PxMaterial *_material, physx::PxFilterData _queryFilterData);
    Collider(nlohmann::json data);
    ~Collider();

    physx::PxShape* GetShape() const;
    void CreateShape(physx::PxRigidActor *actor);

    void RenderDebugGui();
    virtual Mesh* GetRenderMesh() = 0;

    static std::string GetTypeName(ColliderType type);

    virtual ColliderType GetType() const = 0;

    virtual Transform GetLocalTransform() const;
    virtual Transform GetGlobalTransform() const;
protected:
    Transform transform;
    std::string collisionGroup;
    physx::PxShape *shape;
    physx::PxGeometry *geometry;
    physx::PxMaterial *material;
    physx::PxFilterData queryFilterData;
};