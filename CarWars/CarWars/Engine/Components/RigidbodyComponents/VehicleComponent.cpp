#include "VehicleComponent.h"
#include "../../Systems/Content/ContentManager.h"
#include "../../Entities/EntityManager.h"

#include "imgui/imgui.h"
#include "../../Systems/Physics.h"
#include "../Colliders/ConvexMeshCollider.h"
#include "../Colliders/BoxCollider.h"
#include "../../Systems/Physics/VehicleTireFriction.h"

using namespace physx;

VehicleComponent::VehicleComponent() : VehicleComponent(4, false) { }

VehicleComponent::VehicleComponent(nlohmann::json data) : RigidDynamicComponent(data) {
    inputTypeDigital = ContentManager::GetFromJson<bool>(data["DigitalInput"], false);

    if (!data["WheelMesh"].is_null()) {
        wheelMeshPrefab = static_cast<MeshComponent*>(ContentManager::LoadComponent<MeshComponent>(data["WheelMesh"]));
    }
    else {
        wheelMeshPrefab = new MeshComponent("Boulder.obj", "Basic.json", "Boulder.jpg");
    }

    chassisSize = ContentManager::JsonToVec3(data["ChassisSize"], glm::vec3(2.5f, 2.f, 5.f));

    wheelMass = ContentManager::GetFromJson<float>(data["WheelMass"], 20.f);
    wheelRadius = ContentManager::GetFromJson<float>(data["WheelRadius"], 0.5f);
    wheelWidth = ContentManager::GetFromJson<float>(data["WheelWidth"], 0.4f);
    wheelCount = ContentManager::GetFromJson<size_t>(data["WheelCount"], 4);

    // Load any axle data present in data file
    for (nlohmann::json axle : data["Axles"]) {
        axleData.push_back(AxleData(
            ContentManager::GetFromJson<float>(axle["CenterOffset"], 0.f),
            ContentManager::GetFromJson<float>(axle["WheelInset"], 0.f)
        ));
    }

    Initialize();
}

VehicleComponent::VehicleComponent(size_t _wheelCount, bool _inputTypeDigital) : RigidDynamicComponent(),
    inputTypeDigital(_inputTypeDigital), chassisSize(glm::vec3(2.5f, 2.f, 5.f)),
    wheelMass(20.f), wheelRadius(0.5f), wheelWidth(0.4f), wheelCount(_wheelCount) {

    wheelMeshPrefab = new MeshComponent("Boulder.obj", "Basic.json", "Boulder.jpg");

    Initialize();
}

