#include "Audio.h"
#include <iostream>

// Singleton
Audio::Audio() { }

Audio &Audio::Instance() {
    static Audio instance;
    return instance;
}

Audio::~Audio() { 
    sound->release();
    sound3d->release();
    music->release();
    for (auto s : carSounds) { s.sound->release(); }
    for (auto s : soundArray3D) { s->release(); }
    soundSystem->close();
    soundSystem->release();
}

void Audio::ReleaseSounds() {
    sound->release();
    sound3d->release();
    for (auto s : carSounds) { s.sound->release(); }
    for (auto s : soundArray3D) { s->release(); }
}

void Audio::Initialize() { 
    updateFunctionId = 0;
	updatePosition = 0;
    srand(time(NULL));
    currentMusicIndex = rand() % NUM_MUSIC;
    FMOD::System_Create(&soundSystem);
    soundSystem->init(MAX_CHANNELS, FMOD_INIT_NORMAL, 0);
    soundSystem->set3DSettings(1.0f, 1.f, .18f); 
    soundSystem->set3DNumListeners(Game::gameData.humanCount);

	for (int i = 0; i < 100; i++) { availableSound3D[i] = true; }

    prevGameState = StateManager::GetState();
    // main screen intro music
    PlayMusic("Content/Music/imperial-march.mp3");

	// weapons sounds
    AddSoundToMemory("Content/Sounds/rocket-launch.mp3", &Weapons.missleLaunch); 
    AddSoundToMemory("Content/Sounds/explosion.mp3", &Weapons.explosion);

    AddSoundToMemory("Content/Sounds/Weapons/blaster-shoot.mp3", &Weapons.bulletShoot);
    AddSoundToMemory("Content/Sounds/Weapons/blaster-hit1.mp3", &Weapons.bulletHitHeavy);
    AddSoundToMemory("Content/Sounds/Weapons/blaster-hit2.mp3", &Weapons.bulletHitMedium);
    AddSoundToMemory("Content/Sounds/Weapons/blaster-hit3.mp3", &Weapons.bulletHitLight);
    AddSoundToMemory("Content/Sounds/Weapons/blaster-miss.mp3", &Weapons.bulletHitGround);
    AddSoundToMemory("Content/Sounds/Weapons/blaster-miss.mp3", &Weapons.bulletHitWall);

    AddSoundToMemory("Content/Sounds/railgun-shoot.mp3", &Weapons.railgunShoot);
    AddSoundToMemory("Content/Sounds/railgun-charge.mp3", &Weapons.railgunCharge);
    AddSoundToMemory("Content/Sounds/Weapons/railgun-hit1.mp3", &Weapons.railgunHitHeavy);
    AddSoundToMemory("Content/Sounds/railgun-hit.mp3", &Weapons.railgunHitMedium);
    AddSoundToMemory("Content/Sounds/railgun-hit.mp3", &Weapons.railgunHitLight);
    AddSoundToMemory("Content/Sounds/railgun-hit.mp3", &Weapons.railgunHitGround);
    AddSoundToMemory("Content/Sounds/railgun-hit.mp3", &Weapons.railgunHitWall);

	// menu sounds
	AddSoundToMemory("Content/Sounds/Menu/back2.mp3", &Menu.back);
	AddSoundToMemory("Content/Sounds/Menu/navigate.mp3", &Menu.navigate);
	AddSoundToMemory("Content/Sounds/Menu/enter2.mp3", &Menu.enter);

	// environmental sounds
	AddSoundToMemory("Content/Sounds/car-on-car-collision.mp3", &Environment.hitCar);
	AddSoundToMemory("Content/Sounds/car-on-ground-collision.mp3", &Environment.hitGround);
	AddSoundToMemory("Content/Sounds/car-on-ground-collision.mp3", &Environment.hitWall);
    AddSoundToMemory("Content/Sounds/Environment/powerup.mp3", &Environment.powerup);
    AddSoundToMemory("Content/Sounds/Environment/jump.mp3", &Environment.jump);

}

void Audio::AddSoundToMemory(const char *filepath, FMOD::Sound **sound) {
    result = soundSystem->createSound(filepath, FMOD_3D | FMOD_LOOP_OFF, 0, sound);
    if (result != FMOD_OK) {
        std::cout << "Error creating sound " << filepath << std::endl;
    }
    result = (*sound)->set3DMinMaxDistance(MIN_DISTANCE, MAX_DISTANCE);
    if (result != FMOD_OK) {
        std::cout << "Error setting distance on " << filepath << std::endl;
    }
}

