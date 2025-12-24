#pragma once

#include "SubtitleInfo.h"

namespace RE
{
	namespace Override
	{
		// 50
		class SubtitleManagerEx: public SubtitleManager
		{
		public:
			void ShowSubtitle(TESObjectREFR* Speaker, Override::BSFixedStringCS* Subtitle, TESTopicInfo* Topic, bool SpokenToPlayer);
			void DisplayNextSubtitle();

			struct Offsets
			{
				inline static const REL::Relocation<std::uintptr_t> Singleton{ REL::ID(4796374) };
				inline static const REL::Relocation<std::uintptr_t> ShowSubtitle{ REL::ID(2249542) };
				inline static const REL::Relocation<std::uintptr_t> DisplayNextSubtitle{ REL::ID(2249551) };
			};
		};
		static_assert(sizeof(SubtitleManager) == 0x50);
	}
}
