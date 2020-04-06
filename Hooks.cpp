#include "Hooks.h"
#include "xbyak/xbyak.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se_common/Relocation.h"

namespace hookedAddresses
{
	// E8 ? ? ? ? 48 8B F0 EB 03 49 8B F5 44 8B 43 08
	RelocAddr<uintptr_t>	kCachedResponseData_Ctor(MAKE_RVA(0x0000000140CA1BF0));
	uintptr_t				kCachedResponseData_Ctor_Hook = kCachedResponseData_Ctor + 0xEF;
	uintptr_t				kCachedResponseData_Ctor_Ret = kCachedResponseData_Ctor + 0xF4;

	// E8 ? ? ? ? 4C 8B 74 24 ? C6 45 3E 01
	RelocAddr<uintptr_t>	kASCM_QueueNPCChatterData(MAKE_RVA(0x00000001412B2F70));
	uintptr_t				kASCM_QueueNPCChatterData_DialogSubs_Hook = kASCM_QueueNPCChatterData + 0xAA;
	uintptr_t				kASCM_QueueNPCChatterData_DialogSubs_Show = kASCM_QueueNPCChatterData + 0xBB;
	uintptr_t				kASCM_QueueNPCChatterData_DialogSubs_Exit = kASCM_QueueNPCChatterData + 0x284;

	uintptr_t				kASCM_QueueNPCChatterData_GeneralSubs_Hook = kASCM_QueueNPCChatterData + 0x150;
	uintptr_t				kASCM_QueueNPCChatterData_GeneralSubs_Show = kASCM_QueueNPCChatterData + 0x162;
	uintptr_t				kASCM_QueueNPCChatterData_GeneralSubs_Exit = kASCM_QueueNPCChatterData + 0x284;


	// 40 56 41 54 48 83 EC 48 48 8B F1
	RelocAddr<uintptr_t>	kASCM_DisplayQueuedNPCChatterData(MAKE_RVA(0x00000001412B3530));
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_DialogSubs_Hook = kASCM_DisplayQueuedNPCChatterData + 0xC6;
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_DialogSubs_Show = kASCM_DisplayQueuedNPCChatterData + 0xE4;
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_DialogSubs_Exit = kASCM_DisplayQueuedNPCChatterData + 0xCF;

	uintptr_t				kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Hook = kASCM_DisplayQueuedNPCChatterData + 0xD7;
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Show = kASCM_DisplayQueuedNPCChatterData + 0xE4;
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Exit = kASCM_DisplayQueuedNPCChatterData + 0x199;
}


void SneakAtackVoicePath(CachedResponseData* Data, char* VoicePathBuffer)
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

#if 0
	_MESSAGE("Expected: %s", VoicePathBuffer);
	gLog.Indent();
	_MESSAGE("WAV Stream [%s] Validity = %d", WAVPath.c_str(), WAVStream->valid);
	_MESSAGE("FUZ Stream [%s] Validity = %d", FUZPath.c_str(), FUZStream->valid);
	_MESSAGE("XWM Stream [%s] Validity = %d", XWMPath.c_str(), XWMStream->valid);
	gLog.Outdent();
#endif

#ifndef NDEBUG
	if (!(WAVStream->valid == 0 && FUZStream->valid == 0 && XWMStream->valid == 0))
#else
	if (WAVStream->valid == 0 && FUZStream->valid == 0 && XWMStream->valid == 0)
#endif
	{
		static const int kWordsPerSecond = kWordsPerSecondSilence.GetData().i;
		static const int kMaxSeconds = 10;

		int SecondsOfSilence = 2;
		char ShimAssetFilePath[0x104] = { 0 };
		std::string ResponseText(Data->responseText.c_str());

		if (ResponseText.length() > 4 && strncmp(ResponseText.c_str(), "<ID=", 4))
		{
			SME::StringHelpers::Tokenizer TextParser(ResponseText.c_str(), " ");
			int WordCount = 0;

			while (TextParser.NextToken(ResponseText) != -1)
				WordCount++;

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

bool ShouldForceSubs(NPCChatterData* ChatterData, UInt32 ForceRegardless, StringCache::Ref* Subtitle)
{
	bool Result = false;

	if (Subtitle && SubtitleHasher::Instance.HasMatch(Subtitle->c_str()))		// force if the subtitle is for a voiceless response
	{
#ifndef NDEBUG
		_MESSAGE("Found a match for %s - Forcing subs", Subtitle->c_str());
#endif

		Result = true;
	}
	else if (ForceRegardless || (ChatterData && ChatterData->forceSubtitles))
		Result = true;

	return Result;
}

#define PUSH_VOLATILE		push(rcx); push(rdx); push(r8);
#define POP_VOLATILE		pop(r8); pop(rdx); pop(rcx);

bool InstallHooks()
{
	if (!g_branchTrampoline.Create(1024 * 2))
	{
		_ERROR("Couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
		return false;
	}

	if (!g_localTrampoline.Create(1024 * 2, nullptr))
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
				dq(hookedAddresses::kCachedResponseData_Ctor_Ret);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		HotswapReponseAssetPath_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kCachedResponseData_Ctor_Hook, uintptr_t(Code.getCode()));
	}

	{
		struct ASCMQueueNPCChatterData_DialogSubs_Code : Xbyak::CodeGenerator
		{
			ASCMQueueNPCChatterData_DialogSubs_Code(void* buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(byte[rbp + 0x48], r14b);
				mov(rax, (uintptr_t)CanShowDialogSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				xor (rcx, rcx);
				movzx(rdx, byte[rbp + 0x60]);
				mov(r8, r13);
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_QueueNPCChatterData_DialogSubs_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_QueueNPCChatterData_DialogSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		ASCMQueueNPCChatterData_DialogSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kASCM_QueueNPCChatterData_DialogSubs_Hook, uintptr_t(Code.getCode()));
	}


	{
		struct ASCMQueueNPCChatterData_GeneralSubs_Code : Xbyak::CodeGenerator
		{
			ASCMQueueNPCChatterData_GeneralSubs_Code(void* buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(byte[rbp + 0x48], al);
				mov(rax, (uintptr_t)CanShowGeneralSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				xor (rcx, rcx);
				movzx(rdx, byte[rbp + 0x60]);
				mov(r8, r13);
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_QueueNPCChatterData_GeneralSubs_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_QueueNPCChatterData_GeneralSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		ASCMQueueNPCChatterData_GeneralSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kASCM_QueueNPCChatterData_GeneralSubs_Hook, uintptr_t(Code.getCode()));
	}



	{
		struct ASCMDisplayQueuedNPCChatterData_DialogSubs_Code : Xbyak::CodeGenerator
		{
			ASCMDisplayQueuedNPCChatterData_DialogSubs_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(rax, (uintptr_t)CanShowDialogSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				mov(rcx, rdi);
				xor(rdx, rdx);
				xor(r8, r8);
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_DialogSubs_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_DialogSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		ASCMDisplayQueuedNPCChatterData_DialogSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_DialogSubs_Hook, uintptr_t(Code.getCode()));
	}

	{
		struct ASCMDisplayQueuedNPCChatterData_GeneralSubs_Code : Xbyak::CodeGenerator
		{
			ASCMDisplayQueuedNPCChatterData_GeneralSubs_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(rax, (uintptr_t)CanShowGeneralSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				mov(rcx, rdi);
				xor(rdx, rdx);
				xor(r8, r8);
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		ASCMDisplayQueuedNPCChatterData_GeneralSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Hook, uintptr_t(Code.getCode()));
	}


	return true;
}
