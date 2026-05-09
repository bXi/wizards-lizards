#include "game.h"
#include "lobbyconfig.h"

#include <state/state.h>
#include <cmath>
#include <map>
#include <set>
#include "components/debugrigidbody2d.h"
#include "log.h"
#include "extra/LightLayer.h"

void GameState::setupControls()
{
	LobbyConfig& cfg = LobbyConfig::get();
	int totalPlayers = cfg.activeCount();

	const auto playerFilter = ECS::getWorld().filter<PlayerIndex>();
	playerFilter.each([&](flecs::entity entity, PlayerIndex index) {
		auto* input = entity.get_mut<PlayerInput>();
		input->controllers.clear();

		int slot = index.index - 1; // 0-based

		if (totalPlayers <= 1) {
			// Single player: all devices control player 1
			for (auto* dev : Input::GetAllInputs())
				input->addController(dev);
		} else {
			// Multi-player: each player gets their assigned controller
			int ci = (slot < 4 && cfg.players[slot].active) ? cfg.players[slot].controllerIndex : -1;
			if (ci >= 0 && ci < (int)Input::GetAllInputs().size())
				input->addController(Input::GetController(ci));
		}
	});
}

void GameState::resetGame()
{
	ECS::reset();

    puzzleSolvedRooms.clear();
    mirrorPuzzleSolvedRooms.clear();
    puzzleInitialState.clear();
    puzzleResetTimers.clear();
    roomBeamSegments.clear();

	dungeon = World::getDungeon();


	spawners.clear();
	for (unsigned int i = 0; i < World::getSpawners().size(); i++) {
		spawner = new Spawner();
		spawner->pos = World::getSpawners()[i];

		spawner->load();

		spawners.emplace_back(spawner);
		spawner = nullptr;
	}

	rooms = World::getRooms();

    for (auto& room: rooms) {
        if (room->specialRoom) {

            vf2d testpos = (room->pos);// + room->size);// * vf2d(totalRoomSize, totalRoomSize);

            if (room->northExits > 0) {
                CreateGateEntity(testpos + vf2d(18.5f, 0.5f), room->id);
                CreateGateEntity(testpos + vf2d(19.5f, 0.5f), room->id);
                CreateGateEntity(testpos + vf2d(20.5f, 0.5f), room->id);
                CreateGateEntity(testpos + vf2d(21.5f, 0.5f), room->id);
            }

            if (room->southExits > 0) {
                CreateGateEntity(testpos + vf2d(18.5f, 39.5f), room->id);
                CreateGateEntity(testpos + vf2d(19.5f, 39.5f), room->id);
                CreateGateEntity(testpos + vf2d(20.5f, 39.5f), room->id);
                CreateGateEntity(testpos + vf2d(21.5f, 39.5f), room->id);
            }

            if (room->westExits > 0) {
                CreateGateEntity(testpos + vf2d(0.5f, 18.5f), room->id);
                CreateGateEntity(testpos + vf2d(0.5f, 19.5f), room->id);
                CreateGateEntity(testpos + vf2d(0.5f, 20.5f), room->id);
                CreateGateEntity(testpos + vf2d(0.5f, 21.5f), room->id);
            }

            if (room->eastExits > 0) {
                CreateGateEntity(testpos + vf2d(39.5f, 18.5f), room->id);
                CreateGateEntity(testpos + vf2d(39.5f, 19.5f), room->id);
                CreateGateEntity(testpos + vf2d(39.5f, 20.5f), room->id);
                CreateGateEntity(testpos + vf2d(39.5f, 21.5f), room->id);
            }

            CreateRoomSensorEntity(testpos, room->id);

            // Randomly assign color-block puzzle or mirror puzzle
            bool isMirrorRoom = (Helpers::GetRandomValue(0, 1) == 0);
            if (isMirrorRoom) {
                SpawnMirrorPuzzleForRoom(room);
            } else {
                std::vector<std::pair<vf2d, int>> initial;
                SpawnPuzzleForRoom(room, &initial);
                puzzleInitialState[room->id] = initial;

                LobbyConfig& cfg = LobbyConfig::get();
                int numPlates = 0;
                for (int pi = 0; pi < 4; ++pi) if (cfg.players[pi].active) ++numPlates;
                CreatePressurePlateEntities(room, std::max(1, numPlates));
            }
        }
    }

    // Refresh after room setup — mirror puzzle pillars may have modified dungeon tiles
    tileData = World::getDungeon().getDungeonTileData();

	LobbyConfig& cfg = LobbyConfig::get();
	bool anyActive = false;
	for (int i = 0; i < 4; ++i) if (cfg.players[i].active) { anyActive = true; break; }

	if (anyActive) {
		for (int i = 0; i < 4; ++i) {
			if (cfg.players[i].active)
				CreatePlayerEntity(i + 1, World::getStart(), cfg.players[i].classType);
		}
	} else {
		// Fallback for launching without going through lobby (debug)
		CreatePlayerEntity(1, World::getStart());
	}

    // Assign hue shift based on same-class occurrence order.
    // First player of a class gets no shift; each duplicate gets the next shift index.
    // Collect all (index, classtype, entity) sorted by player index first.
    struct PlayerInfo { int index; PlayerClassType classtype; flecs::entity entity; };
    std::vector<PlayerInfo> playerInfos;
    const auto playerEffectFilter = ECS::getWorld().filter<PlayerIndex, PlayerClass>();
    playerEffectFilter.each([&](flecs::entity entity, PlayerIndex idx, PlayerClass& pc) {
        playerInfos.push_back({ idx.index, pc.classtype, entity });
    });
    std::sort(playerInfos.begin(), playerInfos.end(), [](const PlayerInfo& a, const PlayerInfo& b) {
        return a.index < b.index;
    });

    std::map<PlayerClassType, int> classCount;
    for (auto& info : playerInfos) {
        int shiftIndex = classCount[info.classtype]++;
        if (shiftIndex > 0) {
            info.entity.set<SpriteEffect>({ playerHueEffects[std::clamp(shiftIndex, 0, 3)] });
        }
    }

	setupControls();

	vf2d camPos = { 0.0f, 0.0f };

	const auto playerFilter = ECS::getWorld().filter<PlayerIndex>();
	playerFilter.each([&](flecs::entity entity, PlayerIndex index){
		b2BodyId bodyId = entity.get<RigidBody2D>()->RigidBody;
		//b2Body_SetTransform(bodyId, {(float)World::getStart().x, (float)World::getStart().y}, b2Body_GetRotation(bodyId));
		b2Vec2 pos = b2Body_GetPosition(bodyId);
		camPos += vf2d{pos.x, pos.y};
	});

	Camera::SetTarget({ (camPos.x / static_cast<float>(playerFilter.count())) * static_cast<float>(Configuration::tileWidth), (camPos.y / static_cast<float>(playerFilter.count())) * static_cast<float>(Configuration::tileHeight) });

	accumulator += Window::GetFrameTime();
	while (accumulator >= physTime)
	{
		accumulator -= physTime;
		World::doStep(physTime);
	}
}

void GameState::load()
{
    physTime = 1.f / static_cast<float>(Settings::getMonitorRefreshRate());

    dungeonTileset = AssetHandler::GetTexture("assets/tilesets/dungeon.png");

    LightLayer::Init(1024);
    LightLayer::SetExposure(0.8f);
    Particles::Init();


    AssetHandler::GetFont("assets/fonts/APL386.ttf", 20);

    AssetHandler::GetMusic("assets/music/boss-1-loop.wav");
    AssetHandler::GetSound("assets/sfx/slowmo-enter.wav");
    AssetHandler::GetSound("assets/sfx/slowmo-exit.wav");

	screenWidth = static_cast<float>(Window::GetWidth());
	screenHeight = static_cast<float>(Window::GetHeight());
	screenRatio = screenWidth / screenHeight;

    // Hue shift effects — P1 no shift, P2 +120°, P3 +240°, P4 +180°
    huePassthroughVert = AssetHandler::GetShader("assets/shaders/effect_passthrough.vert");
    hueShiftFrag       = AssetHandler::GetShader("assets/shaders/effect_hueshift.frag");
    constexpr float hueShifts[4] = { 0.0f, 0.333f, 0.667f, 0.5f };
    for (int i = 0; i < 4; ++i) {
        playerHueEffects[i] = Effects::Create(huePassthroughVert, hueShiftFrag);
        playerHueEffects[i]["hueShift"] = hueShifts[i];
    }

	World::generateNewMap();

	resetGame();

	roomTitleLerp = Lerp::getLerp("roomTitleLerp", 0.0f, 10.0f, 4.0f);

    Renderer::CreateSpriteRenderTarget("minimap", {.clearOnLoad = true, .clearColor = GRAY, .width = (uint32_t)miniMapSize, .height = (uint32_t)miniMapSize});

}

void GameState::unload()
{}