void VehicleComponent::InitializeWheelsSimulationData(const PxVec3* wheelCenterActorOffsets, PxVehicleWheelsSimData* wheelsSimData) {
    //Set up the wheels.
    PxVehicleWheelData wheels[PX_MAX_NB_WHEELS]; {
        //Set up the wheel data structures with mass, moi, radius, width.
        for (PxU32 i = 0; i < wheelCount; i++) {
            wheels[i].mMass = wheelMass;
            wheels[i].mMOI = GetWheelMomentOfIntertia();
            wheels[i].mRadius = wheelRadius;
            wheels[i].mWidth = wheelWidth;
        }

        //Enable the handbrake for the rear wheels only.
        wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxHandBrakeTorque = 4000.0f;
        wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxHandBrakeTorque = 4000.0f;
        //Enable steering for the front wheels only.
        wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxSteer = PxPi*0.3333f;
        wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxSteer = PxPi*0.3333f;
    }

    //Set up the tires.
    PxVehicleTireData tires[PX_MAX_NB_WHEELS];
    {
        //Set up the tires.
        for (PxU32 i = 0; i < wheelCount; i++)
        {
            tires[i].mType = TIRE_TYPE_NORMAL;
        }
    }

    //Set up the suspensions
    PxVehicleSuspensionData suspensions[PX_MAX_NB_WHEELS];
    {
        //Compute the mass supported by each suspension spring.
        PxF32 suspSprungMasses[PX_MAX_NB_WHEELS];
        PxVehicleComputeSprungMasses(wheelCount, wheelCenterActorOffsets, Transform::ToPx(GetChassisCenterOfMassOffset()), mass, 1, suspSprungMasses);

        //Set the suspension data.
        for (PxU32 i = 0; i < wheelCount; i++)
        {
            suspensions[i].mMaxCompression = 0.3f;
            suspensions[i].mMaxDroop = 0.1f;
            suspensions[i].mSpringStrength = 35000.0f;
            suspensions[i].mSpringDamperRate = 4500.0f;
            suspensions[i].mSprungMass = suspSprungMasses[i];
        }

        //Set the camber angles.
        const PxF32 camberAngleAtRest = 0.0;
        const PxF32 camberAngleAtMaxDroop = 0.01f;
        const PxF32 camberAngleAtMaxCompression = -0.01f;
        for (PxU32 i = 0; i < wheelCount; i += 2)
        {
            suspensions[i + 0].mCamberAtRest = camberAngleAtRest;
            suspensions[i + 1].mCamberAtRest = -camberAngleAtRest;
            suspensions[i + 0].mCamberAtMaxDroop = camberAngleAtMaxDroop;
            suspensions[i + 1].mCamberAtMaxDroop = -camberAngleAtMaxDroop;
            suspensions[i + 0].mCamberAtMaxCompression = camberAngleAtMaxCompression;
            suspensions[i + 1].mCamberAtMaxCompression = -camberAngleAtMaxCompression;
        }
    }

    //Set up the wheel geometry.
    PxVec3 suspTravelDirections[PX_MAX_NB_WHEELS];
    PxVec3 wheelCentreCMOffsets[PX_MAX_NB_WHEELS];
    PxVec3 suspForceAppCMOffsets[PX_MAX_NB_WHEELS];
    PxVec3 tireForceAppCMOffsets[PX_MAX_NB_WHEELS];
    {
        //Set the geometry data.
        for (PxU32 i = 0; i < wheelCount; i++)
        {
            //Vertical suspension travel.
            suspTravelDirections[i] = PxVec3(0, -1, 0);

            //Wheel center offset is offset from rigid body center of mass.
            wheelCentreCMOffsets[i] = wheelCenterActorOffsets[i] - Transform::ToPx(GetChassisCenterOfMassOffset());

            //Suspension force application point 0.3 metres below 
            //rigid body center of mass.
            suspForceAppCMOffsets[i] =
                PxVec3(wheelCentreCMOffsets[i].x, -0.3f, wheelCentreCMOffsets[i].z);

            //Tire force application point 0.3 metres below 
            //rigid body center of mass.
            tireForceAppCMOffsets[i] =
                PxVec3(wheelCentreCMOffsets[i].x, -0.3f, wheelCentreCMOffsets[i].z);
        }
    }

    //Set up the filter data of the raycast that will be issued by each suspension.
    PxFilterData qryFilterData;
    setupNonDrivableSurface(qryFilterData);

    //Set the wheel, tire and suspension data.
    //Set the geometry data.
    //Set the query filter data
    for (PxU32 i = 0; i < wheelCount; i++) {
        wheelsSimData->setWheelData(i, wheels[i]);
        wheelsSimData->setTireData(i, tires[i]);
        wheelsSimData->setSuspensionData(i, suspensions[i]);
        wheelsSimData->setSuspTravelDirection(i, suspTravelDirections[i]);
        wheelsSimData->setWheelCentreOffset(i, wheelCentreCMOffsets[i]);
        wheelsSimData->setSuspForceAppPointOffset(i, suspForceAppCMOffsets[i]);
        wheelsSimData->setTireForceAppPointOffset(i, tireForceAppCMOffsets[i]);
        wheelsSimData->setSceneQueryFilterData(i, qryFilterData);
        wheelsSimData->setWheelShapeMapping(i, PxI32(i + colliders.size() - (wheelCount + 1)));
    }

    //Add a front and rear anti-roll bar
    PxVehicleAntiRollBarData barFront;
    barFront.mWheel0 = PxVehicleDrive4WWheelOrder::eFRONT_LEFT;
    barFront.mWheel1 = PxVehicleDrive4WWheelOrder::eFRONT_RIGHT;
    barFront.mStiffness = 10000.0f;
    wheelsSimData->addAntiRollBarData(barFront);
    PxVehicleAntiRollBarData barRear;
    barRear.mWheel0 = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
    barRear.mWheel1 = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;
    barRear.mStiffness = 10000.0f;
    wheelsSimData->addAntiRollBarData(barRear);
}

