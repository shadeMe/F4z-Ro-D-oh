#include "DialogueResponse.h"

namespace RE
{
	void DialogueResponse::_Ctor(TESTopic* topic, TESTopicInfo* topicInfo, TESObjectREFR* speaker, Override::TESResponse* response, TESQuest* quest)
	{
		using func_t = decltype(&DialogueResponse::_Ctor);
		static REL::Relocation<func_t> func{ Offsets::Ctor.address() };
		func(this, topic, topicInfo, speaker, response, quest);
	}
}