int Audio::PlaySound3D(FMOD::Sound *sound, glm::vec3 position, glm::vec3 velocity, float volume) {
	FMOD_VECTOR pos = { position.x, position.y, position.z };
	FMOD_VECTOR vel = { velocity.x, velocity.y, velocity.z };
	int index = 0;
	for (auto s : availableSound3D) {
		if (s) break;
		index++;
	}

	availableSound3D[index] = false;
	soundSystem->playSound(sound, 0, false, &channelArray3D[index]);
	channelArray3D[index]->setVolume(volume);
	channelArray3D[index]->set3DAttributes(&pos, &vel);
	channelArray3D[index]->setPaused(false);

	return index;
}

void Audio::StopSound3D(int index) {
	availableSound3D[index] = true;
    channelArray3D[index]->setPaused(true);
}

void Audio::PlayAudio2D(FMOD::Sound* sound, float volume) {
	soundSystem->playSound(sound, 0, false, &channel);
	channel->setVolume(volume);
}

void Audio::PlayAudio3D(FMOD::Sound *s, glm::vec3 position, glm::vec3 velocity, float volume) {
    FMOD_VECTOR pos = { position.x, position.y, position.z };
    FMOD_VECTOR vel = { velocity.x, velocity.y, velocity.z };
    soundSystem->playSound(s, 0, true, &channel3d);
    channel3d->set3DAttributes(&pos, &vel);
    channel3d->setPaused(false);
    channel3d->setVolume(volume);
}

void Audio::PlayAudio3DAttached(FMOD::Sound *s, Entity* entity, float volume) {
	FMOD_VECTOR pos = { entity->transform.GetGlobalPosition().x, entity->transform.GetGlobalPosition().y, entity->transform.GetGlobalPosition().z };
	FMOD_VECTOR vel = { 0.f, 0.f, 0.f};

	AttachedSound a;
	a.entity = entity;
	soundSystem->playSound(s, 0, true, &a.channel);
	a.channel->set3DAttributes(&pos, &vel);
	a.channel->setPaused(false);
	a.channel->setVolume(volume);
	attachedSounds.push_back(a);
}

void Audio::PauseSounds() {
    for (auto c : channelArray3D) c->setPaused(true);
    for (auto car : carSounds) car.channel->setPaused(true);
    channel->setPaused(true);
    channel3d->setPaused(true);
}
void Audio::ResumeSounds() {
    for (auto c : channelArray3D) c->setPaused(false);
    for (auto car : carSounds) car.channel->setPaused(false);
    channel->setPaused(false);
    channel3d->setPaused(false);
}

void Audio::PlayMusic(const char *filename) {
    result = soundSystem->createStream(filename, FMOD_LOOP_OFF | FMOD_2D, 0, &music);
    result = soundSystem->playSound(music, 0, false, &musicChannel);
    musicChannel->setVolume(musicVolume);
}

void Audio::MenuMusicControl() {
    auto currGameState = StateManager::GetState();
    if (currGameState != prevGameState) {
        if (currGameState == GameState_Playing) {
			ResumeSounds();
            if (!gameStarted) {
                music->release();
                PlayMusic(musicPlaylist[currentMusicIndex]);
                gameStarted = true;
            }
        } else if (currGameState == GameState_Paused) {
			PauseSounds();
        } else if (currGameState == GameState_Menu) {
            srand(time(NULL));
            currentMusicIndex = rand() % NUM_MUSIC;
            ReleaseSounds();
            if (gameStarted) {
                music->release();
                PlayMusic("Content/Music/imperial-march.mp3");
                gameStarted = false;
            }
        }
        prevGameState = currGameState;

    }
}

void Audio::UpdateListeners() {
    // update listener position for every camera/player vehicle
    if (StateManager::GetState() == GameState_Playing) {
        for (size_t i = 0; i < Game::gameData.humanCount; ++i) {
            auto player = Game::humanPlayers[i];
            if (!player.ready || !player.alive) continue;
            const auto carForward = player.vehicleEntity->transform.GetForward();
            const auto carUp = player.vehicleEntity->transform.GetUp();
            const auto carPosition = player.vehicleEntity->transform.GetGlobalPosition();
            const auto carVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
            player.vehicleEntity->GetComponent<VehicleComponent>()->pxVehicle->computeForwardSpeed(); 
            FMOD_VECTOR forward = { carForward.x, carForward.y, carForward.z };
            FMOD_VECTOR up = { carUp.x, carUp.y, carUp.z };
            FMOD_VECTOR position = { carPosition.x, carPosition.y, carPosition.z };
            FMOD_VECTOR velocity = { carVelocity.x, carVelocity.y, carVelocity.z };
            soundSystem->set3DListenerAttributes(i, &position, &velocity, &forward, &up);
        }
    }
}