void VehicleComponent::CreateVehicle() {
    Physics &physics = Physics::Instance();

    PxMaterial *material = ContentManager::GetPxMaterial("Default.json");

    {
        //Wheel and chassis query filter data.
        //Optional: cars don't drive on other cars.
        PxFilterData wheelQryFilterData;
        setupNonDrivableSurface(wheelQryFilterData);
        PxFilterData chassisQryFilterData;
        setupNonDrivableSurface(chassisQryFilterData);

        //Construct a convex mesh for a cylindrical wheel.
        PxConvexMesh* wheelMesh = createWheelMesh(wheelWidth, wheelRadius, physics.GetApi(), physics.GetCooking());
        for (PxU32 i = 0; i < wheelCount; ++i) {
            ConvexMeshCollider *collider = new ConvexMeshCollider("Wheels", material, wheelQryFilterData, wheelMesh);
            AddCollider(collider);
            wheelColliders.push_back(collider);
        }

        AddCollider(new BoxCollider("Chassis", material, chassisQryFilterData, chassisSize));
    }

    //Set up the sim data for the wheels.
    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(wheelCount);
    {
        //Compute the wheel center offsets from the origin.
        PxVec3 wheelCenterActorOffsets[PX_MAX_NB_WHEELS];
        const PxVec3 chassisDims = Transform::ToPx(chassisSize);
        for (PxU32 i = PxVehicleDrive4WWheelOrder::eFRONT_LEFT; i < wheelCount; i += 2) {
            const AxleData axle = axleData[i / 2];
            wheelCenterActorOffsets[i + 0] = PxVec3((-chassisDims.x + wheelWidth) * 0.5f + axle.wheelInset, -(chassisDims.y*0.5f + wheelRadius), axle.centerOffset);
            wheelCenterActorOffsets[i + 1] = PxVec3((chassisDims.x - wheelWidth) * 0.5f - axle.wheelInset, -(chassisDims.y*0.5f + wheelRadius), axle.centerOffset);
        }

        //Set up the simulation data for all wheels.
        InitializeWheelsSimulationData(wheelCenterActorOffsets, wheelsSimData);
    }

    //Set up the sim data for the vehicle drive model.
    PxVehicleDriveSimData4W driveSimData;
    {
        //Diff
        PxVehicleDifferential4WData diff;
        diff.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
        driveSimData.setDiffData(diff);

        //Engine
        PxVehicleEngineData engine;
        engine.mPeakTorque = 500.0f;
        engine.mMaxOmega = 600.0f;//approx 6000 rpm
        driveSimData.setEngineData(engine);

        //Gears
        PxVehicleGearsData gears;
        gears.mSwitchTime = 0.1f;
        driveSimData.setGearsData(gears);

        //Clutch
        PxVehicleClutchData clutch;
        clutch.mStrength = 10.0f;
        driveSimData.setClutchData(clutch);

        //Ackermann steer accuracy
        PxVehicleAckermannGeometryData ackermann;
        ackermann.mAccuracy = 1.0f;
        ackermann.mAxleSeparation =
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).z -
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).z;
        ackermann.mFrontWidth =
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).x -
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x;
        ackermann.mRearWidth =
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).x -
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).x;
        driveSimData.setAckermannGeometryData(ackermann);
    }

    //Create a vehicle from the wheels and drive sim data.
    pxVehicle = PxVehicleDrive4W::allocate(wheelCount);
    pxVehicle->setup(&physics.GetApi(), actor, *wheelsSimData, driveSimData, wheelCount - 4);

    //Free the sim data because we don't need that any more.
    wheelsSimData->free();
}

