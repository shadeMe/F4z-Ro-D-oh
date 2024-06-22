#include "Hooks.h"
#include "xbyak/xbyak.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se_common/Relocation.h"

namespace hookedAddresses
{
	// E8 ? ? ? ? 48 8B F0 EB 03 49 8B F5 44 8B 43 08
	RelocAddr<uintptr_t>	kDialogueResponse_Ctor(MAKE_RVA(0x0000000140B3ACD0));
	uintptr_t				kDialogueResponse_Ctor_Hook = kDialogueResponse_Ctor + 0x102;
	uintptr_t				kDialogueResponse_Ctor_Ret = kDialogueResponse_Ctor + 0x107;

	// E8 ? ? ? ? 4C 8B 74 24 ? C6 45 3E 01
	RelocAddr<uintptr_t>	kSubtitleManager_ShowSubtitle(MAKE_RVA(0x0000000140FF69D0));
	uintptr_t				kSubtitleManager_ShowSubtitle_DialogSubs_Hook = kSubtitleManager_ShowSubtitle + 0xB6;
	uintptr_t				kSubtitleManager_ShowSubtitle_DialogSubs_Show = kSubtitleManager_ShowSubtitle + 0xC3;
	uintptr_t				kSubtitleManager_ShowSubtitle_DialogSubs_Exit = kSubtitleManager_ShowSubtitle + 0x188;

	uintptr_t				kSubtitleManager_ShowSubtitle_GeneralSubs_Hook = kSubtitleManager_ShowSubtitle + 0x17D;
	uintptr_t				kSubtitleManager_ShowSubtitle_GeneralSubs_Show = kSubtitleManager_ShowSubtitle + 0x19E;
	uintptr_t				kSubtitleManager_ShowSubtitle_GeneralSubs_Exit = kSubtitleManager_ShowSubtitle + 0x188;

	// E8 ? ? ? ? 48 8B 74 24 ? 84 C0 75 18
	RelocAddr<uintptr_t>	kSubtitleManager_DisplayNextSubtitle(MAKE_RVA(0x0000000140FF7070));
	uintptr_t				kSubtitleManager_DisplayNextSubtitle_DialogSubs_Hook = kSubtitleManager_DisplayNextSubtitle + 0x12B;
	uintptr_t				kSubtitleManager_DisplayNextSubtitle_DialogSubs_Show = kSubtitleManager_DisplayNextSubtitle + 0x14E;
	uintptr_t				kSubtitleManager_DisplayNextSubtitle_DialogSubs_Exit = kSubtitleManager_DisplayNextSubtitle + 0x134;

	uintptr_t				kSubtitleManager_DisplayNextSubtitle_GeneralSubs_Hook = kSubtitleManager_DisplayNextSubtitle + 0x141;
	uintptr_t				kSubtitleManager_DisplayNextSubtitle_GeneralSubs_Show = kSubtitleManager_DisplayNextSubtitle + 0x14E;
	uintptr_t				kSubtitleManager_DisplayNextSubtitle_GeneralSubs_Exit = kSubtitleManager_DisplayNextSubtitle + 0x2A2;
}


