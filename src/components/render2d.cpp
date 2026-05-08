#include "render2d.h"
#include "configuration.h"

#include "rigidbody2d.h"
#include "sprite.h"
#include "playerclass.h"
#include "renderframe.h"
#include "spriteeffect.h"
#include "luminoveau.h"

#include "utils/vectors.h"


void Render2DComp::draw(flecs::entity* entity)
{

	if (entity->has<Sprite>()) {

		auto* sprite = entity->get_mut<Sprite>();
		auto* rigidBody = entity->get_mut<RigidBody2D>();

		int renderFrame = 0;

        [[maybe_unused]]auto _getRectangle = [&](int x, int y) {
            return rectf( static_cast<float>(x * Configuration::tileWidth), static_cast<float>(y * Configuration::tileHeight), static_cast<float>(Configuration::tileWidth), static_cast<float>(Configuration::tileHeight) );
        };

        [[maybe_unused]]auto _getTile = [&](int tileId) {
            return _getRectangle(tileId % 16, (int)tileId / 16);
        };

        [[maybe_unused]]auto _getTileEx = [&](int tileId, bool doubleHeight) {
            rectf r = _getRectangle(tileId % 16, (int)tileId / 16);
            if (doubleHeight) {
                r.y -= r.height;
                r.height *= 2;
            }
            return r;
        };

		if (entity->has<PlayerClass>())
		{
			auto* playerClass = entity->get_mut<PlayerClass>();

			renderFrame = playerClass->getRenderFrame(entity);

		} else if (entity->has<RenderFrame>())
        {
            auto* renderFrameComp = entity->get_mut<RenderFrame>();

			renderFrame = renderFrameComp->RenderFrameNumber;
        }

		/*if (hitTimer > 0.0f) {
			renderFrame = 24;
		}*/

		const b2Vec2 b2pos = b2Body_GetPosition(rigidBody->RigidBody);
		const vf2d pos = {b2pos.x, b2pos.y};

		const rectf position = {
			pos.x * static_cast<float>(Configuration::tileWidth) - sprite->originX,
			pos.y * static_cast<float>(Configuration::tileHeight) - sprite->originY,
			sprite->width,
			sprite->height
		};

		rectf source;
		if (sprite->multiSheet) {
			source = _getTileEx(renderFrame, sprite->doubleHeight);
		}
		else
		{
			source = { 0.f, 0.f, sprite->width, sprite->height };
		}

		if (sprite->facing == direction::WEST) {
			source.width *= -1.0f;
		}

        bool hasEffect = entity->has<SpriteEffect>();
        if (hasEffect) Draw::SetEffect(entity->get_mut<SpriteEffect>()->effect);
        Draw::TexturePart(sprite->sprite, {position.x, position.y}, {position.width, position.height}, source);
        if (hasEffect) Draw::ClearEffects();

		// debug for body center and sprite size
		//DrawRectangleRec(position, { 255,255,255,64 });
		//DrawCircleV(pos * 32, 2.f, { 255,0,0,128 });
		//DrawCircleV(pos * 32, rigidBody->radius * 32.f, { 255,0,0,64 });
	}
}
