#pragma once

#include "BSPointerHandleRef.h"
#include "BSStringPool.h"

namespace RE
{
	class TESTopicInfo;
	class BGSScene;

	enum class SubtitlePriority : std::uint8_t
	{
		Low = 0x0,
		Normal = 0x1,
		High = 0x2,
		Force = 0x3,
	};

	namespace Override
	{
		// 20
		class SubtitleInfo
		{
		public:
			// members
			/*00*/ BSPointerHandleRef speaker;
			/*04*/ std::uint32_t pad04;
			/*08*/ Override::BSFixedStringCS text;
			/*10*/ TESTopicInfo* topicInfo;
			/*18*/ SubtitlePriority priority;
			/*20*/ std::uint8_t pad20[3];
			/*1C*/ float distanceFromPlayer;
		};
		static_assert(sizeof(SubtitleInfo) == 0x20);
	}
}