void SneakAtackVoicePath(DialogueResponse* Data, char* VoicePathBuffer)
{
	// overwritten code
	CALL_MEMBER_FN(&Data->voiceFilePath, Set)(VoicePathBuffer);

	if (strlen(VoicePathBuffer) < 17)
		return;

	std::string FUZPath(VoicePathBuffer), WAVPath(VoicePathBuffer), XWMPath(VoicePathBuffer);
	WAVPath.erase(0, 5);

	FUZPath.erase(0, 5);
	FUZPath.erase(FUZPath.length() - 3, 3);
	FUZPath.append("fuz");

	XWMPath.erase(0, 5);
	XWMPath.erase(XWMPath.length() - 3, 3);
	XWMPath.append("xwm");

	BSIStream* WAVStream = BSIStream::CreateInstance(WAVPath.c_str());
	BSIStream* FUZStream = BSIStream::CreateInstance(FUZPath.c_str());
	BSIStream* XWMStream = BSIStream::CreateInstance(XWMPath.c_str());

#ifndef NDEBUG
	_MESSAGE("Expected: %s", VoicePathBuffer);
	gLog.Indent();
	_MESSAGE("WAV Stream [%s] Validity = %d", WAVPath.c_str(), WAVStream->valid);
	_MESSAGE("FUZ Stream [%s] Validity = %d", FUZPath.c_str(), FUZStream->valid);
	_MESSAGE("XWM Stream [%s] Validity = %d", XWMPath.c_str(), XWMStream->valid);
	gLog.Outdent();
#endif

#ifndef NDEBUG
	if (true)
#else
	if (WAVStream->valid == 0 && FUZStream->valid == 0 && XWMStream->valid == 0)
#endif
	{
		static const int kWordsPerSecond = kWordsPerSecondSilence.GetData().i;
		static const int kCharacterPerWord = kWideCharacterPerWord.GetData().i;
		static const int kMaxSeconds = 10;

		int SecondsOfSilence = 2;
		char ShimAssetFilePath[0x104] = { 0 };
		std::string ResponseText(Data->responseText.c_str());

		if (ResponseText.length() > 4 && strncmp(ResponseText.c_str(), "<ID=", 4))
		{
			int WordCount = 0;
			int WideCharCount = 0;
			int CharOver = 0;
			for (char ch : ResponseText) // check each character
			{
				if (CharOver > 0) // this char is part of a wide-character, pass it
				{
					CharOver --;
					continue;
				}
				if (ch & 0x80 && ch & 0x40 && ch & 0x20)
				{
					if (ch & 0x10)
						CharOver = 3; // a 4 wide-character, 3 bytes left
					else
						CharOver = 2; // a 3 wide-character, 2 bytes left
					WideCharCount ++;
					// What about 2 wide-character? These "2 wide-character languages" basically use spaces to separate words by my google
				}
				else
					WordCount += (ch == ' ');
			}
			WordCount += (WideCharCount / kCharacterPerWord);

			SecondsOfSilence = WordCount / ((kWordsPerSecond > 0) ? kWordsPerSecond : 2) + 1;

			if (SecondsOfSilence <= 0)
				SecondsOfSilence = 2;
			else if (SecondsOfSilence > kMaxSeconds)
				SecondsOfSilence = kMaxSeconds;

			// calculate the response text's hash and stash it for later lookups
			SubtitleHasher::Instance.Add(Data->responseText.c_str());
		}

		if (ResponseText.length() > 1 || (ResponseText.length() == 1 && ResponseText[0] == ' ' && kSkipEmptyResponses.GetData().i == 0))
		{
			FORMAT_STR(ShimAssetFilePath, "Data\\Sound\\Voice\\F4z Ro Doh\\Stock_%d.wav", SecondsOfSilence);
			CALL_MEMBER_FN(&Data->voiceFilePath, Set)(ShimAssetFilePath);
#ifndef NDEBUG
			_MESSAGE("Missing Asset - Switching to '%s'", ShimAssetFilePath);
			_MESSAGE("\tResponse: '%s'", Data->responseText.c_str());
#endif
		}
	}

	WAVStream->Dtor();
	FUZStream->Dtor();
	XWMStream->Dtor();
}

bool ShouldForceSubs(SubtitleInfo* SubtitleInfo, UInt32 ForceRegardless, StringCache::Ref* Subtitle)
{
	bool Result = false;

	if (Subtitle && SubtitleHasher::Instance.HasMatch(Subtitle->c_str()))		// force if the subtitle is for a voiceless response
	{
#ifndef NDEBUG
		_MESSAGE("Found a match for %s - Forcing subs", Subtitle->c_str());
#endif

		Result = true;
	}
	else if (ForceRegardless || (SubtitleInfo && SubtitleInfo->priority != 0))
		Result = true;

	return Result;
}

#define PUSH_VOLATILE		push(rcx); push(rdx); push(r8); sub(rsp, 0x20);
#define POP_VOLATILE		add(rsp, 0x20); pop(r8); pop(rdx); pop(rcx); 

