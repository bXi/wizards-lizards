#include "wizard.h"
#include "configuration.h"
#include <algorithm>
#include <set>

#include "components/health.h"
#include "components/damageaccumulator.h"
#include "entities/fireball.h"
#include "entities/timewarp.h"
#include "components/timewarpdata.h"
#include "entities/fragment.h"
#include "components/fragmentdata.h"
#include "entities/puzzleblock.h"
#include "entities/mirrorblock.h"
#include "components/pushable.h"


#include "extra/LightLayer.h"


void WizardClass::update()
{
	const float delta = (Window::GetFrameTime() / Configuration::slowMotionFactor);

	//spriteTimer += delta;
	secondsSinceLastShot -= delta;
	laserLastDamageTick -= delta;
	secondsSinceLastWarp -= delta;
	secondsSinceLastFragment -= delta;
	hitTimer -= delta;

	if (m_laserParticlesInited) {
		for (auto& h : m_laserHitParticles) {
			Particles::Stop(h);
			Particles::QueueDraw(h);
		}
	}
}

void WizardClass::doDamage(flecs::entity* entity, int weaponId)
{
	
}


void WizardClass::shoot(flecs::entity entity)
{

	auto* player = entity.get_mut<PlayerInput>();
	auto* rigidBody2d = entity.get_mut<RigidBody2D>();


	auto dungeon = World::getDungeon();

	b2Vec2 pos = b2Body_GetPosition(rigidBody2d->RigidBody);
	auto aim = player->aim;

	if (m_laserParticlesInited) {
		for (auto& h : m_laserHitParticles)
			Particles::Stop(h);
	}

	switch (player->selectedWeapon) {
	case 1:
	{
		if (secondsSinceLastShot <= 0.0f) {

			auto [spread, damage, speed] = player->getPowerUp(0);

			secondsSinceLastShot = 0.2f - (static_cast<float>(speed) / 200.0f);



			auto shootFireball = [&](const vf2d &_aim)
			{
				CreateFireballEntity(&entity, vf2d{pos.x - 0.5f, pos.y - 0.5f}, (_aim / vi2d{ Configuration::tileWidth, Configuration::tileHeight }));
			};

			spread++;
			float degreesBetweenBullets = (PI / 180) * 3; // 7 degrees
			bool evenShots = !(spread % 2); //false
			int halfArc = spread / 2; //3

			vf2d aimright = aim;
			vf2d aimleft = aim;


			const auto halfArcOffset = static_cast<float>(halfArc + 1);

			if (evenShots) {
				aimleft.rotateBy(-((degreesBetweenBullets) * halfArcOffset) + (degreesBetweenBullets / 2.0f));
				aimright.rotateBy(((degreesBetweenBullets) * halfArcOffset) - (degreesBetweenBullets / 2.0f));
			}
			else {
				aimleft.rotateBy(-(degreesBetweenBullets) * halfArcOffset);
				aimright.rotateBy((degreesBetweenBullets) * halfArcOffset);
			}

			for (int shot = halfArc; shot > 0; shot--) {

				aimleft.rotateBy(degreesBetweenBullets);
				shootFireball(aimleft);

				aimright.rotateBy(-degreesBetweenBullets);
				shootFireball(aimright);

			}

			if (!evenShots) {
				shootFireball(aim);
			}

			player->shooting = false;
		}
	}
	break;
	case 2:
	{
		auto [damage, bounces, beamwidth] = player->getPowerUp(1);

		// Solid stops: color puzzle blocks
		std::set<std::pair<int,int>> blockTiles;
		ECS::getWorld().filter<PuzzleBlock, Pushable, RigidBody2D>().each([&](flecs::entity, PuzzleBlock& pb, Pushable& pp, RigidBody2D& brb) {
			if (pb.fadingOut) return;
			b2Vec2 bp = b2Body_GetPosition(brb.RigidBody);
			int tx = pp.isMoving ? (int)roundf(pp.targetPos.x - 0.5f) : (int)roundf(bp.x - 0.5f);
			int ty = pp.isMoving ? (int)roundf(pp.targetPos.y - 0.5f) : (int)roundf(bp.y - 0.5f);
			blockTiles.insert({tx, ty});
		});

		// Mirrors: reflect beam, don't count as wall bounces
		std::map<std::pair<int,int>, MirrorBlock::Orientation> mirrorMap;
		ECS::getWorld().filter<MirrorBlock, Pushable, RigidBody2D>().each([&](flecs::entity, MirrorBlock& mb, Pushable& mp, RigidBody2D& brb) {
			b2Vec2 bp = b2Body_GetPosition(brb.RigidBody);
			int tx = mp.isMoving ? (int)roundf(mp.targetPos.x - 0.5f) : (int)roundf(bp.x - 0.5f);
			int ty = mp.isMoving ? (int)roundf(mp.targetPos.y - 0.5f) : (int)roundf(bp.y - 0.5f);
			mirrorMap[{tx, ty}] = mb.orientation;
		});

		// Receptors: light up on hit, beam passes through
		std::map<std::pair<int,int>, flecs::entity> receptorMap;
		ECS::getWorld().filter<BeamReceptor>().each([&](flecs::entity e, BeamReceptor& br) {
			receptorMap[{br.tilePos.x, br.tilePos.y}] = e;
		});

		// Beam blockers: solid interior pillars, stop DDA like walls
		ECS::getWorld().filter<BeamBlocker>().each([&](flecs::entity, BeamBlocker& bb) {
			blockTiles.insert({bb.tilePos.x, bb.tilePos.y});
		});

		// DDA helper: march from start in dir until wall/block/mirror/receptor.
		// Returns {found, distance, lastHitX, hitTile}.
		// Mirrors and receptors are found via mirrorMap/receptorMap checks inside loop.
		enum HitType { HIT_WALL, HIT_MIRROR, HIT_RECEPTOR };
		auto doDDA = [&](vf2d start, vf2d dir) -> std::tuple<bool, float, bool, vi2d, HitType> {
			vf2d stepsize = {
				sqrtf(1.f + (dir.y / dir.x) * (dir.y / dir.x)),
				sqrtf(1.f + (dir.x / dir.y) * (dir.x / dir.y))
			};
			vi2d mc = { (int)start.x, (int)start.y };
			vf2d rLen;
			vi2d st;
			if (dir.x < 0) { st.x = -1; rLen.x = (start.x - mc.x) * stepsize.x; }
			else            { st.x =  1; rLen.x = (mc.x + 1 - start.x) * stepsize.x; }
			if (dir.y < 0) { st.y = -1; rLen.y = (start.y - mc.y) * stepsize.y; }
			else            { st.y =  1; rLen.y = (mc.y + 1 - start.y) * stepsize.y; }

			bool found = false, lastHitX = false;
			float dist = 0.f;
			HitType hitType = HIT_WALL;

			while (!found && dist < 250.f) {
				if (rLen.x < rLen.y) { mc.x += st.x; dist = rLen.x; rLen.x += stepsize.x; lastHitX = true; }
				else                  { mc.y += st.y; dist = rLen.y; rLen.y += stepsize.y; lastHitX = false; }

				if (mc.x < 0 || mc.x >= dungeon.getWidth() || mc.y < 0 || mc.y >= dungeon.getHeight()) break;

				if (mirrorMap.count({mc.x, mc.y})) {
					found = true; hitType = HIT_MIRROR;
				} else if (receptorMap.count({mc.x, mc.y})) {
					found = true; hitType = HIT_RECEPTOR;
				} else if (dungeon.getTile(mc.x, mc.y) == Tile::WALL ||
				           dungeon.getTile(mc.x, mc.y) == Tile::VOID ||
				           blockTiles.count({mc.x, mc.y})) {
					found = true; hitType = HIT_WALL;
				}
			}
			return { found, dist, lastHitX, mc, hitType };
		};

		if (!m_laserParticlesInited) {
			static constexpr const char* kLaserHitPreset =
				"djE6NTA2LDAsMTAzLDAsODUsMCwwLDAsMC4xLDcsNC43NywwLjUsNSwxLjUsMC41LDIwLDMuODYs"
				"MCwwLjM3LDAsMCwxLDAuODYsMCwwLDAuOSwwLjgsMC4wNSwwLDAuNCwwLjMsMCwwLDAsMCwwLjE1"
				"LDAuNSwx";
			for (auto& h : m_laserHitParticles) {
				h = Particles::CreateSystemFromPreset(kLaserHitPreset, 500);
				Particles::Start(h);
			}
			m_laserHitCfg = Particles::GetConfig(m_laserHitParticles[0]);
			m_laserParticlesInited = true;
		}
		m_wallHits.clear();
		vecLines.clear();
		float radius = 1.0f * (4.0f * (1.0f + static_cast<float>(beamwidth))) / 32.0f;

		vf2d startPosition = { pos.x - 0.5f, pos.y - 0.5f };
		// Nudge off exact tile boundaries so the DDA never produces a zero-length first segment
		// (when starting exactly on a boundary the initial rLen for the negative direction is 0,
		//  giving hitPos == startPosition which causes ThickLine to receive a zero-length segment)
		constexpr float DDA_EPS = 1e-4f;
		if (std::fabs(startPosition.x - std::floor(startPosition.x)) < DDA_EPS) startPosition.x += DDA_EPS;
		if (std::fabs(startPosition.y - std::floor(startPosition.y)) < DDA_EPS) startPosition.y += DDA_EPS;
		vf2d rayDir = aim.norm();
		// Unified bounce loop: mirrors are free, wall bounces cost from the `bounces` budget.
		int wallBouncesLeft = bounces;
		int totalIter = 0;
		constexpr int MAX_ITER = 24;

		while (totalIter < MAX_ITER) {
			auto [found, dist, lastHitX, hitTile, hitType] = doDDA(startPosition, rayDir);
			if (!found) break;

			vf2d hitPos = startPosition + rayDir * dist;

			if (hitType == HIT_RECEPTOR) {
				receptorMap[{hitTile.x, hitTile.y}].get_mut<BeamReceptor>()->isLit = true;
				// Draw segment ending at receptor center
				vf2d recCenter = { hitTile.x + 0.5f, hitTile.y + 0.5f };
				vecLines.push_back({ startPosition.x, startPosition.y, recCenter.x, recCenter.y, radius });
				// Continue from just past the receptor's far tile boundary so doDDA
				// doesn't re-find the same receptor on the next call
				startPosition = { hitTile.x + 0.5f + rayDir.x * (0.5f + 1e-3f),
				                  hitTile.y + 0.5f + rayDir.y * (0.5f + 1e-3f) };
				++totalIter;
				continue;
			}

			vecLines.push_back({ startPosition.x, startPosition.y, hitPos.x, hitPos.y, radius });

			vf2d newDir;
			if (hitType == HIT_MIRROR) {
				// Mirror reflection — free, uses correct diagonal normal
				auto ori = mirrorMap[{hitTile.x, hitTile.y}];
				vf2d mirNormal = (ori == MirrorBlock::Orientation::Slash)
				    ? vf2d{ 1.f,  1.f }.norm()
				    : vf2d{ 1.f, -1.f }.norm();
				newDir = rayDir.reflectOn(mirNormal);
			} else {
				// Wall / solid block — consumes a bounce
				vf2d wallNormal = {};
				if (lastHitX) wallNormal.x = rayDir.x > 0 ? -1.f : 1.f;
				else          wallNormal.y = rayDir.y > 0 ? -1.f : 1.f;
				newDir = rayDir.reflectOn(wallNormal);
				m_wallHits.push_back({hitPos, newDir});
				if (wallBouncesLeft <= 0) break;
				--wallBouncesLeft;
			}

			startPosition = hitPos;
			rayDir        = newDir;
			++totalIter;
		}

		LightLayer::StartLight();
		for (auto& edge : vecLines) {

			vf2d start = { edge.sx * static_cast<float>(Configuration::tileWidth), edge.sy * static_cast<float>(Configuration::tileHeight )};
			vf2d end = { edge.ex * static_cast<float>(Configuration::tileWidth), edge.ey * static_cast<float>(Configuration::tileHeight )};

			float startAngle = 360.0f - (vf2d(edge.sx, edge.sy).getAngle() - vf2d( edge.sx, edge.sy ).getAngle());
			float endAngle = 360.0f - (vf2d(edge.sx, edge.sy).getAngle() - vf2d( edge.ex, edge.ey ).getAngle());

			if (startAngle >= 0.0f && startAngle <= 180.0f) {
				startAngle += 180.0f;
				endAngle += 180.0f;
			}

			float radius = edge.radius * 16.0f;
			Draw::ThickLine(start, end, { 255,100,100,25 }, radius * 2.0f);

		}
		LightLayer::EndLight();
		constexpr float kLaserParticleSpeed = 150.0f;
		const int activeHits = std::min(static_cast<int>(m_wallHits.size()), LASER_PARTICLE_COUNT);
		for (int pi = 0; pi < activeHits; ++pi) {
			Particles::SetPosition(m_laserHitParticles[pi], {
				m_wallHits[pi].pos.x * static_cast<float>(Configuration::tileWidth),
				m_wallHits[pi].pos.y * static_cast<float>(Configuration::tileHeight) + 5.0f,
				0.0f
			});
			m_laserHitCfg.spawnVelocity = {
				m_wallHits[pi].outDir.x * kLaserParticleSpeed,
				m_wallHits[pi].outDir.y * kLaserParticleSpeed,
				0.0f
			};
			m_laserHitCfg.emitRate = 200.0f;
			Particles::UpdateConfig(m_laserHitParticles[pi], m_laserHitCfg);
			Particles::Start(m_laserHitParticles[pi]);
		}
		for (int pi = activeHits; pi < LASER_PARTICLE_COUNT; ++pi) {
			m_laserHitCfg.emitRate = 0.0f;
			Particles::UpdateConfig(m_laserHitParticles[pi], m_laserHitCfg);
			Particles::Stop(m_laserHitParticles[pi]);
		}
		if (laserLastDamageTick <= 0.0f) {
			for (auto& edge : vecLines) {

				const auto enemyFilter = ECS::getWorld().filter<EnemyEntity>();
				enemyFilter.each([edge,damage](flecs::entity entity, EnemyEntity enemyEntity) {
					auto* rigidBody2d = entity.get<RigidBody2D>();
					auto* health = entity.get_mut<Health>();

					const float fLineX1 = edge.ex - edge.sx;
					const float fLineY1 = edge.ey - edge.sy;

					b2Vec2 enemyPos = b2Body_GetPosition(rigidBody2d->RigidBody);
					const float fLineX2 = enemyPos.x - edge.sx;
					const float fLineY2 = enemyPos.y - edge.sy;

					const float fEdgeLength = fLineX1 * fLineX1 + fLineY1 * fLineY1;

					const float t = std::max(0.0f, std::min(fEdgeLength, (fLineX1 * fLineX2 + fLineY1 * fLineY2))) / fEdgeLength;

					const float fClosestPointX = edge.sx + t * fLineX1;
					const float fClosestPointY = edge.sy + t * fLineY1;

					const float fDistance = sqrtf((enemyPos.x - fClosestPointX) * (enemyPos.x - fClosestPointX) + (enemyPos.y - fClosestPointY) * (enemyPos.y - fClosestPointY));

					if (fDistance <= (rigidBody2d->radius + edge.radius)) {
						if (entity.has<DamageAccumulator>()) {
							entity.get_mut<DamageAccumulator>()->add(float(2 * (damage + 1)), {enemyPos.x * Configuration::tileWidth, enemyPos.y * Configuration::tileHeight});
						}
						health->currentHealth -= (2 * (damage + 1));
					}

				});

				laserLastDamageTick = 0.12f;
			}
		}

	}
	break;
	case 3:
	{
		if (secondsSinceLastWarp <= 0.0f) {
			const int upgrades = player->weaponUpgrades[2];

			// Slow scaling: need 9 upgrades per extra warp
			const int   maxAmount  = 1 + upgrades / 9;
			// Radius: grows 0.05 tiles per upgrade (need 20 upgrades to add 1 tile)
			const float radius     = 3.0f + upgrades * 0.05f;
			// Duration: grows 0.1s per upgrade
			const float duration   = 4.0f + upgrades * 0.1f;
			// Slow factor is fixed — upgrade the count/size, not the intensity
			constexpr float slowFactor = 0.15f;

			// Enforce per-player cap: remove oldest warp if at max
			std::vector<flecs::entity> myWarps;
			const auto existingWarps = ECS::getWorld().filter<TimeWarpData>();
			existingWarps.each([&](flecs::entity we, TimeWarpData& wd) {
				if (wd.ownerId == entity.id()) myWarps.push_back(we);
			});

			while (static_cast<int>(myWarps.size()) >= maxAmount) {
				auto oldest = std::min_element(myWarps.begin(), myWarps.end(),
					[](const flecs::entity& a, const flecs::entity& b) {
						return a.get<TimeWarpData>()->timer < b.get<TimeWarpData>()->timer;
					});
				ECS::removeEntity(&(*oldest));
				myWarps.erase(oldest);
			}

			CreateTimeWarpEntity(vf2d{pos.x - 0.5f, pos.y - 0.5f}, radius, duration, slowFactor, entity.id());
			secondsSinceLastWarp = 5.0f;
			player->shooting = false;
		}
	}
	break;
	case 4:
	{
		if (secondsSinceLastFragment <= 0.0f) {
			const int upgrades = player->weaponUpgrades[3];

			// 1 fragment base, +1 per 4 upgrade levels
			const int maxFragments = 1 + upgrades / 4;

			int currentCount = 0;
			const auto fragCountFilter = ECS::getWorld().filter<FragmentData>();
			fragCountFilter.each([&](flecs::entity, FragmentData& fd) {
				if (fd.ownerId == entity.id()) currentCount++;
			});

			if (currentCount < maxFragments) {
				float maxHp       = 2.f + upgrades / 3.f;
				float maxLifetime = 8.f + upgrades * 0.3f;
				int   maxChains   = 1 + upgrades / 5;
				float chainRange  = 3.f + upgrades * 0.1f;
				float chainDamage = 5.f + upgrades * 0.5f;
				CreateFragmentEntity(vf2d{ pos.x - 0.5f, pos.y - 0.5f }, maxHp, maxLifetime,
				                     maxChains, chainRange, chainDamage, entity.id());
				secondsSinceLastFragment = 0.3f;
			}

			player->shooting = false;
		}
	}
	break;
	default:
		break;
	}
}
