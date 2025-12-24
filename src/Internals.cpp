#include "Internals.h"
#include "Config.h"

#include "RE/BSIStream.h"
#include "RE/BSStringPool.h"
#include "RE/DialogueResponse.h"
#include "RE/SubtitleInfo.h"
#include "RE/SubtitleManager.h"

SubtitleHasher SubtitleHasher::Instance;

SubtitleHasher::HashT SubtitleHasher::CalculateHash(const std::string_view& String)
{
	// Uses the djb2 string hashing algorithm.
	// http://www.cse.yorku.ca/~oz/hash.html
	HashT Hash{ 0 };

	for (const auto Char : String)
		Hash = ((Hash << 5) + Hash) + static_cast<int>(Char);  // Hash * 33 + Char

	return Hash;
}

void SubtitleHasher::Add(const std::string_view& String)
{
	if (String.empty())
		return;

	std::scoped_lock<decltype(Lock)> Guard{ Lock };

	const auto Hash{ CalculateHash(String) };
	if (!Store.contains(Hash))
		Store.insert(Hash);
}

bool SubtitleHasher::HasMatch(const std::string_view& String) const
{
	if (String.empty())
		return false;

	std::scoped_lock<decltype(Lock)> Guard{ Lock };
	return Store.contains(CalculateHash(String));
}

size_t SubtitleHasher::Purge(void)
{
	std::scoped_lock<decltype(Lock)> Guard{ Lock };
	const auto                       Count{ Store.size() };
	Store.clear();
	return Count;
}

void SubtitleHasher::DispatchBackgroundThread()
{
	std::thread CleanupThread([]() {
		using namespace std::chrono_literals;
		const std::chrono::steady_clock Clock;
		const std::chrono::duration     PurgeInterval{ 60s };

		while (true) {
			auto Start{ Clock.now() };
			while ((Clock.now() - Start) < PurgeInterval)
				std::this_thread::sleep_for(2s);

			// We need to periodically purge the hash store as we can't differentiate
			// b'ween topic responses with the same dialog text but different voice
			// assets. For instance, there may be two responses with the text "Hello
			// there!" but only one with a valid voice file.
			const auto PurgedItemCount{ Instance.Purge() };
			if (PurgedItemCount > 0)
				REX::DEBUG("Purged {} items from subtitle hash store", PurgedItemCount);
		}
	});

	CleanupThread.detach();
}


namespace Hooks
{
	using namespace RE;
	using AddressT = REL::Relocation<std::uintptr_t>;

	struct MidFunctionHook : Xbyak::CodeGenerator
	{
		void CallPrologue() 
		{
			push(rcx);
			push(rdx);
			push(r8);
			push(r9);
			sub(rsp, 0x20);
		}

		void CallEpilogue()
		{
			add(rsp, 0x20);
			pop(r9);
			pop(r8);
			pop(rdx);
			pop(rcx);
		}
	};


	namespace DialogueItem_SetAudioFilePath
	{
		AddressT Target{ DialogueResponse::Offsets::Ctor.address() + 0x102 };
		AddressT Return{ Target.address() + 0x5 };

		bool HasValidPath(const char* Path)
		{
			static const std::vector<std::string_view> kExtensions{ "wav", "fuz", "xwm" };

			// If the path is shorter than 'Data\Sound\Voice\' for some odd reason,
			// pretend that it's valid.
			if (strlen(Path) < 17)
				return true;

			for (const auto& Extension : kExtensions) {
				std::string NewPath(Path);

				// Remove the Data prefix (since our lookup assumes that as the parent location).
				NewPath.erase(0, 5);
				// Remove extension.
				NewPath.erase(NewPath.length() - 3, 3);
				NewPath.append(Extension);

				auto FileStream(BSIStream::CreateInstance(NewPath.c_str()));
				bool IsValid = FileStream->valid;
				BSIStream::DeleteInstance(FileStream);

				REX::TRACE("Stream '{}' Validity = {}", NewPath, IsValid);

				if (IsValid)
					return true;
			}

			return false;
		}

