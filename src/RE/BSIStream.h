#pragma once

#include "BSStringPool.h"

namespace RE
{
	// 20
	class BSIStream
	{
		void _Ctor(const char* FilePath, void* ParentLocation, bool Arg3);
		void* _Dtor(bool FreeMemory);

	public:
		// members
		///*00*/ void** vtbl;
		/*08*/ std::uint64_t unk08;    // Smart pointer, actual file stream
		/*10*/ std::uint8_t valid;     // Set to 1 if the stream's valid
		/*11*/ std::uint8_t pad11[7];
		/*18*/ Override::BSFixedStringCS filePath;  // Relative to the Data directory when no BSResource::Location's passed to the ctor (the game uses a static instance); otherwise, use its location

		virtual ~BSIStream() = 0;

		static BSIStream* CreateInstance(const char* FilePath, void* ParentLocation = nullptr, bool Arg3 = false);
		static void       DeleteInstance(BSIStream* Instance);

		struct Offsets
		{
			// E8 ? ? ? ? 33 DB 38 5C 24 30
			inline static const REL::Relocation<std::uintptr_t> Ctor{ REL::ID(2275793) };
			inline static const REL::Relocation<std::uintptr_t> Dtor{ REL::ID(2275803) };
		};
	};
	static_assert(sizeof(BSIStream) == 0x20);
}
