#pragma once
#include "f4se/PluginAPI.h"
#include "f4se/f4se_common/F4SE_version.h"
#include "f4se/GameAPI.h"
#include "f4se/GameTypes.h"
#include "f4se/GameForms.h"
#include "f4se/GameEvents.h"
#include "f4se/GameData.h"
#include "f4se/GameSettings.h"

#include <SME_Prefix.h>
#include <INIManager.h>
#include <StringHelpers.h>
#include <MiscGunk.h>

#include "common/ICriticalSection.h"
#include <thread>
#include <chrono>
#include <unordered_set>

extern IDebugLog						gLog;

namespace interfaces
{
	extern PluginHandle					kPluginHandle;
	extern F4SEMessagingInterface*		kMsgInterface;
}


extern SME::INI::INISetting				kWordsPerSecondSilence;
extern SME::INI::INISetting				kSkipEmptyResponses;

#define MAKE_RVA(addr)		addr - 0x140000000i64


class F4zRoDohINIManager : public SME::INI::INIManager
{
public:
	void								Initialize(const char* INIPath, void* Paramenter) override;

	static F4zRoDohINIManager			Instance;
};

class SubtitleHasher
{
	static const double					kPurgeInterval;		// in ms

	using HashT = unsigned long;
	using HashListT = std::unordered_set<HashT>;

	mutable ICriticalSection			Lock;
	HashListT							Store;
	SME::MiscGunk::ElapsedTimeCounter	TickCounter;
	double								TickReminder;

	static HashT						CalculateHash(const char* String);

	void								Purge(void);
public:
	SubtitleHasher() : Lock(), Store(), TickCounter(), TickReminder(kPurgeInterval) {}

	void								Add(const char* Subtitle);
	bool								HasMatch(const char* Subtitle);
	void								Tick(void);

	static SubtitleHasher				Instance;
};

// 20
class BSIStream
{
public:
	MEMBER_FN_PREFIX(BSIStream);

	// E8 ? ? ? ? 33 DB 38 5C 24 30
	DEFINE_MEMBER_FN(Ctor, BSIStream*, MAKE_RVA(0x0000000141CC5B30), const char* FilePath, void* ParentLocation, bool Arg3);

	// members
	///*00*/ void**					vtbl;
	/*08*/ UInt64					unk04;		// BSTSmartPointer, actual file stream
	/*10*/ UInt8					valid;		// set to 1 if the stream's valid
	/*11*/ UInt8					pad09[7];
	/*18*/ StringCache::Ref			filePath;	// relative to the Data directory when no BSResource::Location's passed to the ctor (the game uses a static instance)
												// otherwise, use its location

	virtual void*					Dtor(bool FreeMemory = true);

	static BSIStream*				CreateInstance(const char* FilePath, void* ParentLocation = nullptr, bool Arg3 = false);		// BSResource::Location* ParentLocation
};
STATIC_ASSERT(sizeof(BSIStream) == 0x20);


// 40
class CachedResponseData
{
public:
	// members
	/*00*/ StringCache::Ref			responseText;
	/*08*/ UInt64					unk08;
	/*10*/ UInt16					unk10;
	/*14*/ UInt16					unk14;
	/*18*/ StringCache::Ref			voiceFilePath;		// relative path to the voice file
	/*20*/ UInt64					unk20;
	/*28*/ UInt64					unk28;
	/*30*/ UInt64					unk30;
	/*38*/ UInt8					useEmotionAnim;
	/*39*/ UInt8					hasLipFile;
	/*3A*/ UInt8					unk3A;
	/*3B*/ UInt8					pad3B[5];
};
STATIC_ASSERT(sizeof(CachedResponseData) == 0x40);

// 20
class NPCChatterData
{
public:
	// members
	/*00*/ UInt32					speaker;				// the BSHandleRefObject handle to the speaker
	/*04*/ UInt8					pad04[4];
	/*08*/ StringCache::Ref			title;
	/*10*/ void*					unk10;					// seen { float, ... }
	/*18*/ UInt8					forceSubtitles;
	/*19*/ UInt8					pad19[3];
	/*1C*/ float					subtitleDistance;		// init to float::MAX, comapred to the second power of "fMaxSubtitleDistance_Interface"
};
STATIC_ASSERT(sizeof(NPCChatterData) == 0x20);



std::string			MakeSillyName();
bool				CanShowDialogSubtitles();
bool				CanShowGeneralSubtitles();