		std::string PickSilentVoiceFile(const std::string_view& ResponseText)
		{
			static const std::string_view kSilentVoiceFilePathPrefix{ "Data\\Sound\\Voice\\F4z Ro Doh\\Stock_" };
			static const std::string_view kSilentVoiceFilePathSuffix{ ".wav" };
			static const std::uint32_t    kMaximumSecondsOfSilence = 10u;

			std::uint32_t WordCount = 0;
			std::uint32_t WideCharCount = 0;
			std::uint32_t CharOver = 0;

			// Count wide-characters.
			// c.f https://github.com/shadeMe/Fuz-Ro-D-oh-64/pull/6
			for (char Character : ResponseText) {
				if (CharOver > 0) {
					// This char is part of a wide-character, skip.
					CharOver--;
					continue;
				}
				if (Character & 0x80 && Character & 0x40 && Character & 0x20) {
					if (Character & 0x10)
						CharOver = 3;  // 4 wide-character, 3 bytes left.
					else
						CharOver = 2;  // 3 wide-character, 2 bytes left.
					WideCharCount++;
				} else
					WordCount += (Character == ' ');
			}


			WordCount += (WideCharCount / Config::WideCharactersPerWord);
			const auto SecondsOfSilence = std::clamp((WordCount / Config::WordsPerSecondSilence) + 1u,
				Config::MinimumSecondsOfSilence.GetValue(),
				kMaximumSecondsOfSilence);

			return std::format("{}{}{}",
				kSilentVoiceFilePathPrefix,
				SecondsOfSilence,
				kSilentVoiceFilePathSuffix);
		}

		bool HasEmptyResponseText(DialogueResponse* DialogueResponse)
		{
			const auto Text{ DialogueResponse->responseText.Get() };
			return Text.empty() || (Text.size() == 1 && Text.at(0) == ' ');
		}

		void UseSilentVoice(DialogueResponse* DialogueResponse, char* FilePath)
		{
			const auto ReplacementFilePath(PickSilentVoiceFile(DialogueResponse->responseText.Get()));

			DialogueResponse->voiceFilePath.Set(ReplacementFilePath.c_str());
			SubtitleHasher::Instance.Add(DialogueResponse->responseText.Get());
			REX::DEBUG("Swapped '{}' with silent voice path '{}'", FilePath, ReplacementFilePath);
		}

		void SwapAudioFilePath(DialogueResponse* DialogueResponse, char* FilePath)
		{
			if (Config::ForceSilentVoice) {
				UseSilentVoice(DialogueResponse, FilePath);
			} else if (HasValidPath(FilePath)) {
				DialogueResponse->voiceFilePath.Set(FilePath);
				REX::DEBUG("Path '{}' is valid", FilePath);
			} else if (HasEmptyResponseText(DialogueResponse) && Config::SkipEmptyResponses) {
				REX::DEBUG("Response is empty - Ignoring");
			} else
				UseSilentVoice(DialogueResponse, FilePath);
		}

		struct Hook : MidFunctionHook
		{
			Hook()
			{
				CallPrologue();
				{
					mov(rcx, rsi);
					mov(rax, reinterpret_cast<std::uintptr_t>(SwapAudioFilePath));
					call(rax);
				}
				CallEpilogue();

				jmp(ptr[rip]);
				dq(Return.address());
			}
		};

		void Install()
		{
			Hook Code;
			Code.ready();

			auto& Trampoline = REL::GetTrampoline();
			Trampoline.write_jmp5(
				Target.address(),
				reinterpret_cast<std::uintptr_t>(Trampoline.allocate(Code)));
		}
	}

	namespace SubtitleManager_ShowSubtitle
	{
		AddressT Function{ Override::SubtitleManagerEx::Offsets::ShowSubtitle };

		namespace DialogSubs
		{
			AddressT Target{ Function.address() + 0xB6 };
			AddressT ReturnShow{ Target.address() + 0x0D };
			AddressT ReturnSkip{ Target.address() + 0xD2 };

