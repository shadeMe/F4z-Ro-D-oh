#include "Internals.h"
#include "Config.h"

namespace
{
	void MessageHandler(F4SE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case F4SE::MessagingInterface::kPostLoad:
			{
				SubtitleHasher::Instance.DispatchBackgroundThread();
				break;
			}
		default:
			break;
		}
	}
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	//while (IsDebuggerPresent() == FALSE)
	//	::Sleep(500);

	F4SE::Init(a_f4se, { .trampoline = true, .trampolineSize = 0x256, });
	F4SE::GetMessagingInterface()->RegisterListener(MessageHandler);

	if (Config::LogVerbose)
		spdlog::default_logger_raw()->set_level(spdlog::level::trace);

	Config::Load();
	Hooks::Install();

	REX::INFO("Load");

	return true;
}
