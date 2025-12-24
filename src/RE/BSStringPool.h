#pragma once

#include "RE/B/BSStringPool.h"

namespace RE
{
	// To workaround the fact that CommonLib definition doesn't define 
	// the string data as part of the Entry class (which makes inspecting 
	// them in the debugger a pain)?
	namespace Override
	{
		struct InspectableStringPoolEntry : public BSStringPool::Entry
		{
			union
			{
				char u8[0];
				wchar_t u16[0];
			} string;
		};

		// 08
		class BSFixedString
		{
		public:
			/*00*/ InspectableStringPoolEntry* data;

			std::string_view Get() const;
			void Set(const char* string);

			struct Offsets
			{
				// Append, actualy.
				inline static const REL::Relocation<std::uintptr_t> Set{ REL::ID(2268671) };
			};
		};
		static_assert(sizeof(BSFixedString) == 0x8);

		using BSFixedStringCS = BSFixedString;

		// 08
		class BGSLocalizedStringIL
		{
		public:
			/*00*/ BSFixedStringCS data;
		};
		static_assert(sizeof(BGSLocalizedStringIL) == 0x8);
	}
}