void GameState::draw()
{

//    Window::ClearBackground(World::getBackgroundColor());

	auto findRoomContainingPoint = [&](const vf2d& target) -> Room*
	{
		for (const auto& room : rooms)
		{
			// Check if the target point falls within the bounds of the room
			if (target.x >= room->pos.x && target.x <= room->pos.x + room->size.x &&
				target.y >= room->pos.y && target.y <= room->pos.y + room->size.y)
			{
				return room; // Return the room pointer
			}
		}

		return nullptr; // Return nullptr if no room contains the point
	};

	vf2d camPos = { 0.0f, 0.0f };

    const auto playerFilter = ECS::getWorld().filter<PlayerIndex>();
	playerFilter.each([&](flecs::entity entity, PlayerIndex index) {
		auto* rigidBody2d = entity.get<RigidBody2D>();
		auto* input = entity.get_mut<PlayerInput>();
		auto* playerClass = entity.get_mut<PlayerClass>();

		input->doMovement(&entity);
		input->doShoot(entity);

		// Upgrade hold logic
		if (entity.has<PlayerXP>()) {
			auto* xp = entity.get_mut<PlayerXP>();
			int weaponIdx = input->selectedWeapon - 1;
			bool holding = false;
			for (const auto& controller : input->controllers) {
				if (controller->is(Buttons::ACCEPT, Action::HELD)) { holding = true; break; }
			}

			if (holding && xp->canAfford(input->weaponUpgrades[weaponIdx])) {
				input->upgradeLock = true;
				xp->isHolding = true;
				xp->holdTimer += Window::GetFrameTime() / Configuration::slowMotionFactor;

				if (xp->holdTimer >= PlayerXP::HOLD_DURATION) {
					xp->spend(input->weaponUpgrades[weaponIdx]);
					input->weaponUpgrades[weaponIdx]++;
					xp->holdTimer = 0.f;
					xp->isHolding = false;
					input->upgradeLock = false;
				}
			} else {
				xp->holdTimer = 0.f;
				xp->isHolding = false;
				input->upgradeLock = false;
			}
		}

		if (Input::KeyPressed(SDLK_F2)) input->weaponUpgrades[input->selectedWeapon - 1]++;
		if (Input::KeyPressed(SDLK_F3)) input->weaponUpgrades[input->selectedWeapon - 1] += 10;
		if (Input::KeyPressed(SDLK_F4)) input->weaponUpgrades[input->selectedWeapon - 1] += 100;


		playerClass->update();

		auto newSpeed = speed;
		if (input->isRunning) newSpeed *= 1.2f;

		newSpeed /= Configuration::slowMotionFactor;

		if (isPanning) {
			b2Body_SetLinearVelocity(rigidBody2d->RigidBody, {0.f, 0.f});
		} else {
			b2Body_SetLinearVelocity(rigidBody2d->RigidBody, {input->vel.x * newSpeed, input->vel.y * newSpeed});
		}

		b2Vec2 rbPos = b2Body_GetPosition(rigidBody2d->RigidBody);
		camPos += vf2d{rbPos.x, rbPos.y};
	});


    vf2d averagePlayerPos = { (camPos.x / static_cast<float>(playerFilter.count())), // * static_cast<float>(Configuration::tileWidth),
                              (camPos.y / static_cast<float>(playerFilter.count())) // * static_cast<float>(Configuration::tileHeight)
                       };


	if (!isPanning) {
		Room* detectedRoom = findRoomContainingPoint(averagePlayerPos);
		if (detectedRoom && detectedRoom != currentRoom) {
			const float tileW = static_cast<float>(Configuration::tileWidth);
			const float tileH = static_cast<float>(Configuration::tileHeight);

			panFrom = Camera::GetTarget();

			// Pan to where the camera would normally follow in the new room —
			// player's current pixel position clamped to the new room's bounds.
			// This preserves the axis perpendicular to the transition instead of
			// always sliding to the room center.
			float minX = static_cast<float>(detectedRoom->pos.x) * tileW + 20.f * tileW;
			float maxX = static_cast<float>(detectedRoom->pos.x + detectedRoom->size.x) * tileW - 20.f * tileW;
			float minY = static_cast<float>(detectedRoom->pos.y) * tileH + 11.f * tileH;
			float maxY = static_cast<float>(detectedRoom->pos.y + detectedRoom->size.y) * tileH - 11.f * tileH;
			vf2d playerPixel = { averagePlayerPos.x * tileW, averagePlayerPos.y * tileH };
			panTo = { std::clamp(playerPixel.x, minX, maxX), std::clamp(playerPixel.y, minY, maxY) };

			isPanning = true;
			panTimer = 0.f;

			roomTitleLerp->time = 0.0f;
			if (!detectedRoom->visited) {
				detectedRoom->visited = true;
				for (auto& s : detectedRoom->spawners)
					spawners.push_back(s);
			}
			lastRoom = currentRoom;
			currentRoom = detectedRoom;
		}
	}

	if (World::getSlowMotionTimer() <= 0.0f) {
		Configuration::slowMotionFactor = 1.0f;
	}

	if (World::getSlowMotionTimer() <= 1.1f)
	{
		if (World::getPlaySlowMotionExit()) {
			//Audio::PlaySound("assets/sfx/slowmo-exit.wav");
			World::setPlaySlowMotionExit(false);
		}
	}

	handleInput();
	update();

    if (isPanning) {
        panTimer += Window::GetFrameTime();
        float t  = std::min(panTimer / PAN_DURATION, 1.f);
        float et = t * t * (3.f - 2.f * t); // smoothstep ease
        Camera::SetTarget({
            panFrom.x + (panTo.x - panFrom.x) * et,
            panFrom.y + (panTo.y - panFrom.y) * et
        });
        if (panTimer >= PAN_DURATION) {
            Camera::SetTarget(panTo);
            isPanning = false;
        }
    } else {
        vf2d camTarget = {
            (camPos.x / static_cast<float>(playerFilter.count())) * static_cast<float>(Configuration::tileWidth),
            (camPos.y / static_cast<float>(playerFilter.count())) * static_cast<float>(Configuration::tileHeight)
        };
        float minimumCamX = static_cast<float>(currentRoom->pos.x * Configuration::tileWidth  + 20 * Configuration::tileWidth);
        float maximumCamX = static_cast<float>((currentRoom->pos.x + currentRoom->size.x) * Configuration::tileWidth  - 20 * Configuration::tileWidth);
        float minimumCamY = static_cast<float>(currentRoom->pos.y * Configuration::tileHeight + 11 * Configuration::tileHeight);
        float maximumCamY = static_cast<float>((currentRoom->pos.y + currentRoom->size.y) * Configuration::tileHeight - 11 * Configuration::tileHeight);
        camTarget.x = std::clamp(camTarget.x, minimumCamX, maximumCamX);
        camTarget.y = std::clamp(camTarget.y, minimumCamY, maximumCamY);
        Camera::SetTarget(camTarget);
    }

	const auto aiFilter = ECS::getWorld().filter<AIController>();
	aiFilter.each([&](flecs::entity entity, AIController aicontroller) {
		aicontroller.seek();
	});

    if (Input::KeyPressed(SDLK_C)) {
        std::vector<flecs::entity> gateOpenEntities;
        std::vector<flecs::entity> gateCloseEntities;

        const auto gates = ECS::getWorld().filter<GateEntity>();
        gates.each([&](flecs::entity entity, GateEntity gate) {
            if (gate.roomId == currentRoom->id) {
                if (gate.closed) {
                    gateOpenEntities.push_back(entity);
                } else {
                    gateCloseEntities.push_back(entity);
                }
            }
        });

        for (auto entity : gateOpenEntities) entity.get_mut<GateEntity>()->open(entity);
        for (auto entity : gateCloseEntities) entity.get_mut<GateEntity>()->close(entity);
    }



	/*if
X
		for (auto& gameEntity : entities) {
			gameEntity->update(dungeon);
			if (gameEntity->canDelete && State::randomChance(std::max(97.0f + ((0.0f - static_cast<float>(Configuration::gameTime)) / 20.f), 90.0f))) {
				powerup = new Powerup();
				powerup->pos.x = gameEntity->pos.x;
				powerup->pos.y = gameEntity->pos.y;
				powerup->load();
				powerups.emplace_back(powerup);
				powerup = nullptr;
			}
		}

	}*/

	accumulator += (float)Window::GetFrameTime();
	while (accumulator >= physTime)
	{
		accumulator -= physTime;
		World::doStep(physTime);
	}

    // Puzzle block fade-out and spawn-in animation (PuzzleBlock-specific)
    {
        const float dt = Window::GetFrameTime();
        std::vector<flecs::entity> toDelete;

        ECS::getWorld().filter<PuzzleBlock, RigidBody2D>().each([&](flecs::entity e, PuzzleBlock& pb, RigidBody2D& rb) {
            if (!b2Body_IsValid(rb.RigidBody)) return;

            if (pb.fadingOut) {
                pb.fadeTimer += dt;
                if (pb.fadeTimer >= PuzzleBlock::FADE_DURATION)
                    toDelete.push_back(e);
                return;
            }

            if (pb.spawningIn) {
                pb.spawnFadeTimer += dt;
                if (pb.spawnFadeTimer >= PuzzleBlock::FADE_IN_DURATION) {
                    pb.spawningIn    = false;
                    e.get_mut<Pushable>()->settled = true;
                    pb.spawnFadeTimer = PuzzleBlock::FADE_IN_DURATION;
                }
                // Hard-lock position while spawning — not yet interactable
                b2Vec2 pos = b2Body_GetPosition(rb.RigidBody);
                float snapX = roundf(pos.x - 0.5f) + 0.5f;
                float snapY = roundf(pos.y - 0.5f) + 0.5f;
                b2Body_SetTransform(rb.RigidBody, { snapX, snapY }, b2Body_GetRotation(rb.RigidBody));
                b2Body_SetLinearVelocity(rb.RigidBody, { 0.f, 0.f });
                return;
            }
        });

        for (auto& e : toDelete) ECS::removeEntity(&e);
    }

    // Pressure plate hold-to-reset logic
    {
        const float dt = Window::GetFrameTime();

        // Tally plates per room
        std::map<int, std::pair<int,int>> plateStats; // roomId → (total, pressed)
        ECS::getWorld().filter<PressurePlate>().each([&](flecs::entity, PressurePlate& pp) {
            auto& s = plateStats[pp.roomId];
            s.first++;
            if (pp.isPressed) s.second++;
        });

        for (auto& [roomId, stats] : plateStats) {
            if (puzzleSolvedRooms.count(roomId)) continue;
            bool allPressed = (stats.first > 0 && stats.first == stats.second);
            if (allPressed) {
                puzzleResetTimers[roomId] += dt;
                if (puzzleResetTimers[roomId] >= 1.5f)
                    resetPuzzle(roomId);
            } else {
                puzzleResetTimers[roomId] = 0.f;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Unified pushable block update (push charge + slide animation)
    // -----------------------------------------------------------------------
    {
        const float dt = Window::GetFrameTime();
        std::set<int> roomsToCheck;
        std::set<int> mirrorRoomsToCheck;

        ECS::getWorld().filter<Pushable, RigidBody2D>().each([&](flecs::entity e, Pushable& p, RigidBody2D& rb) {
            if (!b2Body_IsValid(rb.RigidBody)) return;

            // Skip PuzzleBlocks that are fading or spawning — handled in separate loop above
            if (e.has<PuzzleBlock>()) {
                auto* pb = e.get<PuzzleBlock>();
                if (pb->fadingOut || pb->spawningIn) return;
            }

            if (!p.isMoving) {
                b2Vec2 bpos = b2Body_GetPosition(rb.RigidBody);
                float snapX = roundf(bpos.x - 0.5f) + 0.5f;
                float snapY = roundf(bpos.y - 0.5f) + 0.5f;
                b2Body_SetTransform(rb.RigidBody, { snapX, snapY }, b2Body_GetRotation(rb.RigidBody));
                b2Body_SetLinearVelocity(rb.RigidBody, { 0.f, 0.f });

                if (p.pushCooldown > 0.f) {
                    p.pushCooldown -= dt;
                    if (p.pushCooldown <= 0.f) {
                        p.pushCooldown = 0.f;
                        // Player still holding — skip charge delay, fire immediately next frame
                        if (p.pendingPushPlayerId != 0) {
                            p.pushChargePlayerId  = p.pendingPushPlayerId;
                            p.pushChargeTimer     = Pushable::PUSH_CHARGE_DURATION;
                            p.pendingPushPlayerId = 0;
                        }
                    } else {
                        p.pushChargePlayerId = 0;
                        p.pushChargeTimer    = 0.f;
                    }
                    return;
                }

                if (p.pushChargePlayerId != 0 && p.settled) {
                    p.pushChargeTimer += dt;
                    if (p.pushChargeTimer >= Pushable::PUSH_CHARGE_DURATION) {
                        p.pushChargeTimer    = 0.f;
                        flecs::entity pusher = ECS::getWorld().entity(p.pushChargePlayerId);
                        p.pushChargePlayerId = 0;
                        if (pusher.is_alive() && pusher.has<RigidBody2D>()) {
                            b2Vec2 ppos = b2Body_GetPosition(pusher.get<RigidBody2D>()->RigidBody);
                            float dx = snapX - ppos.x, dy = snapY - ppos.y;
                            vf2d dir = fabsf(dx) >= fabsf(dy)
                                       ? vf2d{ dx > 0 ? 1.f : -1.f, 0.f }
                                       : vf2d{ 0.f, dy > 0 ? 1.f : -1.f };
                            vf2d target = { snapX + dir.x, snapY + dir.y };
                            int tx = (int)roundf(target.x - 0.5f), ty = (int)roundf(target.y - 0.5f);
                            Dungeon dg = World::getDungeon();
                            bool walkable = dg.isWalkable(dg.getTile(tx, ty));
                            bool occupied = false;
                            ECS::getWorld().filter<Pushable, RigidBody2D>().each([&](flecs::entity oe, Pushable& op, RigidBody2D& orb) {
                                if (oe == e) return;
                                b2Vec2 opos = b2Body_GetPosition(orb.RigidBody);
                                vf2d check = op.isMoving ? op.targetPos
                                           : vf2d{ roundf(opos.x - 0.5f) + 0.5f, roundf(opos.y - 0.5f) + 0.5f };
                                if (fabsf(check.x - target.x) < 0.1f && fabsf(check.y - target.y) < 0.1f) occupied = true;
                            });
                            // Rail constraint: railed blocks can only move along their axis
                            bool railAllows = true;
                            if (e.has<MirrorBlock>()) {
                                const auto& mb = *e.get<MirrorBlock>();
                                if (mb.rail == MirrorBlock::Rail::Horizontal && dir.y != 0.f) railAllows = false;
                                if (mb.rail == MirrorBlock::Rail::Vertical   && dir.x != 0.f) railAllows = false;
                            }
                            if (walkable && !occupied && railAllows) {
                                p.pendingPushPlayerId = pusher.id(); // keep player for re-push after cooldown
                                p.isMoving   = true;
                                p.settled    = false;
                                p.startPos   = { snapX, snapY };
                                p.targetPos  = target;
                                p.moveTimer  = 0.f;
                                p.pushCooldown = Pushable::MOVE_DURATION + Pushable::PUSH_COOLDOWN;
                                if (e.has<MirrorBlock>())
                                    mirrorRoomsToCheck.insert(e.get<MirrorBlock>()->roomId);
                                if (e.has<BeamSplitter>())
                                    mirrorRoomsToCheck.insert(e.get<BeamSplitter>()->roomId);
                                if (e.has<BeamCombiner>())
                                    mirrorRoomsToCheck.insert(e.get<BeamCombiner>()->roomId);
                            }
                        }
                    }
                }
                return;
            }

            p.moveTimer += dt;
            float t  = std::min(p.moveTimer / Pushable::MOVE_DURATION, 1.0f);
            float et = t * t * (3.f - 2.f * t); // smoothstep
            vf2d pos = {
                p.startPos.x + (p.targetPos.x - p.startPos.x) * et,
                p.startPos.y + (p.targetPos.y - p.startPos.y) * et
            };
            b2Body_SetTransform(rb.RigidBody, { pos.x, pos.y }, b2Body_GetRotation(rb.RigidBody));
            b2Body_SetLinearVelocity(rb.RigidBody, { 0.f, 0.f });

            if (t >= 1.0f) {
                b2Body_SetTransform(rb.RigidBody, { p.targetPos.x, p.targetPos.y }, b2Body_GetRotation(rb.RigidBody));
                p.isMoving = false;
                p.settled  = true;
                // pendingPushPlayerId preserved — cooldown will promote it to pushChargePlayerId

                if (e.has<PuzzleBlock>() && !puzzleSolvedRooms.count(e.get<PuzzleBlock>()->roomId))
                    roomsToCheck.insert(e.get<PuzzleBlock>()->roomId);
                if (e.has<MirrorBlock>())
                    mirrorRoomsToCheck.insert(e.get<MirrorBlock>()->roomId);
            }
        });

        for (int roomId : roomsToCheck)
            checkPuzzleMatches(roomId);
        for (int roomId : mirrorRoomsToCheck)
            checkMirrorPuzzle(roomId);
    }

    // -----------------------------------------------------------------------
    // Beam trace: reset receptors, trace all emitters, store segments
    // -----------------------------------------------------------------------
    {
        auto localDungeon = World::getDungeon();

        // Reset all receptors and combiners
        ECS::getWorld().filter<BeamReceptor>().each([](flecs::entity, BeamReceptor& br) {
            br.isLit        = false;
            br.currentColor = BeamColors::None;
        });
        ECS::getWorld().filter<BeamCombiner>().each([](flecs::entity, BeamCombiner& bc) {
            bc.accumulated = BeamColors::None;
        });

        // Trace each emitter, store segments for rendering
        BeamTraceContext beamCtx;
        BuildBeamTraceContext(beamCtx);
        roomBeamSegments.clear();
        ECS::getWorld().filter<BeamEmitter>().each([&](flecs::entity, BeamEmitter& be) {
            if (mirrorPuzzleSolvedRooms.count(be.roomId)) return;
            auto segs = traceBeam(be.tilePos, be.direction, be.color, localDungeon, beamCtx);
            auto& roomSegs = roomBeamSegments[be.roomId];
            roomSegs.insert(roomSegs.end(), segs.begin(), segs.end());
        });

        // Check solved state for all rooms with emitters
        std::set<int> emitterRooms;
        ECS::getWorld().filter<BeamEmitter>().each([&](flecs::entity, BeamEmitter& be) {
            emitterRooms.insert(be.roomId);
        });
        for (int roomId : emitterRooms)
            checkMirrorPuzzle(roomId);
    }

	Draw::BeginMode2D();

    widthMargin = (int)(1.f + (((float)Window::GetWidth() / (float)Configuration::tileWidth) / 2.f) / Camera::GetScale());
    heightMargin = (int)(1.f + (((float)Window::GetHeight() /(float)Configuration::tileHeight) / 2.f) / Camera::GetScale());

    int leftMargin = std::max(static_cast<int>(Camera::GetTarget().x) / Configuration::tileWidth - widthMargin, 0);
    int rightMargin = std::min(leftMargin + widthMargin * 2 + 1, tileData.width);

    int topMargin = std::max(static_cast<int>(Camera::GetTarget().y) / Configuration::tileHeight - heightMargin, 0);
    int bottomMargin = std::min(topMargin + heightMargin * 2 + 1, tileData.height);

	auto localDungeon = World::getDungeon();
    auto localDungeonTiles = localDungeon.getTiles();




for (int y = topMargin; y < bottomMargin; y++) {
		for (int x = leftMargin; x < rightMargin; x++) {
            int index = y * tileData.width + x;

            const rectf destRect = {
                    static_cast<float>(x * Configuration::tileWidth),
                    static_cast<float>(y * Configuration::tileHeight),
                    static_cast<float>(Configuration::tileWidth),
                    static_cast<float>(Configuration::tileHeight)
            };
            Tile tile = localDungeonTiles.at(index);

            if (tile == Tile::CLOSEDHDOOR || tile == Tile::CLOSEDVDOOR) {
                tile = Tile::FLOOR;
            }


            const rectf srcRect = getTile(tileData.tiles.at(index));

            if (tile == Tile::FLOOR) {

                Draw::TexturePart(dungeonTileset, {destRect.x, destRect.y}, {destRect.width, destRect.height}, srcRect, WHITE);

                Tile UPLEFTTILE = localDungeon.getTile(x - 1, y - 1);
                Tile LEFTTILE = localDungeon.getTile(x - 1, y);
                Tile UPTILE = localDungeon.getTile(x, y - 1);


                if (LEFTTILE == Tile::WALL && UPTILE == Tile::WALL) {
                    Draw::TexturePart(dungeonTileset, {destRect.x, destRect.y}, {destRect.width, destRect.height}, getTile(50), WHITE);
                } else if (LEFTTILE == Tile::WALL && (UPTILE == Tile::FLOOR || UPTILE == Tile::CLOSEDHDOOR) && UPLEFTTILE == Tile::WALL) {
                    Draw::TexturePart(dungeonTileset, {destRect.x, destRect.y}, {destRect.width, destRect.height}, getTile(52), WHITE);
                } else if (LEFTTILE == Tile::WALL && (UPTILE == Tile::FLOOR || UPTILE == Tile::CLOSEDHDOOR) && dungeon.isWalkable(UPLEFTTILE)) {
                    Draw::TexturePart(dungeonTileset, {destRect.x, destRect.y}, {destRect.width, destRect.height}, getTile(51), WHITE);
                } else if ((LEFTTILE == Tile::FLOOR || LEFTTILE == Tile::CLOSEDHDOOR) && UPTILE == Tile::WALL && UPLEFTTILE == Tile::WALL) {
                    Draw::TexturePart(dungeonTileset, {destRect.x, destRect.y}, {destRect.width, destRect.height}, getTile(48), WHITE);
                } else if ((LEFTTILE == Tile::FLOOR || LEFTTILE == Tile::CLOSEDHDOOR) && (UPTILE == Tile::FLOOR || UPTILE == Tile::CLOSEDHDOOR) &&
                           UPLEFTTILE == Tile::WALL) {
                    Draw::TexturePart(dungeonTileset, {destRect.x, destRect.y}, {destRect.width, destRect.height}, getTile(53), WHITE);
                } else if ((LEFTTILE == Tile::FLOOR || LEFTTILE == Tile::CLOSEDHDOOR) && UPTILE == Tile::WALL &&
                           (UPLEFTTILE == Tile::FLOOR || UPLEFTTILE == Tile::CLOSEDHDOOR)) {
                    Draw::TexturePart(dungeonTileset, {destRect.x, destRect.y}, {destRect.width, destRect.height}, getTile(49), WHITE);
                }

            }

        }
    }



    LightLayer::Draw({0,0}, Window::GetSize());




	for (int y = topMargin; y < bottomMargin; y++) {
		for (int x = leftMargin; x < rightMargin; x++) {
            int index = y * tileData.width + x;

            const rectf destRect = {
                    static_cast<float>(x * Configuration::tileWidth),
                    static_cast<float>(y * Configuration::tileHeight),
                    static_cast<float>(Configuration::tileWidth),
                    static_cast<float>(Configuration::tileHeight)
            };
            Tile tile = localDungeonTiles.at(index);

            if (tile == Tile::CLOSEDHDOOR || tile == Tile::CLOSEDVDOOR) {
                tile = Tile::FLOOR;
            }


            const rectf srcRect = getTile(tileData.tiles.at(index));



            if (tile != Tile::FLOOR) {
                Draw::TexturePart(dungeonTileset, {destRect.x, destRect.y}, {destRect.width, destRect.height}, srcRect, WHITE);
            }
        }
    }

    // Draw pressure plates
    {
        const float tileW = (float)Configuration::tileWidth;
        const float tileH = (float)Configuration::tileHeight;
        ECS::getWorld().filter<PressurePlate>().each([&](flecs::entity, PressurePlate& pp) {
            if (puzzleSolvedRooms.count(pp.roomId)) return;
            float px = (pp.tilePos.x + 0.5f) * tileW;
            float py = (pp.tilePos.y + 0.5f) * tileH;
            float hw = 0.38f * tileW;
            float hh = 0.38f * tileH;

            // Fill: dark when idle, bright when pressed
            float holdFrac = 0.f;
            if (puzzleResetTimers.count(pp.roomId))
                holdFrac = std::min(puzzleResetTimers.at(pp.roomId) / 1.5f, 1.f);

            Color fill   = pp.isPressed ? Color{220, 220, 100, 220} : Color{100, 100, 100, 160};
            Color border = pp.isPressed ? Color{255, 255, 200, 255} : Color{180, 180, 180, 200};

            // Flash brighter as hold completes
            if (pp.isPressed && holdFrac > 0.f) {
                uint8_t boost = (uint8_t)(holdFrac * 35.f);
                fill.r = (uint8_t)std::min(255, (int)fill.r + boost);
                fill.g = (uint8_t)std::min(255, (int)fill.g + boost);
            }

            Draw::RectangleFilled({ px - hw, py - hh }, { hw * 2.f, hh * 2.f }, fill);
            Draw::Rectangle(      { px - hw, py - hh }, { hw * 2.f, hh * 2.f }, border);
        });
    }

    // Draw puzzle blocks
    {
        const float tileW = (float)Configuration::tileWidth;
        const float tileH = (float)Configuration::tileHeight;
        const auto puzzleDrawFilter = ECS::getWorld().filter<PuzzleBlock, RigidBody2D>();
        puzzleDrawFilter.each([&](flecs::entity, PuzzleBlock& pb, RigidBody2D& rb) {
            if (!b2Body_IsValid(rb.RigidBody)) return;
            b2Vec2 pos = b2Body_GetPosition(rb.RigidBody);
            float cx = pos.x * tileW;
            float cy = pos.y * tileH;

            float alpha = 1.f;
            float scale = 1.f;
            if (pb.fadingOut) {
                // Match-clear: expand and vanish
                float t = pb.fadeTimer / PuzzleBlock::FADE_DURATION;
                alpha = 1.f - t;
                scale = 1.f + t * 0.4f;
            } else if (pb.spawningIn) {
                // Spawn-in after reset: grow from small to full size
                float t = pb.spawnFadeTimer / PuzzleBlock::FADE_IN_DURATION;
                alpha = t;
                scale = 0.6f + t * 0.4f;
            } else {
                // Hold-to-reset: fade blocks out while plates are held
                if (!puzzleSolvedRooms.count(pb.roomId) && puzzleResetTimers.count(pb.roomId))
                    alpha = 1.f - std::min(puzzleResetTimers.at(pb.roomId) / 1.5f, 1.f);
            }

            float hw = 0.44f * tileW * scale;
            float hh = 0.44f * tileH * scale;
            uint8_t a  = (uint8_t)(alpha * 255.f);
            Color col    = PUZZLE_BLOCK_COLORS[pb.colorIdx]; col.a = a;
            Color border = { 255, 255, 255, (uint8_t)(160 * alpha) };
            Draw::RectangleFilled({ cx - hw, cy - hh }, { hw * 2.f, hh * 2.f }, col);
            Draw::Rectangle({ cx - hw, cy - hh }, { hw * 2.f, hh * 2.f }, border);
        });
    }

    // Draw beam emitters
    {
        const float tileW = (float)Configuration::tileWidth;
        const float tileH = (float)Configuration::tileHeight;
        ECS::getWorld().filter<BeamEmitter>().each([&](flecs::entity, BeamEmitter& be) {
            if (mirrorPuzzleSolvedRooms.count(be.roomId)) return;
            float cx = (be.tilePos.x + 0.5f) * tileW;
            float cy = (be.tilePos.y + 0.5f) * tileH;
            float hw = 0.4f * tileW, hh = 0.4f * tileH;
            Draw::RectangleFilled({ cx - hw, cy - hh }, { hw * 2.f, hh * 2.f }, { 255, 220, 60, 255 });
            Draw::Rectangle({ cx - hw, cy - hh }, { hw * 2.f, hh * 2.f }, { 255, 255, 255, 200 });
            // Arrow indicating direction
            vf2d arrowEnd = { cx + be.direction.x * hw * 0.7f, cy + be.direction.y * hh * 0.7f };
            Draw::Line({ cx, cy }, arrowEnd, { 255, 255, 255, 255 });
        });
    }

    // Draw beam receptors
    {
        const float tileW = (float)Configuration::tileWidth;
        const float tileH = (float)Configuration::tileHeight;
        ECS::getWorld().filter<BeamReceptor>().each([&](flecs::entity, BeamReceptor& br) {
            if (mirrorPuzzleSolvedRooms.count(br.roomId)) return;
            float cx = (br.tilePos.x + 0.5f) * tileW;
            float cy = (br.tilePos.y + 0.5f) * tileH;
            float r  = 0.35f * tileW;
            Color req  = beamToColor(br.requiredColor);
            Color fill = br.isLit ? req : Color{ req.r/3, req.g/3, req.b/3, 200 };
            Draw::CircleFilled({ cx, cy }, r, fill);
            Draw::Circle({ cx, cy }, r, { 255, 255, 255, 180 });
        });
    }

    // Draw mirror blocks
    {
        const float tileW = (float)Configuration::tileWidth;
        const float tileH = (float)Configuration::tileHeight;
        ECS::getWorld().filter<MirrorBlock, RigidBody2D>().each([&](flecs::entity, MirrorBlock& mb, RigidBody2D& rb) {
            if (!b2Body_IsValid(rb.RigidBody)) return;
            b2Vec2 pos = b2Body_GetPosition(rb.RigidBody);
            float cx = pos.x * tileW;
            float cy = pos.y * tileH;
            float hw = 0.44f * tileW, hh = 0.44f * tileH;
            // Draw rail track as a dark underlay before the block
            if (mb.rail == MirrorBlock::Rail::Horizontal && mb.railMin < mb.railMax) {
                Draw::RectangleFilled(
                    vf2d{ mb.railMin * tileW, cy - tileH * 0.5f },
                    vf2d{ (mb.railMax - mb.railMin + 1) * tileW, tileH },
                    { 0, 0, 0, 20 });
            } else if (mb.rail == MirrorBlock::Rail::Vertical && mb.railMin < mb.railMax) {
                Draw::RectangleFilled(
                    vf2d{ cx - tileW * 0.5f, mb.railMin * tileH },
                    vf2d{ tileW, (mb.railMax - mb.railMin + 1) * tileH },
                    { 0, 0, 0, 20 });
            }
            Draw::RectangleFilled({ cx - hw, cy - hh }, { hw * 2.f, hh * 2.f }, { 60, 160, 200, 220 });
            Draw::Rectangle({ cx - hw, cy - hh }, { hw * 2.f, hh * 2.f }, { 200, 240, 255, 200 });
            // Draw / or \ line
            Color linCol = { 255, 255, 255, 230 };
            if (mb.orientation == MirrorBlock::Orientation::Slash)
                Draw::Line({ cx + hw * 0.7f, cy - hh * 0.7f }, { cx - hw * 0.7f, cy + hh * 0.7f }, linCol);
            else
                Draw::Line({ cx - hw * 0.7f, cy - hh * 0.7f }, { cx + hw * 0.7f, cy + hh * 0.7f }, linCol);
        });
    }

    // Draw beam segments
    {
        const float tileW = (float)Configuration::tileWidth;
        const float tileH = (float)Configuration::tileHeight;
        for (auto& [roomId, segs] : roomBeamSegments) {
            for (auto& seg : segs) {
                vf2d p1 = { seg.start.x * tileW, seg.start.y * tileH };
                vf2d p2 = { seg.end.x   * tileW, seg.end.y   * tileH };
                Color core = beamToColor(seg.color);
                Color glow = { core.r, core.g, core.b, 70 };
                Draw::ThickLine(p1, p2, glow,  6.f);
                Draw::ThickLine(p1, p2, core, 3.f);
            }
        }
    }

	playerFilter.each([&](flecs::entity entity, PlayerIndex index) {
		auto* input = entity.get_mut<PlayerInput>();
		auto* playerClass = entity.get_mut<PlayerClass>();

		if (input->shooting)
		{
			playerClass->shoot(entity);
		}
	});

	drawEntities();

	World::draw();

    if (Configuration::showGameStats) {
        const auto debugRigids = ECS::getWorld().filter<DebugRigidBody2D>();
        debugRigids.each([&](flecs::entity entity, DebugRigidBody2D db) {
            db.draw(entity);
        });
    }

    // Chain lightning render
    {
        const float dt = Window::GetFrameTime();
        std::vector<flecs::entity> expiredLightning;

        const auto lightFilter = ECS::getWorld().filter<ChainLightningData>();
        lightFilter.each([&](flecs::entity e, ChainLightningData& cl) {
            cl.timer += dt;
            if (cl.timer >= ChainLightningData::MAX_TIMER) {
                expiredLightning.push_back(e);
                return;
            }

            float alpha = 1.f - (cl.timer / ChainLightningData::MAX_TIMER);
            // Outer glow: thick, faint yellow
            Color outer = { 255, 240, 100, static_cast<uint8_t>(alpha * 60.f) };
            // Core bolt: thin, bright white
            Color core  = { 220, 240, 255, static_cast<uint8_t>(alpha * 230.f) };

            for (size_t i = 0; i + 1 < cl.points.size(); ++i) {
                Draw::ThickLine(cl.points[i], cl.points[i + 1], outer, 5.0f);
                Draw::ThickLine(cl.points[i], cl.points[i + 1], core,  1.5f);
            }
        });

        for (auto& e : expiredLightning) ECS::removeEntity(&e);
    }

    // Fragment update + render
    {
        const float dt = Window::GetFrameTime() / Configuration::slowMotionFactor;
        constexpr float orbitRadius = 1.8f; // tile units
        constexpr float orbitSpeed  = 2.0f; // radians/sec

        std::vector<flecs::entity> deadFragments;

        const auto fragFilter = ECS::getWorld().filter<FragmentData, RigidBody2D>();
        fragFilter.each([&](flecs::entity fragEntity, FragmentData& frag, RigidBody2D& rb) {
            // Spawn animation — don't tick lifetime until fully deployed
            if (frag.spawnProgress < 1.f) {
                frag.spawnProgress = std::min(1.f, frag.spawnProgress + dt / FragmentData::SPAWN_DURATION);
            } else {
                frag.lifetime -= dt;
            }

            if (frag.lifetime <= 0.f || frag.hp <= 0.f) {
                deadFragments.push_back(fragEntity);
                return;
            }

            flecs::entity owner = ECS::getWorld().entity(frag.ownerId);
            if (!owner.is_alive() || !owner.has<RigidBody2D>()) {
                deadFragments.push_back(fragEntity);
                return;
            }
            b2Vec2 ownerPos = b2Body_GetPosition(owner.get<RigidBody2D>()->RigidBody);

            // Collect sibling fragment IDs for this owner to determine orbit slot
            // Only count deployed fragments for spacing; spawning ones trail behind
            std::vector<uint64_t> siblingIds;
            const auto sibFilter = ECS::getWorld().filter<FragmentData>();
            sibFilter.each([&](flecs::entity se, FragmentData& sd) {
                if (sd.ownerId == frag.ownerId && sd.lifetime > 0.f && sd.hp > 0.f)
                    siblingIds.push_back(se.id());
            });
            std::sort(siblingIds.begin(), siblingIds.end());

            int myIndex = static_cast<int>(std::find(siblingIds.begin(), siblingIds.end(), fragEntity.id()) - siblingIds.begin());
            int total   = static_cast<int>(siblingIds.size());
            if (total == 0) return;

            float angle = (static_cast<float>(myIndex) / total) * 2.f * PI
                        + Configuration::gameTime * orbitSpeed;

            // Ease out: fast launch, gentle settle into orbit
            float easedProgress = 1.f - (1.f - frag.spawnProgress) * (1.f - frag.spawnProgress);
            float currentRadius = orbitRadius * easedProgress;

            vf2d newPos = {
                ownerPos.x + currentRadius * cosf(angle),
                ownerPos.y + currentRadius * sinf(angle)
            };
            b2Body_SetTransform(rb.RigidBody, { newPos.x, newPos.y }, b2Body_GetRotation(rb.RigidBody));

            // Color: lerp blue → red based on whichever is more depleted
            float hpRatio       = frag.hp       / frag.maxHp;
            float lifetimeRatio = frag.lifetime  / frag.maxLifetime;
            float ratio = std::min(hpRatio, lifetimeRatio);

            auto lerp8 = [](float a, float b, float t) -> uint8_t {
                return static_cast<uint8_t>(a + (b - a) * t);
            };
            Color gemColor  = { lerp8(220, 0,  ratio), lerp8(20, 60, ratio), lerp8(30, 210, ratio), 210 };
            Color coreColor = { lerp8(255, 80, ratio), lerp8(80, 140, ratio), lerp8(80, 255, ratio), 255 };
            Color glowColor = { gemColor.r, gemColor.g, gemColor.b, 50 };

            const float tileW = static_cast<float>(Configuration::tileWidth);
            vf2d drawPos = { newPos.x * tileW, newPos.y * tileW };
            float pixR = 0.28f * tileW;

            Draw::CircleFilled(drawPos, pixR * 1.6f, glowColor); // outer glow
            Draw::CircleFilled(drawPos, pixR,         gemColor);  // gem body
            Draw::CircleFilled(drawPos, pixR * 0.35f, coreColor); // bright core
        });

        for (auto& e : deadFragments) ECS::removeEntity(&e);
    }

    // Time warp update + render
    {
        const float dt = Window::GetFrameTime() / Configuration::slowMotionFactor;
        std::vector<flecs::entity> expiredWarps;

        const auto warpFilter = ECS::getWorld().filter<TimeWarpData>();
        warpFilter.each([&](flecs::entity warpEntity, TimeWarpData& warp) {
            warp.timer -= dt;
            if (warp.timer <= 0.f) {
                expiredWarps.push_back(warpEntity);
                return;
            }

            const float tileW = static_cast<float>(Configuration::tileWidth);
            const float tileH = static_cast<float>(Configuration::tileHeight);
            vf2d center = { warp.position.x * tileW, warp.position.y * tileH };
            float pixRadius = warp.radius * tileW;

            // Background fill
            Draw::CircleFilled(center, pixRadius, { 80, 0, 160, 18 });

            // Pulsing outer ring
            float pulse = 1.0f + 0.05f * sinf(Configuration::gameTime * 5.0f);
            float outerR = pixRadius * pulse;
            constexpr int segments = 36;
            for (int s = 0; s < segments; ++s) {
                float a0 = (static_cast<float>(s)     / segments) * 2.f * PI;
                float a1 = (static_cast<float>(s + 1) / segments) * 2.f * PI;
                vf2d p0 = { center.x + outerR * cosf(a0), center.y + outerR * sinf(a0) };
                vf2d p1 = { center.x + outerR * cosf(a1), center.y + outerR * sinf(a1) };
                Draw::ThickLine(p0, p1, { 160, 50, 255, 140 }, 2.0f);
            }

            // Inner ring
            float innerR = pixRadius * 0.5f;
            for (int s = 0; s < segments; ++s) {
                float a0 = (static_cast<float>(s)     / segments) * 2.f * PI;
                float a1 = (static_cast<float>(s + 1) / segments) * 2.f * PI;
                vf2d p0 = { center.x + innerR * cosf(a0), center.y + innerR * sinf(a0) };
                vf2d p1 = { center.x + innerR * cosf(a1), center.y + innerR * sinf(a1) };
                Draw::ThickLine(p0, p1, { 160, 50, 255, 60 }, 1.5f);
            }

            // Duration arc — depletes clockwise from top
            float progress = warp.timer / warp.maxTimer;
            int activeSeg = static_cast<int>(progress * segments);
            for (int s = 0; s < activeSeg; ++s) {
                float a0 = (-PI / 2.f) + (static_cast<float>(s)     / segments) * 2.f * PI;
                float a1 = (-PI / 2.f) + (static_cast<float>(s + 1) / segments) * 2.f * PI;
                vf2d p0 = { center.x + outerR * cosf(a0), center.y + outerR * sinf(a0) };
                vf2d p1 = { center.x + outerR * cosf(a1), center.y + outerR * sinf(a1) };
                Draw::ThickLine(p0, p1, { 200, 100, 255, 220 }, 3.5f);
            }

            // Apply slow to enemies in range
            const auto enemySlowFilter = ECS::getWorld().filter<EnemyEntity, RigidBody2D, AIController>();
            enemySlowFilter.each([&](flecs::entity enemy, EnemyEntity, RigidBody2D& rb, AIController& ai) {
                b2Vec2 eposB2 = b2Body_GetPosition(rb.RigidBody);
                vf2d epos = { eposB2.x, eposB2.y };
                float distSq = (epos - warp.position).mag2();
                if (distSq <= warp.radius * warp.radius) {
                    b2Vec2 vel = b2Body_GetLinearVelocity(rb.RigidBody);
                    float slowedMax = (ai.maxSpeed / Configuration::slowMotionFactor) * warp.slowFactor;
                    float speed = sqrtf(vel.x * vel.x + vel.y * vel.y);
                    if (speed > slowedMax && speed > 0.001f) {
                        float scale = slowedMax / speed;
                        b2Body_SetLinearVelocity(rb.RigidBody, { vel.x * scale, vel.y * scale });
                    }
                }
            });
        });

        for (auto& e : expiredWarps) ECS::removeEntity(&e);
    }

    // Whirlwind object update + render
    {
        const float dt         = Window::GetFrameTime() / Configuration::slowMotionFactor;
        constexpr float rampUp   = 3.5f;  // radians/sec per second
        constexpr float rampDown = 2.5f;
        const float tileW = static_cast<float>(Configuration::tileWidth);

        std::vector<flecs::entity> deadWhirlwinds;

        const auto wwFilter = ECS::getWorld().filter<WhirlwindObjectData, RigidBody2D>();
        wwFilter.each([&](flecs::entity wwEnt, WhirlwindObjectData& wwd, RigidBody2D& rb) {
            flecs::entity owner = ECS::getWorld().entity(wwd.ownerId);
            if (!owner.is_alive() || !owner.has<RigidBody2D>()) {
                deadWhirlwinds.push_back(wwEnt);
                return;
            }

            auto* playerInput = owner.get<PlayerInput>();
            bool isRamping = playerInput->shooting && playerInput->selectedWeapon == 2;

            if (isRamping) {
                wwd.spinSpeed = std::min(wwd.maxSpinSpeed, wwd.spinSpeed + rampUp * dt);
            } else {
                wwd.spinSpeed = std::max(0.f, wwd.spinSpeed - rampDown * dt);
            }

            if (wwd.spinSpeed <= 0.f && !isRamping) {
                deadWhirlwinds.push_back(wwEnt);
                return;
            }

            wwd.orbitAngle += wwd.spinSpeed * dt;

            b2Vec2 ownerPos = b2Body_GetPosition(owner.get<RigidBody2D>()->RigidBody);
            float slotOffset = static_cast<float>(wwd.slotIndex) * (2.f * PI / 3.f);
            float angle = wwd.orbitAngle + slotOffset;

            vf2d newPos = {
                ownerPos.x + wwd.orbitRadius * cosf(angle),
                ownerPos.y + wwd.orbitRadius * sinf(angle)
            };
            b2Body_SetTransform(rb.RigidBody, { newPos.x, newPos.y }, b2Body_GetRotation(rb.RigidBody));

            // Color: cold blue at low spin → hot orange/white at max spin
            float ratio = wwd.spinSpeed / wwd.maxSpinSpeed;
            auto lerp8 = [](float a, float b, float t) -> uint8_t {
                return static_cast<uint8_t>(a + (b - a) * t);
            };
            Color coreColor = { lerp8(60, 255, ratio), lerp8(120, 160, ratio), lerp8(230, 40, ratio), 255 };
            Color glowColor = { coreColor.r, coreColor.g, coreColor.b, static_cast<uint8_t>(40 + ratio * 80.f) };

            vf2d drawPos    = { newPos.x * tileW, newPos.y * tileW };
            float pixR      = 0.32f * tileW;

            // Draw thin chain line from owner to object
            vf2d ownerDraw = { ownerPos.x * tileW, ownerPos.y * tileW };
            Draw::ThickLine(ownerDraw, drawPos, { 180, 180, 180, static_cast<uint8_t>(60 + ratio * 100.f) }, 1.5f);

            Draw::CircleFilled(drawPos, pixR * 1.8f, glowColor);
            Draw::CircleFilled(drawPos, pixR,         coreColor);
            Draw::CircleFilled(drawPos, pixR * 0.35f, { 255, 255, 255, static_cast<uint8_t>(180 + ratio * 75.f) });
        });

        for (auto& e : deadWhirlwinds) ECS::removeEntity(&e);
    }

	Draw::EndMode2D();


	// Floating damage numbers
	{
		auto fontNormal = AssetHandler::GetFont("assets/fonts/APL386.ttf", 18);
		auto fontDeath  = AssetHandler::GetFont("assets/fonts/APL386.ttf", 24);

		// Live numbers — window still open, counting up in real time
		const auto liveFilter = ECS::getWorld().filter<DamageAccumulator>();
		liveFilter.each([&](flecs::entity entity, DamageAccumulator& accum) {
			if (!accum.active || accum.accumulated <= 0.f) return;
			vf2d screenPos = Camera::ToScreenSpace({ accum.spawnPos.x + accum.offsetX, accum.spawnPos.y - 60.f });
			const char* text = Helpers::TextFormat("%.0f", accum.accumulated);
			const int textWidth = Text::MeasureText(fontNormal, text, 18);
			Text::DrawText(fontNormal, { screenPos.x - textWidth / 2.f, screenPos.y }, text, WHITE);
		});
		const float dt = Window::GetFrameTime();

		for (auto it = floatingNumbers.begin(); it != floatingNumbers.end(); ) {
			it->timer += dt;
			if (it->timer >= it->maxTimer) {
				it = floatingNumbers.erase(it);
				continue;
			}

			const float progress = it->timer / it->maxTimer;
			const float alpha    = 1.0f - progress;
			const float yOffset  = progress * 80.f;

			vf2d worldPos = { it->worldPixelPos.x + it->offsetX, it->worldPixelPos.y - yOffset - 60.f };
			vf2d screenPos = Camera::ToScreenSpace(worldPos);

			auto&       font     = it->isDeath ? fontDeath : fontNormal;
			const int   fontSize = it->isDeath ? 24 : 18;
			Color color = it->color;
			color.a = static_cast<uint8_t>(alpha * 255.f);

			const int   textWidth = Text::MeasureText(font, it->text.c_str(), fontSize);
			Text::DrawText(font, { screenPos.x - textWidth / 2.f, screenPos.y }, it->text.c_str(), color);

			++it;
		}
	}

	drawUI();

	const auto deleteFilter = ECS::getWorld().filter<DeleteMe>();
	std::vector<flecs::entity> entitiesToDelete;
	deleteFilter.each([&](flecs::entity entity, DeleteMe _dm) {
		entitiesToDelete.push_back(entity);

	});

	for (auto& entity: entitiesToDelete) ECS::removeEntity(&entity);

	if (Input::KeyPressed(SDLK_ESCAPE)) {
		//TODO: debug this in linux/VM produces a black screen
		//_th->setTextureFromImage("_pause", LoadImageFromScreen());
		State::SetState("pause");
	}

	if (World::getSlowMotionLerp()->started)
	{
		Configuration::slowMotionFactor = World::getSlowMotionLerp()->getValue();
	}

	if (World::getSlowMotionLerp()->isFinished())
	{
		World::getSlowMotionLerp()->started = false;
	}

	World::setSlowMotionTimer(World::getSlowMotionTimer() - (float)Window::GetFrameTime() / Configuration::slowMotionFactor);
	Configuration::gameTime += Window::GetFrameTime();
	ECS::getWorld().progress((float)Window::GetFrameTime());
}

void GameState::drawEntities()
{
	for (const auto& _spawner : spawners) { _spawner->draw(); }

	std::vector<flecs::entity> renderableEntities;

	const auto renderFilter = ECS::getWorld().filter<Render2DComp>();
	renderFilter.each([&](flecs::entity entity, Render2DComp renderer) {
		renderableEntities.push_back(entity);
		renderer.draw(&entity);
	});

	std::sort(renderableEntities.begin(), renderableEntities.end(),
		[](const flecs::entity& entity1, const flecs::entity& entity2) {
			b2Vec2 p1 = b2Body_GetPosition(entity1.get<RigidBody2D>()->RigidBody);
			b2Vec2 p2 = b2Body_GetPosition(entity2.get<RigidBody2D>()->RigidBody);
			return p1.y < p2.y;
		});

	for (auto& entity : renderableEntities) {
		entity.get_mut<Render2DComp>()->draw(&entity);
		
	}

	std::vector<flecs::entity> entitiesHealth;

	const auto healthFilter = ECS::getWorld().filter<Health>();
	healthFilter.each([&](flecs::entity entity, Health health) {

		health.draw(entity);

		if (health.currentHealth <= 0.0f) {
			entitiesHealth.push_back(entity);

			if (entity.has<DamageAccumulator>()) {
				auto* accum = entity.get_mut<DamageAccumulator>();
				if (accum->accumulated > 0.f) {
					FloatingNumberData num;
					num.text = Helpers::TextFormat("%.0f", accum->consume());
					num.color = { 255, 165, 0, 255 };
					num.worldPixelPos = accum->spawnPos;
					num.offsetX = accum->offsetX;
					num.isDeath = true;
					num.maxTimer = 1.4f;
					floatingNumbers.push_back(num);
					accum->active = false;
				}
			}

			// Distribute XP to all players on enemy death
			if (entity.has<EnemyEntity>()) {
				float xpAmount = health.maxHealth;
				const auto xpPlayerFilter = ECS::getWorld().filter<PlayerXP, RigidBody2D>();
				xpPlayerFilter.each([&](flecs::entity playerEntity, PlayerXP& pxp, RigidBody2D& rb) {
					pxp.xp += xpAmount;
					b2Vec2 ppos = b2Body_GetPosition(rb.RigidBody);
					FloatingNumberData xpNum;
					xpNum.text = Helpers::TextFormat("+%.0f", xpAmount);
					xpNum.color = { 255, 215, 0, 255 };
					xpNum.worldPixelPos = { ppos.x * static_cast<float>(Configuration::tileWidth), ppos.y * static_cast<float>(Configuration::tileHeight) };
					xpNum.offsetX = 0.f;
					xpNum.isDeath = false;
					xpNum.maxTimer = 1.2f;
					floatingNumbers.push_back(xpNum);
				});
			}
		}
	});

	for (auto entity: entitiesHealth)
	{
		entity.set<DeleteMe>({});
	}

	const auto accumFilter = ECS::getWorld().filter<DamageAccumulator>();
	accumFilter.each([&](flecs::entity entity, DamageAccumulator& accum) {
		if (accum.update(Window::GetFrameTime())) {
			FloatingNumberData num;
			num.text = Helpers::TextFormat("%.0f", accum.consume());
			num.color = WHITE;
			num.worldPixelPos = accum.spawnPos;
			num.offsetX = accum.offsetX;
			num.isDeath = false;
			num.maxTimer = 1.0f;
			floatingNumbers.push_back(num);
		}
	});

}

void GameState::update()
{
	/*for (const auto& _player : players) {

		_player->update(dungeon);
		for (const auto& _powerup : powerups) {
			if (doCirclesOverlap(_powerup->pos.x, _powerup->pos.y, _powerup->radius,
				_player->pos.x, _player->pos.y, _player->radius)) {
				_powerup->handlePowerUp(_player);
			}
		}
	}*/

	Particles::Update(static_cast<float>(Window::GetFrameTime()));

	for (const auto& _spawner : spawners) {
		_spawner->spawnTimer -= (float)(Window::GetFrameTime() / Configuration::slowMotionFactor);
		if (_spawner->spawnTimer <= 0.0f && _spawner->spawnsLeft > 0 && _spawner->spawnsLeft <= _spawner->maxSpawns) {
			CreateLizardEntity(_spawner->pos + 2.f, 5 * Helpers::getDifficultyModifier(Configuration::gameTime));

			_spawner->spawnTimer = 2.0f;
			_spawner->spawnsLeft--;
		}
	}
}

void GameState::handleInput()
{
	//only for global input
	if (Input::KeyPressed(SDLK_LEFTBRACKET)) {
		Camera::SetScale( Camera::GetScale() / 2.0f);
		if (Camera::GetScale() < 0.5f) {
			Camera::SetScale( 0.5f);
		}
	}

	if (Input::KeyPressed(SDLK_RIGHTBRACKET)) {
		Camera::SetScale( Camera::GetScale() * 2.0f);
	}

	if (Input::KeyPressed(SDLK_M))
	{
		miniMapVisible = !miniMapVisible;
	}

	if (Input::KeyPressed(SDLK_F1)) {
		/*if (players.size() == 1) {
			players.emplace_back(new Wizard());

			players[1]->pos = players[0]->pos + vf2d({ 2.5f, 0.5f });
			players[1]->load();

			World::addObject(players[1]);

			setupControls();
		}*/
	}
}

void GameState::drawUI()
{
	// UI
	const auto playerFilter = ECS::getWorld().filter<PlayerIndex>();
	playerFilter.each([&](flecs::entity entity, PlayerIndex index) {
        UI::DrawPlayerBar(entity);
	});

	UI::drawTimerBar();

	if (miniMapVisible) UI::DrawMiniMap();
	if (Configuration::showGameStats) { WLLog::resetLog(); UI::drawDebugInfo(); }

	if (Input::KeyPressed(SDLK_F10)) {
		World::clearObjects();
		World::generateNewMap();

		resetGame();

		// Rooms were replaced — these pointers are dangling; null them before drawUI continues
		currentRoom = nullptr;
		lastRoom    = nullptr;
	}

	if (Input::KeyPressed(SDLK_F11) && !Input::KeyDown(SDLK_LSHIFT)) {
        Configuration::showGameStats = !Configuration::showGameStats;
    }


	if (!roomTitleLerp->isFinished() && currentRoom) {

		Color titleColor = { 255,255,255,255 };


		int textWidth = Text::MeasureText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 40), currentRoom->name);

		float newAlpha = 0.0f;

		if (roomTitleLerp->getValue() <= 1.0f) {
			newAlpha = roomTitleLerp->getValue();
		}
		else if (roomTitleLerp->getValue() < 7.0f) {
			newAlpha = 1.f;
		}
		else {
			newAlpha = (3.f - (roomTitleLerp->getValue() - 7.f)) / 3.f;
		}

		titleColor.a = static_cast<int>(newAlpha * 255.f);

		Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 40), {((float)Window::GetWidth() / 2.f) - ((float)textWidth / 2.f), 100.f,}, currentRoom->name.c_str(), titleColor);
	}
}

