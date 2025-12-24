#pragma once

#include "BSPointerHandleRef.h"
#include "BSStringPool.h"
#include "TESResponse.h"

namespace RE
{
	class TESTopic;
	class TESTopicInfo;
	class TESObjectREFR;
	class BGSKeyword;
	class TESIdleForm;
	class BGSSoundDescriptorForm;
	class TESQuest;

	// 40
	class DialogueResponse
	{
		void _Ctor(TESTopic* topic, TESTopicInfo* topicInfo, TESObjectREFR* speaker, Override::TESResponse* response, TESQuest* quest);

	public:
		// members
		/*00*/ Override::BSFixedStringCS responseText;
		/*08*/ BGSKeyword* animFace;
		/*10*/ std::uint16_t uiPercent;
		/*12*/ std::uint8_t pad12[6];
		/*18*/ Override::BSFixedString voiceFilePath;
		/*20*/ TESIdleForm* speakerIdle;
		/*28*/ TESIdleForm* listenerIdle;
		/*30*/ BGSSoundDescriptorForm* pVoiceSound;
		/*38*/ std::uint8_t useEmotionAnim;
		/*39*/ std::uint8_t hasLipFile;
		/*3A*/ std::uint8_t endOnSceneEnd;
		/*3B*/ std::uint8_t pad3B[5];

		struct Offsets
		{
			// E8 ? ? ? ? 48 8B F0 EB 03 49 8B F5 44 8B 43 08
			inline static const REL::Relocation<std::uintptr_t> Ctor{ REL::ID(2227131) };
		};
	};

	static_assert(sizeof(DialogueResponse) == 0x40);
}
