#include "MachineGunComponent.h"

#include "../Component.h"
#include "../../Entities/EntityManager.h"
#include "../../Components/CameraComponent.h"
#include "../../Components/RigidbodyComponents/PowerUpSpawnerComponent.h"
#include "../../Systems/Content/ContentManager.h"
#include "../../Systems/Physics/RaycastGroups.h"
#include "../../Systems/Audio.h"
#include "../../Systems/StateManager.h"
#include "../../Systems/Physics.h"
#include "../RigidbodyComponents/VehicleComponent.h"
#include "../ParticleEmitterComponent.h"
#include "../LineComponent.h"
#include "../../Systems/Effects.h"
#include "PennerEasing/Linear.h"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.inl>
#include <glm/gtc/type_ptr.hpp>

const float SPRAY = 1.1f;

MachineGunComponent::MachineGunComponent() : WeaponComponent(20.0f) {}

void MachineGunComponent::Shoot(glm::vec3 position) {
	if (StateManager::gameTime.GetSeconds() > nextShotTime.GetSeconds()) {
		turnTurret(position);

		Entity* vehicle = GetEntity();
		//Audio::Instance().PlayAudio3D(Audio::Instance().Weapons.bulletShoot, vehicle->transform.GetGlobalPosition(), glm::vec3(0.f, 0.f, 0.f), 0.11f);
		Audio::Instance().PlayAudio3DAttached(Audio::Instance().Weapons.bulletShoot, vehicle, 0.11f);
		//Audio::Instance().PlayAudio2D("Content/Sounds/machine_gun_shot.mp3");

		Entity* mgTurret = EntityManager::FindFirstChild(vehicle, "GunTurret");
		const glm::vec3 gunPosition = mgTurret->transform.GetGlobalPosition();

        auto emitters = mgTurret->GetComponents<ParticleEmitterComponent>();
        for (auto emitter : emitters) {
            emitter->Emit(3);
        }

		glm::vec3 gunDirection = position - gunPosition;
		float distanceToTarget = glm::length(gunDirection);
		gunDirection = glm::normalize(gunDirection);

		// pick a random point on a sphere for spray
		float randomHorizontalAngle = (float)rand() / (float)RAND_MAX * M_PI * 2.f;
		float randomVerticalAngle = (float)rand() / (float)RAND_MAX * M_PI;
		
		glm::vec3 randomOffset = glm::vec3(cos(randomHorizontalAngle) * sin(randomVerticalAngle),
			cos(randomVerticalAngle),
			sin(randomHorizontalAngle) * sin(randomVerticalAngle)) * SPRAY;

		glm::vec3 shotPosition = glm::normalize(gunDirection)*100.f + randomOffset + gunPosition;

		//Calculate Next Shooting Time
		nextShotTime = StateManager::gameTime + timeBetweenShots;

		//Variables Needed
		const glm::vec3 shotDirection = glm::normalize(shotPosition - gunPosition);

		//Cast Gun Ray
	    PxScene* scene = &Physics::Instance().GetScene();
		const float rayLength = 1000.0f;
		PxRaycastBuffer cameraHit;
		PxQueryFilterData filterData;
		glm::vec3 hitPosition;
		filterData.data.word0 = RaycastGroups::GetGroupsMask(vehicle->GetComponent<VehicleComponent>()->GetRaycastGroup() | RaycastGroups::GetPowerUpGroup());
		PxRaycastBuffer gunHit;
		if (scene->raycast(Transform::ToPx(gunPosition), Transform::ToPx(shotDirection), rayLength, gunHit, PxHitFlag::eDEFAULT, filterData)) {
			hitPosition = Transform::FromPx(gunHit.block.position);
			Entity* thingHit = EntityManager::FindEntity(gunHit.block.actor);
            auto powerupPointer = thingHit->GetComponent<PowerUpSpawnerComponent>();
          
            if (thingHit) {
                thingHit->TakeDamage(this, GetDamage());
            }

            Entity* explosionEffect;
            if (thingHit && thingHit->HasTag("Vehicle")) {
                explosionEffect = ContentManager::LoadEntity("BulletHitCarEffect.json");
            } else {
                explosionEffect = ContentManager::LoadEntity("BulletHitGroundEffect.json");
            }
            explosionEffect->transform.SetPosition(hitPosition);
            explosionEffect->transform.LookInDirection(Transform::FromPx(gunHit.block.normal));
            float duration = 0.f;
            for (ParticleEmitterComponent* emitter : explosionEffect->GetComponents<ParticleEmitterComponent>()) {
                emitter->Emit(2);
                duration = std::max(duration, emitter->GetLifetimeSeconds());
            }
            auto tween = Effects::Instance().CreateTween<float, easing::Linear::easeInOut>(0.f, 1.f, duration, StateManager::gameTime);
            tween->SetFinishedCallback([explosionEffect](float& value) mutable {
                EntityManager::DestroyEntity(explosionEffect);
            });
            tween->Start();
		} else {
			hitPosition = gunPosition + (shotDirection * rayLength);
		}

		PlayerData* player = Game::GetPlayerFromEntity(GetEntity());

		Entity* bullet = ContentManager::LoadEntity("Bullet.json");
		LineComponent* line = bullet->GetComponent<LineComponent>();
		line->SetPoint0(gunPosition);
		line->SetPoint1(hitPosition);
		line->SetColor(glm::vec4(1.0f, .1f, .1f, 1.f));

		auto tween = Effects::Instance().CreateTween<float, easing::Linear::easeNone>(1.f, 0.f, timeBetweenShots*0.5, StateManager::gameTime);
		tween->SetUpdateCallback([line, player, mgTurret, tween](float& value) mutable {
			if (!player->alive) return;
			line->SetPoint0(mgTurret->transform.GetGlobalPosition());
		});
		tween->SetFinishedCallback([bullet](float& value) mutable {
			EntityManager::DestroyEntity(bullet);
		});
		tween->Start();

	} else { // betweeen shots
	}
}

void MachineGunComponent::Charge() {
	return;
}

ComponentType MachineGunComponent::GetType() {
	return ComponentType_MachineGun;
}

void MachineGunComponent::HandleEvent(Event *event) {
	return;
}

void MachineGunComponent::RenderDebugGui() {
    WeaponComponent::RenderDebugGui();
}