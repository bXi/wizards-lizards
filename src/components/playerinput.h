#pragma once
#include <utils/lerp.h>
#include "configuration.h"
#include "input/inputhandler.h"

#include "utils/vectors.h"

#include "sprite.h"
#include "rigidbody2d.h"
#include "utils/camera.h"

struct PlayerInput
{
	std::vector<InputDevice*> controllers;

	vf2d vel = { 0.f, 0.f };
	vf2d aim = { 0.f, 0.f };

	int selectedWeapon = 1;
	bool shooting = false;
	bool isRunning = false;
	bool upgradeLock = false;

	int weaponUpgrades[4] = { 0,0,0,0 };

    LerpAnimator* weaponSelectLerp = nullptr;
    LerpAnimator* weaponSelectVisibleLerp = nullptr;
    bool moveWeaponLeft = false;

    void init(int index)
    {
        weaponSelectLerp = Lerp::getLerp(Helpers::TextFormat("weaponSelectLerpP%d", index), 0.0f, 1.0f, 0.4f);
        weaponSelectLerp->callback = EaseSineInOut;
        weaponSelectLerp->started = false;

        weaponSelectVisibleLerp = Lerp::getLerp(Helpers::TextFormat("weaponSelectVisibleLerpP%d", index), 0.0f, 4.0f, 2.f);
        weaponSelectVisibleLerp->callback = EaseQuadIn;
        weaponSelectVisibleLerp->started = false;

    }

	void addController(InputDevice* input)
	{
		controllers.emplace_back(input);
	}

	[[nodiscard]] std::tuple<int, int, int> getPowerUp(int weapon) const
	{
		return {
			static_cast<int>(std::ceil(weaponUpgrades[weapon] / 3)),
			static_cast<int>(std::floor((weaponUpgrades[weapon] + 1) / 3)),
			static_cast<int>(std::floor(weaponUpgrades[weapon] / 3))
		};
	}

	void doMovement(flecs::entity* entity)
	{
		isRunning = false;


		vel = { 0.f, 0.f };
		for (const auto& controller : controllers)
		{
			if (controller->is(Buttons::RUN, Action::HELD)) isRunning = true;

			if (!upgradeLock && controller->is(Buttons::SWITCH_NEXT, Action::PRESSED)) {
                selectedWeapon++;
                weaponSelectLerp->time = 0.0f;
                weaponSelectLerp->started = true;
                moveWeaponLeft = false;
                weaponSelectVisibleLerp->time = 0.0f;
                weaponSelectVisibleLerp->started = false;

            }
			if (!upgradeLock && controller->is(Buttons::SWITCH_PREV, Action::PRESSED)) {
                selectedWeapon--;
                weaponSelectLerp->time = 0.0f;
                weaponSelectLerp->started = true;
                moveWeaponLeft = true;
                weaponSelectVisibleLerp->time = 0.0f;
                weaponSelectVisibleLerp->started = false;
            }

			if (selectedWeapon < 1) selectedWeapon = 4;
			if (selectedWeapon > 4) selectedWeapon = 1;

			switch (controller->getType())
			{
			case InputType::MOUSE_KB:
				if (controller->is(Buttons::LEFT, Action::HELD)) vel.x -= 1.0f;
				if (controller->is(Buttons::RIGHT, Action::HELD)) vel.x += 1.0f;
				if (controller->is(Buttons::UP, Action::HELD)) vel.y -= 1.0f;
				if (controller->is(Buttons::DOWN, Action::HELD)) vel.y += 1.0f;
				
				break;
			case InputType::GAMEPAD:
				vel.x += Input::GetGamepadAxisMovement(controller->getGamepadID(), SDL_GAMEPAD_AXIS_LEFTX);
				vel.y += Input::GetGamepadAxisMovement(controller->getGamepadID(), SDL_GAMEPAD_AXIS_LEFTY);
				break;
			}
			if (vel.mag2() > 0)
			{
				auto tempVel = vel.norm();

                if (vel.mag2() > tempVel.mag2()) vel = tempVel;

				if (entity->has<Sprite>())
				{
					auto sprite = entity->get_mut<Sprite>();
					if (vel.x > 0) 
						sprite->facing = direction::EAST;
					else if (vel.x < 0)
						sprite->facing = direction::WEST;

				}

			}
		}
	}

	void doShoot(flecs::entity entity)
	{

		shooting = false;
		auto* rigidBody2d = entity.get<RigidBody2D>();

for (const auto& controller : controllers)
		{
			switch (controller->getType())
			{
			case InputType::MOUSE_KB:

				if (controller->is(Buttons::SHOOT, Action::HELD)) {
					const vf2d mouseScreenPos = Input::GetMousePosition();
					b2Vec2 rb2pos = b2Body_GetPosition(rigidBody2d->RigidBody);
				const vf2d playerScreenPos = Camera::ToScreenSpace({
						rb2pos.x * static_cast<float>(Configuration::tileWidth),
						rb2pos.y * static_cast<float>(Configuration::tileHeight)
						});
					aim = {
						playerScreenPos.x - (mouseScreenPos.x + static_cast<float>(Configuration::tileWidth) * 0.5f),
						playerScreenPos.y - (mouseScreenPos.y + static_cast<float>(Configuration::tileHeight) * 0.5f)
					};
					aim *= -1.0f;
					shooting = true;
				}
				break;
			case InputType::GAMEPAD:
				vf2d _aim = {
                        Input::GetGamepadAxisMovement(controller->getGamepadID(), SDL_GAMEPAD_AXIS_RIGHTX),
                        Input::GetGamepadAxisMovement(controller->getGamepadID(), SDL_GAMEPAD_AXIS_RIGHTY)
                };
				if (_aim.mag2() > 0) {
					aim = _aim;
					shooting = true;
				}
				break;
			}
		}
	}

};