void Audio::UpdateAttached() {
	vector<int> toDelete;
	for (int i = updatePosition; i < attachedSounds.size() && availableUpdates > 0; ++i) {
		bool isPlaying;
		attachedSounds[i].channel->isPlaying(&isPlaying);
		if (!isPlaying || attachedSounds[i].entity == nullptr || &(attachedSounds[i].entity->transform) == nullptr) {
			//delete
			toDelete.push_back(i);
		} else {
			// update position
            if (!attachedSounds[i].entity->IsMarkedForDeletion() && &attachedSounds[i].entity != nullptr && &attachedSounds[i].entity->transform != nullptr) {
                FMOD_VECTOR pos = {
                    attachedSounds[i].entity->transform.GetGlobalPosition().x,
                    attachedSounds[i].entity->transform.GetGlobalPosition().y,
                    attachedSounds[i].entity->transform.GetGlobalPosition().z
                };
                FMOD_VECTOR vel = { 0.f, 0.f, 0.f };
                attachedSounds[i].channel->set3DAttributes(&pos, &vel);
            } else {
                toDelete.push_back(i);
            }
		}
        availableUpdates--;
	}

    // reset position
    // give way to 2nd function to update
    updatePosition = 0;
    updateFunctionId = 1;


    // does not count against updates...
	for (int i = toDelete.size() - 1; i >= 0 && availableUpdates > 0; --i) {
		attachedSounds.erase(attachedSounds.begin() + i);
	}
}

void Audio::StartCars() {
    const char *engineSound = "Content/Sounds/Truck/idle.mp3";

    // channels and sounds for cars
    carSounds.resize(
        Game::gameData.aiCount +
        Game::gameData.humanCount);

    //players
    for (int i = 0; i < 4; i++) {
        HumanData& player = Game::humanPlayers[i];
        if (!player.ready || !player.alive) continue;
        const auto playerPos = player.vehicleEntity->transform.GetGlobalPosition();
        soundSystem->createSound(engineSound, FMOD_3D | FMOD_LOOP_NORMAL, 0, &carSounds[i].sound);
        carSounds[i].sound->set3DMinMaxDistance(MIN_DISTANCE, MAX_DISTANCE);
        soundSystem->playSound(carSounds[i].sound, 0, true, &carSounds[i].channel);
        FMOD_VECTOR pos = { playerPos.x+1.0f, playerPos.y+1.0f, playerPos.z+1.0f };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
        carSounds[i].channel->set3DAttributes(&pos, &vel);
        carSounds[i].channel->setPaused(false);
        carSounds[i].channel->setVolume(playerSoundVolume);
    }
    size_t offset = Game::gameData.humanCount;
    //ai
    for (int i = 0; i < Game::gameData.aiCount; i++) {
		AiData& player = Game::aiPlayers[i];
		if (!player.alive) continue;
        auto aiPos = player.vehicleEntity->transform.GetGlobalPosition();
        soundSystem->createSound(engineSound, FMOD_3D | FMOD_LOOP_NORMAL, 0, &carSounds[i + offset].sound);
        carSounds[i + offset].sound->set3DMinMaxDistance(MIN_DISTANCE, MAX_DISTANCE);
        soundSystem->playSound(carSounds[i + offset].sound, 0, true, &carSounds[i + offset].channel);
        FMOD_VECTOR pos = { aiPos.x, aiPos.y, aiPos.z };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
        carSounds[i + offset].channel->set3DAttributes(&pos, &vel);
        carSounds[i + offset].channel->setPaused(false);
        carSounds[i + offset].channel->setVolume(aiSoundVolume);
    }
    carsStarted = true;
}

void Audio::PauseCars(bool paused) {
    for (auto carSound : carSounds) { carSound.channel->setPaused(paused); }
}

