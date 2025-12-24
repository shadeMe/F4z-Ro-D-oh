#pragma once

namespace Config
{
	static REX::INI::U32 WordsPerSecondSilence{ "General", "uWordsPerSecondSilence", 2u };
	static REX::INI::U32 WideCharactersPerWord{ "General", "uWideCharactersPerWord", 3u };
	static REX::INI::U32 MinimumSecondsOfSilence{ "General", "uMinimumSecondsOfSilence", 2u };
	static REX::INI::Bool SkipEmptyResponses{ "General", "bSkipEmptyResponses", true };
	static REX::INI::Bool ForceSilentVoice{ "General", "bForceSilentVoice", false };
	static REX::INI::Bool LogVerbose{ "Debug", "bLogVerbose", false };

	void Load();
}