bool InstallHooks()
{
	if (!g_branchTrampoline.Create(512))
	{
		_ERROR("Couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
		return false;
	}

	if (!g_localTrampoline.Create(512, nullptr))
	{
		_ERROR("Couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
		return false;
	}

	{
		struct HotswapReponseAssetPath_Code : Xbyak::CodeGenerator
		{
			HotswapReponseAssetPath_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label RetnLabel;

				push(rcx);
				push(rdx);		// asset path
				mov(rcx, rsi);	// cached response
				mov(rax, (uintptr_t)SneakAtackVoicePath);
				call(rax);
				pop(rdx);
				pop(rcx);
				jmp(ptr[rip + RetnLabel]);

			L(RetnLabel);
				dq(hookedAddresses::kDialogueResponse_Ctor_Ret);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		HotswapReponseAssetPath_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kDialogueResponse_Ctor_Hook, uintptr_t(Code.getCode()));
	}

	{
		struct SubtitleManager_ShowSubtitle_DialogSubs_Code : Xbyak::CodeGenerator
		{
			SubtitleManager_ShowSubtitle_DialogSubs_Code(void* buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				movzx(rax, byte[rsp + 0xC0]); // is being spoken to the PC
				push(rsi);	// save/restore the handle to the source actor
				push(rax);

				mov(rax, (uintptr_t)CanShowDialogSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				pop(rax); // is being spoken to the PC need for the next check
				jnz(ShowLabel);

				PUSH_VOLATILE;
				xor (rcx, rcx);
				mov(rdx, rax);  
				mov(r8, r13);
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				pop(rsi);
				jmp(ptr[rip]);
				dq(hookedAddresses::kSubtitleManager_ShowSubtitle_DialogSubs_Exit);

			L(ShowLabel);
				pop(rsi);
				jmp(ptr[rip]);
				dq(hookedAddresses::kSubtitleManager_ShowSubtitle_DialogSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		SubtitleManager_ShowSubtitle_DialogSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kSubtitleManager_ShowSubtitle_DialogSubs_Hook, uintptr_t(Code.getCode()));
	}


	{
		struct SubtitleManager_ShowSubtitle_GeneralSubs_Code : Xbyak::CodeGenerator
		{
			SubtitleManager_ShowSubtitle_GeneralSubs_Code(void* buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				movzx(rax, byte[rsp + 0xC0]); // is being spoken to the PC
				push(rsi);	// save/restore the handle to the source actor
				push(rax);

				mov(rax, (uintptr_t)CanShowGeneralSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				pop(rax); // is being spoken to the PC need for the next check
				jnz(ShowLabel);

				PUSH_VOLATILE;
				xor (rcx, rcx);
				mov(rdx, rax);
				mov(r8, r13);
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				pop(rsi);
				jmp(ptr[rip]);
				dq(hookedAddresses::kSubtitleManager_ShowSubtitle_GeneralSubs_Exit);

			L(ShowLabel);
				pop(rsi);
				jmp(ptr[rip]);
				dq(hookedAddresses::kSubtitleManager_ShowSubtitle_GeneralSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		SubtitleManager_ShowSubtitle_GeneralSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kSubtitleManager_ShowSubtitle_GeneralSubs_Hook, uintptr_t(Code.getCode()));
	}



	{
		struct SubtitleManager_DisplayNextSubtitle_DialogSubs_Code : Xbyak::CodeGenerator
		{
			SubtitleManager_DisplayNextSubtitle_DialogSubs_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(rax, (uintptr_t)CanShowDialogSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				mov(rcx, r14);	// queued subtitle buffer
				add(rcx, rsi);	// index into the above
				xor(rdx, rdx);
				xor(r8, r8);
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kSubtitleManager_DisplayNextSubtitle_DialogSubs_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kSubtitleManager_DisplayNextSubtitle_DialogSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		SubtitleManager_DisplayNextSubtitle_DialogSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kSubtitleManager_DisplayNextSubtitle_DialogSubs_Hook, uintptr_t(Code.getCode()));
	}

	{
		struct SubtitleManager_DisplayNextSubtitle_GeneralSubs_Code : Xbyak::CodeGenerator
		{
			SubtitleManager_DisplayNextSubtitle_GeneralSubs_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(rax, (uintptr_t)CanShowGeneralSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				mov(rcx, r14);	// queued subtitle buffer
				add(rcx, rsi);	// index into the above
				xor(rdx, rdx);
				xor(r8, r8);
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kSubtitleManager_DisplayNextSubtitle_GeneralSubs_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kSubtitleManager_DisplayNextSubtitle_GeneralSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		SubtitleManager_DisplayNextSubtitle_GeneralSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kSubtitleManager_DisplayNextSubtitle_GeneralSubs_Hook, uintptr_t(Code.getCode()));
	}


	return true;
}