void Audio::UpdateCars() {
    const char *engineIdle = "Content/Sounds/Truck/idle.mp3";
    const char *engineAccelerate = "Content/Sounds/Truck/accelerate.mp3";
    const char *engineReverse = "Content/Sounds/Truck/reverse.mp3";

    //player
    for (int i = updatePosition; i < 4 && availableUpdates > 0; i++) {
        HumanData& player = Game::humanPlayers[i];
        if (!player.ready || !player.alive) continue;
        const auto playerPos = player.vehicleEntity->transform.GetGlobalPosition();
        VehicleComponent* vehicle = player.vehicleEntity->GetComponent<VehicleComponent>();
        if (vehicle->pxVehicle->mDriveDynData.getCurrentGear() == PxVehicleGearsData::eREVERSE) {
            // set reverse sound
            if (!carSounds[i].reversing) {
                carSounds[i].sound->release();
                carSounds[i].reversing = true;
                carSounds[i].changedDirection = true;
                soundSystem->createSound(engineReverse, FMOD_3D | FMOD_LOOP_NORMAL, 0, &carSounds[i].sound);
                carSounds[i].sound->set3DMinMaxDistance(MIN_DISTANCE, MAX_DISTANCE);
                soundSystem->playSound(carSounds[i].sound, 0, true, &carSounds[i].channel);
                carSounds[i].channel->setPaused(false);

            }
        } else {
            // set forward sound
            if (carSounds[i].changedDirection) {
                carSounds[i].sound->release();
                carSounds[i].changedDirection = false;
                carSounds[i].reversing = false;
                soundSystem->createSound(engineIdle, FMOD_3D | FMOD_LOOP_NORMAL, 0, &carSounds[i].sound);
                carSounds[i].sound->set3DMinMaxDistance(MIN_DISTANCE, MAX_DISTANCE);
                soundSystem->playSound(carSounds[i].sound, 0, true, &carSounds[i].channel);
                carSounds[i].channel->setPaused(false);

            }
        }
        FMOD_VECTOR pos = { playerPos.x, playerPos.y+1.0f, playerPos.z };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
        carSounds[i].channel->set3DAttributes(&pos, &vel);
        //carSounds[i].channel->setPaused(true);
        carSounds[i].channel->setVolume(playerSoundVolume);
        availableUpdates--;
    }
    size_t offset = Game::gameData.humanCount;
    //ai
    for (int i = updatePosition - 4; i < Game::gameData.aiCount && availableUpdates > 0; i++) {
        AiData& ai = Game::aiPlayers[i];
        if (!ai.alive) continue;
        const auto aiPos = ai.vehicleEntity->transform.GetGlobalPosition();
        FMOD_VECTOR pos = { aiPos.x, aiPos.y, aiPos.z };
        FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
        carSounds[i + offset].channel->set3DAttributes(&pos, &vel);
        carSounds[i + offset].channel->setVolume(aiSoundVolume);
        availableUpdates--;
    }
}

void Audio::StopCars() {
    for (auto carSound : carSounds) { carSound.sound->release(); }
    carsStarted = false;
}


void Audio::UpdateRunningCars() {
    auto currGameState = StateManager::GetState();
    if (currGameState != prevGameState) {
        if (carsStarted && currGameState == GameState_Playing) {
            PauseCars(false);
        } else if (!carsStarted && currGameState == GameState_Playing) {
            StartCars();
        } else if (carsStarted && currGameState == GameState_Paused) {
            PauseCars(true);
        } else if (carsStarted && currGameState == GameState_Menu) {
            StopCars();
        }
    }

    if (carsStarted && currGameState == GameState_Playing) {
        UpdateCars();
    }
    // give way to first function
    updateFunctionId = 0;
    updatePosition = 0;
}

void Audio::CheckMusic() {
	bool isPlaying;
	musicChannel->isPlaying(&isPlaying);

    if (!isPlaying && (StateManager::GetState() == GameState_Playing || StateManager::GetState() == GameState_Paused)) {
		//play next song
        music->release();
        srand(time(NULL));
        currentMusicIndex = rand() % NUM_MUSIC;
        PlayMusic(musicPlaylist[currentMusicIndex]);
    } else if (!isPlaying && StateManager::GetState() == GameState_Menu) {
        music->release();
        PlayMusic("Content/Music/imperial-march.mp3");
    }
}

int LimitedUpdate(int updatePosition, int updatesAvailable) {

	return updatePosition;
}

void Audio::Update() { 
    availableUpdates = UPDATES_TO_RUN;

	UpdateListeners(); // 4 updates

    while (availableUpdates > 0 && StateManager::GetState() == GameState_Playing) {
        if (updateFunctionId == 0) UpdateAttached();
        if (updateFunctionId == 1) UpdateRunningCars(); // 
        break; // break out of the loops when there are fewer updtes to be done
    }
	
    MenuMusicControl(); // prevGameState saved - 1 update
    CheckMusic(); // 1 update

    soundSystem->update();
}
