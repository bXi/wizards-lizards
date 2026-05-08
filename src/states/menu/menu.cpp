
#include <utils/helpers.h>
#include "menu.h"
#include "configuration.h"
#include "settings/settingshandler.h"
#include "state/state.h"

void MenuState::load()
{

	AssetHandler::GetMusic("assets/music/menu.mp3");
//
	AssetHandler::GetSound("assets/sfx/cursor.wav");
	AssetHandler::GetSound("assets/sfx/confirm.wav");

	mo["main"].SetTable(1, 6);
	mo["main"]["Start game"].setCallback([&]() { State::SetState("lobby"); });

	mo["main"]["Options"].SetTable(1, 6);

	auto& videoSettings = mo["main"]["Options"]["Video"].SetTable(1, 10);

	mo["main"]["Options"]["Video"]["Resolution"].SetTable(1, 10);

	const auto modes = Settings::resolutions();

	for (auto& res : modes) {

		int width = res.first;
		int height = res.second;

		auto name = std::string(Helpers::TextFormat("%i x %i", width, height));

		if (Window::GetHeight() == height && Window::GetWidth() == width) {
			videoSettings["Resolution"][name.c_str()].setActive(true);
		}
		videoSettings["Resolution"][name.c_str()].setCallback([width, height, name, &mo = mo]() {
			Settings::setRes(width, height);
			mo["main"]["Options"]["Video"]["Resolution"].clearActive();
			mo["main"]["Options"]["Video"]["Resolution"][name.c_str()].setActive(true);
			});
	}

	mo["main"]["Options"]["Video"]["Fullscreen"].setCallback([&mo = mo]() {

		Window::ToggleFullscreen();
		mo["main"]["Options"]["Video"]["Fullscreen"].setActive(Window::IsFullscreen());
		}).setActive(Window::IsFullscreen());

    mo["main"]["Options"]["Video"]["VSync"].setCallback([&mo = mo]() {

			Settings::toggleVsync();
			mo["main"]["Options"]["Video"]["VSync"].setActive(Settings::getVsync());
			}).setActive(Settings::getVsync());

    mo["main"]["Options"]["Audio"].SetTable(1, 3);
    mo["main"]["Options"]["Audio"]["Master volume"].SetTable(1, 3);
    mo["main"]["Options"]["Audio"]["Master volume"][getVolumeBar(Settings::getMasterVolume()).c_str()].SetInternalName("masterVolume").SetTable(1, 1);

    mo["main"]["Options"]["Audio"]["Music volume"].SetTable(1, 3);
    mo["main"]["Options"]["Audio"]["Music volume"][getVolumeBar(Settings::getMusicVolume()).c_str()].SetInternalName("musicVolume").SetTable(1, 1);

    mo["main"]["Options"]["Audio"]["Effects volume"].SetTable(1, 3);
    mo["main"]["Options"]["Audio"]["Effects volume"][getVolumeBar(Settings::getSoundVolume()).c_str()].SetInternalName("soundVolume").SetTable(1, 1);

    mo["main"]["Debug"].setCallback([&]() { State::SetState("test"); });
    mo["main"]["Quit"].setCallback([&]() { State::SetState("quit"); });

    mo.Build();

    mm.Open(&mo["main"]);

    Audio::PlayMusic(AssetHandler::GetMusic("assets/music/menu.mp3"));



}

void MenuState::unload()
{
	Audio::StopMusic();

}

void MenuState::draw()
{
//	if (!Audio::IsMusicPlaying()) {
//		Audio::PlayMusic("assets/music/menu.mp3");
//	}

	const int leftMargin = Window::GetWidth() / 10;
	const int topMargin = Window::GetHeight() / 10;

    Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {(float)leftMargin, (float)topMargin}, "Wizards & Lizards", WHITE);

	menuobject* command = nullptr;

	bool doVolume = false;

	float volumeModifier = 0.0f;

	auto inputs = Input::GetAllInputs();

	for (auto& input : inputs)
	{
		if (input->is(Buttons::UP,     Action::PRESSED)) { Audio::PlaySound(AssetHandler::GetSound("assets/sfx/cursor.wav"));  mm.OnUp(); }
		if (input->is(Buttons::DOWN,   Action::PRESSED)) { Audio::PlaySound(AssetHandler::GetSound("assets/sfx/cursor.wav"));  mm.OnDown(); }
		if (input->is(Buttons::LEFT,   Action::PRESSED)) { Audio::PlaySound(AssetHandler::GetSound("assets/sfx/cursor.wav"));  doVolume = true; }
		if (input->is(Buttons::RIGHT,  Action::PRESSED)) { Audio::PlaySound(AssetHandler::GetSound("assets/sfx/cursor.wav"));  doVolume = true; }
		if (input->is(Buttons::ACCEPT, Action::PRESSED)) { Audio::PlaySound(AssetHandler::GetSound("assets/sfx/confirm.wav")); command = mm.OnConfirm(); }
		if (input->is(Buttons::BACK,   Action::PRESSED)) { Audio::PlaySound(AssetHandler::GetSound("assets/sfx/cursor.wav"));  mm.OnBack(); }

		volumeModifier = input->is(Buttons::LEFT, Action::PRESSED) ? -0.05f : 0.05f;
	}




	if (doVolume) {
		command = mm.OnConfirm();

		if (command != nullptr) {
			const std::string name = command->GetInternalName();

			if (name == "masterVolume")
			{
				Settings::setMasterVolume(Settings::getMasterVolume() + volumeModifier);
				command->SetName(getVolumeBar(Settings::getMasterVolume()));
			}
			else if (name == "soundVolume")
			{
				Settings::setSoundVolume(Settings::getSoundVolume() + volumeModifier);
				command->SetName(getVolumeBar(Settings::getSoundVolume()));
			}
			else if (name == "musicVolume")
			{
				Settings::setMusicVolume(Settings::getMusicVolume() + volumeModifier);
				command->SetName(getVolumeBar(Settings::getMusicVolume()));
			}

		}
	}

	int baseX = 20;
	int baseY = 300;

	if (Configuration::showFPS) {
        Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {(float)baseX, (float)baseY}, Helpers::TextFormat("%d", Window::GetFPS()), GREEN);
	}

	if (command != nullptr)
	{
		command->callBack();
	}

	mm.Draw({ leftMargin, topMargin + 144 });
}

std::string MenuState::getVolumeBar(float volume)
{
	constexpr int maxVolume = 20;

	std::string text = "0% [" + std::string(maxVolume, '-') + "] 100%";// " (" + Helpers::TextFormat("%i", static_cast<int>(volume * 100)) + ")";

	const int barPos = static_cast<int>(volume * maxVolume);

	text[std::max(4, barPos+3)] = '|';

	return text;

}
