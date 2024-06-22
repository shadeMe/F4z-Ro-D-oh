#include "F4zRoDohInternals.h"
#include "Hooks.h"
#include "VersionInfo.h"
#include <ShlObj.h>


extern "C"
{
	void MessageHandler(F4SEMessagingInterface::Message * msg)
	{
		switch (msg->type)
		{
		case F4SEMessagingInterface::kMessage_InputLoaded:
			{
				// schedule a cleanup thread for the subtitle hasher
				std::thread CleanupThread([]() {
					while (true)
					{
						std::this_thread::sleep_for(std::chrono::seconds(2));
						SubtitleHasher::Instance.Tick();
					}
				});
				CleanupThread.detach();

				_MESSAGE("Scheduled cleanup thread");
				_MESSAGE("%s Initialized!", MakeSillyName().c_str());
			}
			break;
		}
	}

	__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface * F4SE)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\F4z Ro D-oh.log");
		_MESSAGE("%s Initializing...", MakeSillyName().c_str());

		interfaces::kPluginHandle = F4SE->GetPluginHandle();
		interfaces::kMsgInterface = (F4SEMessagingInterface*)F4SE->QueryInterface(kInterface_Messaging);

		if (F4SE->isEditor)
			return false;
		else if (!interfaces::kMsgInterface)
		{
			_MESSAGE("Couldn't initialize messaging interface");
			return false;
		}

		_MESSAGE("Initializing INI Manager");
		F4zRoDohINIManager::Instance.Initialize("Data\\F4SE\\Plugins\\F4z Ro D'oh.ini", nullptr);

		if (interfaces::kMsgInterface->RegisterListener(interfaces::kPluginHandle, "F4SE", MessageHandler) == false)
		{
			_MESSAGE("Couldn't register message listener");
			return false;
		}
		else if (InstallHooks() == false)
			return false;

		return true;
	}

	__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version =
	{
		F4SEPluginVersionData::kVersion,

		PACKED_SME_VERSION,
		"Fuz Ro D'oh",
		"shadeMe",
		0,	// Version-dependent
		0,	// Structure-dependent
		{ RUNTIME_VERSION_1_10_984, 0 },
		0,
	};

};