void VehicleComponent::Initialize() {
    Physics &physics = Physics::Instance();

    SetMomentOfInertia(GetChassisMomentOfInertia());
    SetCenterOfMassOffset(GetChassisCenterOfMassOffset());

    //Create a vehicle that will drive on the plane.
    CreateVehicle();

    //Set the vehicle to rest in first gear.
    //Set the vehicle to use auto-gears.
    pxVehicle->setToRestState();
    pxVehicle->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    pxVehicle->mDriveDynData.setUseAutoGears(true);

    // Fill any remaining any remaining axle data
    const float axleCount = ceil(static_cast<float>(wheelCount) * 0.5f);
    for (size_t i = axleData.size(); i < axleCount; ++i) {
        axleData.push_back(AxleData(glm::mix(0.5f*chassisSize.z, -0.5f*chassisSize.z, static_cast<float>(i) / axleCount)));
    }

    // Create the meshes for each of the wheels
    for (size_t i = 0; i < wheelCount; ++i) {
        MeshComponent* wheel = new MeshComponent(wheelMeshPrefab);
        wheelMeshes.push_back(wheel);
    }
}

void VehicleComponent::UpdateWheelTransforms() {
    for (size_t i = 0; i < wheelCount; ++i) {
        MeshComponent* wheel = wheelMeshes[i];
        Transform pose = wheelColliders[i]->GetLocalTransform();
        wheel->transform.SetPosition(pose.GetLocalPosition());
        wheel->transform.SetRotationAxisAngles(Transform::UP, glm::radians(i % 2 == 0 ? 180.f : 0.f));
        wheel->transform.Rotate(pose.GetLocalRotation());
    }
}

float VehicleComponent::GetChassisMass() const {
    return mass;
}

glm::vec3 VehicleComponent::GetChassisSize() const {
    return chassisSize;
}

glm::vec3 VehicleComponent::GetChassisMomentOfInertia() const {
    return glm::vec3((chassisSize.y*chassisSize.y + chassisSize.z*chassisSize.z)*mass / 12.0f,
        (chassisSize.x*chassisSize.x + chassisSize.z*chassisSize.z)*0.8f*mass / 12.0f,
        (chassisSize.x*chassisSize.x + chassisSize.y*chassisSize.y)*mass / 12.0f);
}

glm::vec3 VehicleComponent::GetChassisCenterOfMassOffset() const {
    return glm::vec3(0.0f, -1.0f, 0.25f);
}

float VehicleComponent::GetWheelMass() const {
    return wheelMass;
}

float VehicleComponent::GetWheelRadius() const {
    return wheelRadius;
}

float VehicleComponent::GetWheelWidth() const {
    return wheelWidth;
}

float VehicleComponent::GetWheelMomentOfIntertia() const {
    return 0.5f*wheelMass*wheelRadius*wheelRadius;
}

size_t VehicleComponent::GetWheelCount() const {
    return wheelCount;
}

std::vector<AxleData> VehicleComponent::GetAxleData() const {
    return axleData;
}

void VehicleComponent::RenderDebugGui() {
    RigidDynamicComponent::RenderDebugGui();
}

ComponentType VehicleComponent::GetType() {
    return ComponentType_Vehicle;
}

void VehicleComponent::HandleEvent(Event *event) {}

void VehicleComponent::SetEntity(Entity* _entity) {
    RigidbodyComponent::SetEntity(_entity);
    for (MeshComponent *component : wheelMeshes) {
        /*if (component->GetEntity() != nullptr) {
        EntityManager::RemoveComponent(component->GetEntity(), component);
        }*/
        EntityManager::AddComponent(GetEntity(), component);
    }
}

void VehicleComponent::UpdateFromPhysics(physx::PxTransform t) {
    Component::UpdateFromPhysics(t);
    UpdateWheelTransforms();
}
