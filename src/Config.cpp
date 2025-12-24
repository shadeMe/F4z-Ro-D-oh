#include "Config.h"

namespace Config
{
	void Load()
	{
		const auto ini = REX::INI::SettingStore::GetSingleton();
		ini->Init(
			"Data/F4SE/Plugins/F4z-Ro-D'oh.ini",
			"Data/F4SE/Plugins/F4z-Ro-D'oh.ini");
		ini->Load();

		REX::INFO("Loaded INI file");
		REX::INFO("uWordsPerSecondSilence: {}", WordsPerSecondSilence.GetValue());
		REX::INFO("uWideCharactersPerWord: {}", WideCharactersPerWord.GetValue());
		REX::INFO("uMinimumSecondsOfSilence: {}", MinimumSecondsOfSilence.GetValue());
		REX::INFO("bSkipEmptyResponses: {}", SkipEmptyResponses.GetValue());
		REX::INFO("bForceSilentVoice: {}", ForceSilentVoice.GetValue());
		REX::INFO("bLogVerbose: {}", LogVerbose.GetValue());
	}
}