void GameState::resetPuzzle(int roomId) {
    if (puzzleSolvedRooms.count(roomId)) return;
    if (!puzzleInitialState.count(roomId)) return;

    // Destroy all current blocks for this room (including any fading ones)
    std::vector<flecs::entity> toDelete;
    ECS::getWorld().filter<PuzzleBlock>().each([&](flecs::entity e, PuzzleBlock& pb) {
        if (pb.roomId == roomId) toDelete.push_back(e);
    });
    for (auto& e : toDelete) ECS::removeEntity(&e);

    // Re-create blocks at their original spawn positions with a fade-in
    for (const auto& [tilePos, colorIdx] : puzzleInitialState[roomId])
        CreatePuzzleBlockEntity(tilePos, colorIdx, roomId, true);

    puzzleResetTimers[roomId] = 0.f;

    // Force all plates for this room to unpressed so the player must step off
    // and back on before they can trigger another reset.
    ECS::getWorld().filter<PressurePlate>().each([&](flecs::entity, PressurePlate& pp) {
        if (pp.roomId == roomId) pp.isPressed = false;
    });
}

void GameState::checkPuzzleMatches(int roomId) {
    struct BlockInfo {
        flecs::entity entity;
        int gx, gy, colorIdx;
    };

    // Collect only settled, non-fading blocks in this room
    std::vector<BlockInfo> blocks;
    const auto pf = ECS::getWorld().filter<PuzzleBlock, Pushable, RigidBody2D>();
    pf.each([&](flecs::entity e, PuzzleBlock& pb, Pushable& p, RigidBody2D& rb) {
        if (pb.roomId != roomId || !p.settled || pb.fadingOut) return;
        b2Vec2 pos = b2Body_GetPosition(rb.RigidBody);
        int gx = (int)roundf((pos.x - 0.5f) / (float)PUZZLE_GRID_STEP);
        int gy = (int)roundf((pos.y - 0.5f) / (float)PUZZLE_GRID_STEP);
        blocks.push_back({ e, gx, gy, pb.colorIdx });
    });

    if (blocks.empty()) return;

    std::set<flecs::entity_t> toDelete;

    // checkLine: assumes input is already sorted by the axis being checked (gx for rows, gy for cols)
    // coord[i] is the sorted coordinate value for block i
    auto checkLine = [&](std::vector<BlockInfo*>& line, auto getCoord) {
        for (int i = 0; i < (int)line.size(); ) {
            int j = i + 1;
            while (j < (int)line.size()
                   && line[j]->colorIdx == line[i]->colorIdx
                   && getCoord(line[j]) - getCoord(line[i]) == j - i)
                ++j;
            if (j - i >= 3)
                for (int k = i; k < j; ++k) toDelete.insert(line[k]->entity.id());
            i = j;
        }
    };

    std::map<int, std::vector<BlockInfo*>> byRow, byCol;
    for (auto& b : blocks) { byRow[b.gy].push_back(&b); byCol[b.gx].push_back(&b); }

    for (auto& [gy, row] : byRow) {
        std::sort(row.begin(), row.end(), [](BlockInfo* a, BlockInfo* b) { return a->gx < b->gx; });
        checkLine(row, [](BlockInfo* b) { return b->gx; });
    }
    for (auto& [gx, col] : byCol) {
        std::sort(col.begin(), col.end(), [](BlockInfo* a, BlockInfo* b) { return a->gy < b->gy; });
        checkLine(col, [](BlockInfo* b) { return b->gy; });
    }

    if (toDelete.empty()) return;

    // Mark matched blocks as fading (don't delete immediately)
    pf.each([&](flecs::entity e, PuzzleBlock& pb, Pushable& p, RigidBody2D& rb) {
        if (pb.roomId == roomId && toDelete.count(e.id()) && !pb.fadingOut) {
            pb.fadingOut = true;
            pb.fadeTimer = 0.f;
            p.settled    = false;
            b2Body_SetLinearVelocity(rb.RigidBody, { 0.f, 0.f });
        }
    });

    // Check if all non-fading blocks are cleared
    int remaining = 0;
    pf.each([&](flecs::entity, PuzzleBlock& pb, Pushable&, RigidBody2D&) {
        if (pb.roomId == roomId && !pb.fadingOut) remaining++;
    });

    if (remaining == 0) {
        puzzleSolvedRooms.insert(roomId);

        // Open all gates for this room
        std::vector<flecs::entity> gateEntities;
        const auto gateFilter = ECS::getWorld().filter<GateEntity>();
        gateFilter.each([&](flecs::entity e, GateEntity& gate) {
            if (gate.roomId == roomId && gate.closed) gateEntities.push_back(e);
        });
        for (auto& e : gateEntities) e.get_mut<GateEntity>()->open(e);

        // Remove the room sensor so re-entering won't close gates again
        std::vector<flecs::entity> sensorEntities;
        const auto sensorFilter = ECS::getWorld().filter<RoomSensorEntity>();
        sensorFilter.each([&](flecs::entity e, RoomSensorEntity& rs) {
            if (rs.roomId == roomId) sensorEntities.push_back(e);
        });
        for (auto& e : sensorEntities) ECS::removeEntity(&e);
    }
}

void GameState::checkMirrorPuzzle(int roomId) {
    if (mirrorPuzzleSolvedRooms.count(roomId)) return;

    // Count receptors; all must be lit
    int total = 0, lit = 0;
    ECS::getWorld().filter<BeamReceptor>().each([&](flecs::entity, BeamReceptor& br) {
        if (br.roomId != roomId) return;
        ++total;
        if (br.isLit) ++lit;
    });

    if (total == 0 || lit < total) return;

    mirrorPuzzleSolvedRooms.insert(roomId);

    std::vector<flecs::entity> gateEntities;
    ECS::getWorld().filter<GateEntity>().each([&](flecs::entity e, GateEntity& gate) {
        if (gate.roomId == roomId && gate.closed) gateEntities.push_back(e);
    });
    for (auto& e : gateEntities) e.get_mut<GateEntity>()->open(e);

    std::vector<flecs::entity> sensorEntities;
    ECS::getWorld().filter<RoomSensorEntity>().each([&](flecs::entity e, RoomSensorEntity& rs) {
        if (rs.roomId == roomId) sensorEntities.push_back(e);
    });
    for (auto& e : sensorEntities) ECS::removeEntity(&e);
}