			bool ShouldForceSubs(Override::BSFixedStringCS* Subtitle)
			{
				bool ForceSubs = SubtitleHasher::Instance.HasMatch(Subtitle->Get());
				if (ForceSubs)
					REX::DEBUG("[SubtitleManager::ShowSubtitle] Forced subtitles for response '{}'", Subtitle->Get());

				return ForceSubs || RE::INIPrefSettingCollection::GetSingleton()->GetSetting("bDialogueSubtitles:Interface")->GetBinary();
			}

			struct Hook : MidFunctionHook
			{
				Hook()
				{
					Xbyak::Label LShow;

					movzx(rax, byte[rsp + 0xC0]); // is being spoken to the PC
					push(rsi); // save/restore the handle to the source actor
					push(rax); 

					CallPrologue();
					{
						mov(rcx, r13);
						mov(rax, reinterpret_cast<std::uintptr_t>(ShouldForceSubs));
						call(rax);
					}
					CallEpilogue();

					test(al, al);
					pop(rax); // restore saved
					pop(rsi);
					jnz(LShow);

					jmp(ptr[rip]);
					dq(ReturnSkip.address());
					
				L(LShow);
					jmp(ptr[rip]);
					dq(ReturnShow.address());
				}
			};

			void Install()
			{
				Hook Code;
				Code.ready();

				auto& Trampoline = REL::GetTrampoline();
				Trampoline.write_jmp6(
					Target.address(),
					reinterpret_cast<std::uintptr_t>(Trampoline.allocate(Code)));
				Target.write_fill<0x6>(REL::NOP, 1); 

				REX::INFO("SubtitleManager::ShowSubtitle Dialog Subs Hook");
				REX::INFO("  Target: 0x{:X}", Target.address());
				REX::INFO("  ReturnShow: 0x{:X}", ReturnShow.address());
				REX::INFO("  ReturnSkip: 0x{:X}", ReturnSkip.address());
			}
		}

		namespace GeneralSubs
		{
			AddressT Target{ Function.address() + 0x17D };
			AddressT ReturnShow{ Target.address() + 0x21 };
			AddressT ReturnSkip{ Target.address() + 0x0B };

			bool ShouldForceSubs(Override::BSFixedStringCS* Subtitle)
			{
				bool ForceSubs = SubtitleHasher::Instance.HasMatch(Subtitle->Get());
				if (ForceSubs)
					REX::DEBUG("[SubtitleManager::ShowSubtitle] Forced subtitles for response '{}'", Subtitle->Get());

				return ForceSubs || RE::INIPrefSettingCollection::GetSingleton()->GetSetting("bGeneralSubtitles:Interface")->GetBinary();
			}

			struct Hook : MidFunctionHook
			{
				Hook()
				{
					Xbyak::Label LShow;

					movzx(rax, byte[rsp + 0xC0]); // is being spoken to the PC
					push(rsi); // save/restore the handle to the source actor
					push(rax); 

					CallPrologue();
					{
						mov(rcx, r13);
						mov(rax, reinterpret_cast<std::uintptr_t>(ShouldForceSubs));
						call(rax);
					}
					CallEpilogue();

					test(al, al);
					pop(rax); // restore saved
					pop(rsi);
					jnz(LShow);

					jmp(ptr[rip]);
					dq(ReturnSkip.address());
					
				L(LShow);
					jmp(ptr[rip]);
					dq(ReturnShow.address());
				}
			};

			void Install()
			{
				Hook Code;
				Code.ready();

				auto& Trampoline = REL::GetTrampoline();
				Trampoline.write_jmp6(
					Target.address(),
					reinterpret_cast<std::uintptr_t>(Trampoline.allocate(Code)));
				Target.write_fill<0x6>(REL::NOP, 1); 

				REX::INFO("SubtitleManager::ShowSubtitle General Subs Hook");
				REX::INFO("  Target: 0x{:X}", Target.address());
				REX::INFO("  ReturnShow: 0x{:X}", ReturnShow.address());
				REX::INFO("  ReturnSkip: 0x{:X}", ReturnSkip.address());
			}
		}
	}

