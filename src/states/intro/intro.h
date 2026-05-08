#pragma once

#include <algorithm>
#include "luminoveau.h"

#include "utils/lerp.h"
#include <state/basestate.h>


class IntroState : public BaseState {
	Texture logo_raylib;
	float grey = 0.0f;
	float intro_logo_time = 0.2f;
	float total_intro_time = 4.0f;

	float gameTime = 0.0f;
	LerpAnimator* firstLogoLerp = nullptr;
	public:
		
		void load() override;
		void unload() override;
		void draw() override;
};
