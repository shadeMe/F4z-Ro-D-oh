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

	bool F4SEPlugin_Query(const F4SEInterface * F4SE, PluginInfo * info)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\F4z Ro D-oh.log");
		_MESSAGE("%s Initializing...", MakeSillyName().c_str());

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "F4z Ro D'oh";
		info->version = PACKED_SME_VERSION;

		interfaces::kPluginHandle = F4SE->GetPluginHandle();
		interfaces::kMsgInterface = (F4SEMessagingInterface*)F4SE->QueryInterface(kInterface_Messaging);

		if (F4SE->isEditor)
			return false;
		else if (F4SE->runtimeVersion != RUNTIME_VERSION_1_10_163)
		{
			_MESSAGE("Unsupported runtime version %08X", F4SE->runtimeVersion);
			return false;
		}
		else if (!interfaces::kMsgInterface)
		{
			_MESSAGE("Couldn't initialize messaging interface");
			return false;
		}

		// supported runtime version
		return true;
	}

	bool F4SEPlugin_Load(const F4SEInterface * F4SE)
	{
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

};