	namespace SubtitleManager_DisplayNextSubtitle
	{
		AddressT Function{ Override::SubtitleManagerEx::Offsets::DisplayNextSubtitle };

		namespace DialogSubs
		{
			AddressT Target{ Function.address() + 0x12B };
			AddressT Return{ Target.address() + 0x7 };

			bool ShouldForceSubs(Override::SubtitleInfo* Subtitle)
			{
				bool ForceSubs = SubtitleHasher::Instance.HasMatch(Subtitle->text.Get());
				if (ForceSubs)
					REX::DEBUG("[SubtitleManager::DisplayNextSubtitle] Forced subtitles for response '{}'", Subtitle->text.Get());

				return ForceSubs || RE::INIPrefSettingCollection::GetSingleton()->GetSetting("bDialogueSubtitles:Interface")->GetBinary();
			}

			struct Hook : MidFunctionHook
			{
				Hook()
				{
					CallPrologue();
					{
						lea(rcx, qword[rsi+r14]);
						mov(rax, reinterpret_cast<std::uintptr_t>(ShouldForceSubs));
						call(rax);
					}
					CallEpilogue();

					cmp(al, 0);
					jmp(ptr[rip]);
					dq(Return.address());
				}
			};

			void Install()
			{
				Hook Code;
				Code.ready();

				auto& Trampoline = REL::GetTrampoline();
				Trampoline.write_jmp6(
					Target.address(),
					reinterpret_cast<std::uintptr_t>(Trampoline.allocate(Code)));
				Target.write_fill<0x6>(REL::NOP, 1); 
								
				REX::INFO("SubtitleManager::DisplayNextSubtitle Dialog Subs Hook");
				REX::INFO("  Target: 0x{:X}", Target.address());
				REX::INFO("  Return: 0x{:X}", Return.address());
			}
		}

		namespace GeneralSubs
		{
			AddressT Target{ Function.address() + 0x141 };
			AddressT Return{ Target.address() + 0x7 };

			bool ShouldForceSubs(Override::SubtitleInfo* Subtitle)
			{
				bool ForceSubs = SubtitleHasher::Instance.HasMatch(Subtitle->text.Get());
				if (ForceSubs)
					REX::DEBUG("[SubtitleManager::DisplayNextSubtitle] Forced subtitles for response '{}'", Subtitle->text.Get());
				return ForceSubs || RE::INIPrefSettingCollection::GetSingleton()->GetSetting("bGeneralSubtitles:Interface")->GetBinary();
			}

			struct Hook : MidFunctionHook
			{
				Hook()
				{

					CallPrologue();
					{
						lea(rcx, qword[rsi+r14]);
						mov(rax, reinterpret_cast<std::uintptr_t>(ShouldForceSubs));
						call(rax);
					}
					CallEpilogue();

					cmp(al, 0);
					jmp(ptr[rip]);
					dq(Return.address());
				}
			};

			void Install()
			{
				Hook Code;
				Code.ready();

				auto& Trampoline = REL::GetTrampoline();
				Trampoline.write_jmp6(
					Target.address(),
					reinterpret_cast<std::uintptr_t>(Trampoline.allocate(Code)));
				Target.write_fill<0x6>(REL::NOP, 1); 

				REX::INFO("SubtitleManager::DisplayNextSubtitle General Subs Hook");
				REX::INFO("  Target: 0x{:X}", Target.address());
				REX::INFO("  Return: 0x{:X}", Return.address());
			}
		}
	}

	void Install()
	{
		DialogueItem_SetAudioFilePath::Install();
		SubtitleManager_ShowSubtitle::DialogSubs::Install();
		SubtitleManager_ShowSubtitle::GeneralSubs::Install();
		SubtitleManager_DisplayNextSubtitle::DialogSubs::Install();
		SubtitleManager_DisplayNextSubtitle::GeneralSubs::Install();
	}
}