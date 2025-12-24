#include "SubtitleManager.h"

namespace RE
{
	namespace Override
	{
		void SubtitleManagerEx::ShowSubtitle(TESObjectREFR* Speaker, Override::BSFixedStringCS* Subtitle, TESTopicInfo* Topic, bool SpokenToPlayer)
		{
			using func_t = decltype(&SubtitleManagerEx::ShowSubtitle);
			static REL::Relocation<func_t> func{ Offsets::ShowSubtitle.address() };
			func(this, Speaker, Subtitle, Topic,SpokenToPlayer);
		}
		void SubtitleManagerEx::DisplayNextSubtitle()
		{
			using func_t = decltype(&SubtitleManagerEx::DisplayNextSubtitle);
			static REL::Relocation<func_t> func{ Offsets::DisplayNextSubtitle.address() };
			func(this);
		}	
	}
}
