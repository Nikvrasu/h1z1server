#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "VCPatcher.h"
#include "Hooking.Patterns.h"
#include "Utils.h"
#include <stdio.h>
#include <winternl.h>
#include <ntstatus.h>
#include <windows.h>
#include <tlhelp32.h>
#include <MinHook.h>
#include "UdpPlatformAddress.h"

//FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
#define CONSOLE_ENABLED_THAT_CRASHES

static bool consoleShowing = false;
static float lasttext;

intptr_t* MenuManager = (intptr_t*)0x868638;
void* ConnectionMgrDummy = (void*)0x143B3E498;

#include "timer.h"
#include <iostream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>
#include <udis86.h>

#pragma comment(lib, "ws2_32.lib")

static bool(*g_origSetupInitialConnection)(intptr_t a1);
static bool(*g_InitGameWorld)(intptr_t a1);
static bool(*g_orig_TrySignalLaunchPadEvent)(intptr_t a1, void* a2);
static bool(*g_RegisterCommands)();
static void(*g_InitCharacterStuff)(intptr_t a1, intptr_t a2, intptr_t a3, intptr_t a4);
static void(*g_BeginZoningAreaWrapper)(intptr_t a1, float possiblyRadius);

void hexDump(const char* desc, const void* addr, const int len);

static void* FindCallFromAddress(void* methodPtr, ud_mnemonic_code mnemonic = UD_Icall, bool breakOnFirst = false)
{
	// return value holder
	void* retval = nullptr;

	// initialize udis86
	ud_t ud;
	ud_init(&ud);

	// set the correct architecture
	ud_set_mode(&ud, 64);

	// set the program counter
	ud_set_pc(&ud, reinterpret_cast<uint64_t>(methodPtr));

	// set the input buffer
	ud_set_input_buffer(&ud, reinterpret_cast<uint8_t*>(methodPtr), INT32_MAX);

	// loop the instructions
	while (true)
	{
		// disassemble the next instruction
		ud_disassemble(&ud);

		// if this is a retn, break from the loop
		if (ud_insn_mnemonic(&ud) == UD_Iint3 || ud_insn_mnemonic(&ud) == UD_Inop)
		{
			break;
		}

		if (ud_insn_mnemonic(&ud) == mnemonic)
		{
			// get the first operand
			auto operand = ud_insn_opr(&ud, 0);

			// if it's a static call...
			if (operand->type == UD_OP_JIMM)
			{
				// ... and there's been no other such call...
				if (retval == nullptr)
				{
					// ... calculate the effective address and store it
					retval = reinterpret_cast<void*>(ud_insn_len(&ud) + ud_insn_off(&ud) + operand->lval.sdword);

					if (breakOnFirst)
					{
						break;
					}
				}
				else
				{
					// return an empty pointer
					retval = nullptr;
					break;
				}
			}
		}
	}

	return retval;
}

intptr_t LaunchPadA1Ptr;
static bool TrySignalLaunchPadEvent(intptr_t a1,  void* a2)
{
	LaunchPadA1Ptr = a1;
	//g_origSetupInitialConnection(a1);
	return g_orig_TrySignalLaunchPadEvent(a1, a2);
}

HANDLE h_console;
static void tryAllocConsole() {
	if (!consoleShowing)
	{
		//Allocate a console
		AllocConsole();
		AttachConsole(GetCurrentProcessId());
		freopen("CON", "w", stdout);
		consoleShowing = true;
		h_console = GetStdHandle(STD_OUTPUT_HANDLE);
	}
}

static void doSomeLogging(const char* fmt, va_list args) {
	tryAllocConsole();

	FILE* logFile = _wfopen(L"GameMessages.log", L"a");
	if (logFile)
	{
		char buffer[2048*4], bufferNewLine[(2048 * 4) + 1];

		vsnprintf(buffer, sizeof(buffer), fmt, args);
		SetConsoleTextAttribute(h_console, 7);

		sprintf(bufferNewLine, "%s\n", buffer);
		vfprintf(logFile, bufferNewLine, args); //write to file

		va_end(args);

		fclose(logFile);
		printf_s(bufferNewLine);
	}
}

#if 1
void logFuncCustom(intptr_t unka1, const char* logEntry, ...) {

	va_list args;
	va_start(args, logEntry);
	doSomeLogging(logEntry, args);
	va_end(args);

	return;
}
#endif

static void(*logFuncCustom1_orig)(intptr_t a1, intptr_t size, const char* logEntry, ...);
static void logFuncCustom1(intptr_t a1, intptr_t size, const char* logEntry, ...) {

	logFuncCustom1_orig(a1, size, logEntry);

	va_list args;
	va_start(args, logEntry);
	doSomeLogging(logEntry, args);
	va_end(args);

	return;
}

static void(*logFuncCustom2_orig)(int loglevel, intptr_t* unka1, const char* logEntry, va_list args);
static void logFuncCustom2(int loglevel, intptr_t* unka1, const char* logEntry, va_list args) {
	printf("Return Address: %p\n", _ReturnAddress());

	logFuncCustom2_orig(loglevel, unka1, logEntry, args);

	doSomeLogging(logEntry, args);

	return;
}

static void(*logFuncCustom_orig3)(int lvl, const char* logEntry, void* a3, va_list args);
static void logFuncCustom3(int lvl, const char* logEntry, void* a3, va_list args) {

	logFuncCustom_orig3(lvl, logEntry, a3, args);

	doSomeLogging(logEntry, args);
	return;
}

static void(*logFuncCustom4_orig)(intptr_t* unka1, const char* logEntry, va_list args);
static void logFuncCustom4(intptr_t* unka1, const char* logEntry, va_list args) {

	logFuncCustom4_orig(unka1, logEntry, args);
	doSomeLogging(logEntry, args);
	return;
}

enum ClientStates {
	cClientRunStateNone = 0,
	cClientRunStateAdminBackdoorLoginStart = 1,
	cClientRunStateAdminBackdoorLogin,
	cClientRunStateAdminPreInitialize,
	cClientRunStatePreInitialize,
	cClientRunStateVerifyPsnLogin = 6,
	cClientRunStateCheckPsnChatRestrictions,
	cClientRunStateWaitForPsnChatRestrictionsDialog,
	cClientRunStateCheckPsnUgcRestrictions,
	cClientRunStateWaitForPsnUgcRestrictionsDialog,
	cClientRunStateWaitingForPsnLogin,
	cClientRunStateStartingLogin,
	cClientRunStateWaitForCharacterList,
	cClientRunStateWaitForCharacterSelectLoad,
	cClientRunStateCharacterCreateOrDelete,
	cClientRunStateLoggingIn,
	cClientRunStateNetInitialize,
	cClientRunStateConnecting,
	cClientRunStatePostInitialize,
	cClientRunStateWaitForInitialDeployment,
	cClientRunStatePostInitialDeployment,
	cClientRunStateWaitForFirstZone,
	cClientRunStateWaitForConfirmationPacket,
	cClientRunStatePostWaitForFirstZone,
	cClientRunStateWaitForContinue,
	cClientRunStateRunning,
	cClientRunStateWaitForTeleport,
	cClientRunStateWaitForZoneLoad,
	cClientRunStateWaitingForReloginSession,
	cClientRunStateStartingRelogin,
	cClientRunStateVerifyXBLiveLogin = 34,
	cClientRunStateShuttingDown
};

static bool(*g_origRespawnWindow__DisplayRespawn)(intptr_t a1);
static bool RespawnWindow__DisplayRespawn(intptr_t a1)
{
	g_origRespawnWindow__DisplayRespawn(a1);
	return true; //never fail
}

static void(*g_origShowErrorCodeAndExitImmediately)(int code, char* reason, bool a3, bool* a4);
static void ShowErrorCodeAndExitImmediately(int code, char* reason, bool a3, bool* a4)
{
	if (code == 8) return;

	g_origShowErrorCodeAndExitImmediately(code, reason, a3, a4);
}

static void*(*g_origAllocSomeMemory)(int size);
static void*(*g_instanceUnknownClass)(void* a1);
void* unk_143B3E5B8 = (void*)0x143B3E5B8;
void* g_proxiedCharacter = (void*)0x143B3E438;

std::atomic<bool> alreadyDoneDeployment = false;
std::atomic<bool> waitingForZoneLoad = true;

bool alreadyDone = false;
static void(*g_origLoadConfigFile)(intptr_t a1, bool a2);
static void(*g_origOnGameStartup)(intptr_t a1);
static void OnGameStartup(intptr_t a1)
{
	int switchCaseReason = *(int*)(a1 + 0x202208);
	int switchCaseReason2 = *(int*)(a1 + 0x315E0);

	//attickminimalrendering workaround
	*(intptr_t*)(a1 + 0x231873) = 1; //render timer related thing, set to 1
	*(intptr_t*)(a1 + 0x389C1) = 1; //render timer related thing, set to 1

	if (switchCaseReason == 0 || switchCaseReason2 == 0)
	{
		if (!alreadyDone)
			*(int*)(a1 + 0x315E0) = cClientRunStatePreInitialize;//4; //skip login

		alreadyDone = true;
	}

	g_origOnGameStartup(a1);
	return;
}

static char*(*g_origGetShutdownReasonString)(int index);
static char* GetShutdownReasonString(int index)
{
	char* string = g_origGetShutdownReasonString(index);
	logFuncCustom(0, "(%d), %s", index, string);
	return string; //index is 6 when it fails which is assigned by shutdown (35)
}

static void(*g_orig_SetupAuthData)(intptr_t a1);
static void SetupAuthData(intptr_t a1)
{
	*(intptr_t*)(a1 + 0x31B68) = 0; //feed it a bunch of lies
	*(intptr_t*)(a1 + 0x31F20) = 0;
	*(intptr_t*)(a1 + 0x31F21) = 0;

	g_orig_SetupAuthData(a1);
	return;
}

#ifdef CONSOLE_ENABLED_THAT_CRASHES
static HookFunction hookFunction([]()
{
});
#endif

bool ReturnTrue() {
	return true;
}
bool ReturnFalse() {
	return false;
}
extern VCPatcher gl_patcher;

static bool(*g_origConstructDisplay)(intptr_t a1);
static bool ConstructDisplay(intptr_t a1)
{
	g_origConstructDisplay(a1);
	return true; //never fail
}

static bool(*g_origOnReceiveServer)(void* a1, void* a2, void* a3);
static bool OnReceiveServer(void* a1, void* a2, void* a3)
{
	return g_origOnReceiveServer(a1, a2, a3);
}

static SOCKET g_gameSocket;

std::string string_to_hex(const std::string& input)
{
	static const char* const lut = "0123456789ABCDEF";
	size_t len = input.length();

	std::string output;
	output.reserve(2 * len);
	for (size_t i = 0; i < len; ++i)
	{
		const unsigned char c = input[i];
		output.push_back(lut[c >> 4]);
		output.push_back(lut[c & 15]);
	}
	return output;
}

int __stdcall CfxBind(SOCKET s, sockaddr * addr, int addrlen)
{
	sockaddr_in* addrIn = (sockaddr_in*)addr;

	printf_s("binder on %i is %p, %p\n", htons(addrIn->sin_port), (void*)s, _ReturnAddress());

	//if (htons(addrIn->sin_port) == 34567)
	{
		g_gameSocket = s;
	}

	return bind(s, addr, addrlen);
}

int __stdcall CfxRecvFrom(SOCKET s, char * buf, int len, int flags, sockaddr * from, int * fromlen)
{
	static char buffer[65536];
	uint16_t netID = 0;
	sockaddr_in* outFrom = (sockaddr_in*)from;
	char addr[60];
	if (s == g_gameSocket)
	{
		inet_ntop(AF_INET, &outFrom->sin_addr.s_addr, addr, sizeof(addr));
		printf_s("CfxRecvFrom (from %i %s) %i bytes on %p, port: %i, %p\n", netID, addr, len, (void*)s, htons(outFrom->sin_port), _ReturnAddress());
	}

	return recvfrom(s, buf, len, flags, from, fromlen);
}

int __stdcall CfxSendTo(SOCKET s, char * buf, int len, int flags, sockaddr * to, int tolen)
{
	sockaddr_in* toIn = (sockaddr_in*)to;

	if (s == g_gameSocket)
	{
		if (toIn->sin_addr.S_un.S_un_b.s_b1 == 0xC0 && toIn->sin_addr.S_un.S_un_b.s_b2 == 0xA8)
		{
			//g_pendSendVar = 0;

			//if (CoreIsDebuggerPresent())
			{
				printf_s("CfxSendTo (to internal address %i) port: %i, %i b (from thread 0x%x), %p\n", (htonl(toIn->sin_addr.s_addr) & 0xFFFF) ^ 0xFEED, htons(toIn->sin_port), len, GetCurrentThreadId(), _ReturnAddress());
				printf_s("CfxSendTo: Data: %s Hex: %s\n", buf, string_to_hex(buf));
			}
		}
		else
		{
			char publicAddr[256];
			inet_ntop(AF_INET, &toIn->sin_addr.s_addr, publicAddr, sizeof(publicAddr));

			if (toIn->sin_addr.s_addr == 0xFFFFFFFF)
			{
				return len;
			}

			printf_s("CfxSendTo (to %s) port: %i, %i b, %p\n", publicAddr, htons(toIn->sin_port), len, _ReturnAddress());
		}

		//g_netLibrary->RoutePacket(buf, len, (uint16_t)((htonl(toIn->sin_addr.s_addr)) & 0xFFFF) ^ 0xFEED);

		//return len;
	}

	return sendto(s, buf, len, flags, to, tolen);
}

int __stdcall CfxSend(SOCKET s, char* buf, int len, int flags)
{

	if (s == g_gameSocket)
	{
		printf_s("CfxSend %i b, %p\n", len, _ReturnAddress());
	}

	return send(s, buf, len, flags);
}

int __stdcall CfxWSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData) {
	return WSAStartup(wVersionRequested, lpWSAData);
}

int __stdcall CfxGetSockName(SOCKET s, struct sockaddr* name, int* namelen)
{
	int retval = getsockname(s, name, namelen);

	sockaddr_in* addrIn = (sockaddr_in*)name;

	if (s == g_gameSocket /*&& wcsstr(GetCommandLine(), L"cl2")*/)
	{
		addrIn->sin_port = htons(6672);
	}

	return retval;
}

static int(__stdcall* g_oldSelect)(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const timeval* timeout);

int __stdcall CfxSelect(_In_ int nfds, _Inout_opt_ fd_set FAR *readfds, _Inout_opt_ fd_set FAR *writefds, _Inout_opt_ fd_set FAR *exceptfds, _In_opt_ const struct timeval FAR *timeout)
{
	bool shouldAddSocket = false;

	for (int i = 0; i < readfds->fd_count; i++)
	{
		if (readfds->fd_array[i] == g_gameSocket)
		{
			memmove(&readfds->fd_array[i + 1], &readfds->fd_array[i], readfds->fd_count - i - 1);
			readfds->fd_count -= 1;
			nfds--;

			/*
			if (g_netLibrary->WaitForRoutedPacket((timeout) ? ((timeout->tv_sec * 1000) + (timeout->tv_usec / 1000)) : INFINITE))
			{
				shouldAddSocket = true;
			}
			*/
		}
	}

	//FD_ZERO(readfds);

	if (nfds > 0)
	{
		nfds = g_oldSelect(nfds, readfds, writefds, exceptfds, timeout);
	}

	if (shouldAddSocket)
	{
		FD_SET(g_gameSocket, readfds);

		nfds += 1;
	}

	return nfds;
}

static void(*logFuncCustomCallOrig_orig)(void* a1, const char* fmt, va_list args);
static void logFuncCustomCallOrig(void* a1, const char* fmt, va_list args) {
	__try
	{
		doSomeLogging(fmt, args);
		logFuncCustomCallOrig_orig(a1, fmt, args);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		printf_s("logFuncCustomCallOrig excepted, caught and returned.\n");
	}
}

//ANTI DEBUG
bool IsDebuggerPresentOurs() {
	return true;
}


typedef NTSTATUS(NTAPI* pfnNtQueryInformationProcess)(
	_In_      HANDLE           ProcessHandle,
	_In_      UINT             ProcessInformationClass,
	_Out_     PVOID            ProcessInformation,
	_In_      ULONG            ProcessInformationLength,
	_Out_opt_ PULONG           ReturnLength
	);

int ProcessDebugPort2 = 7;

pfnNtQueryInformationProcess g_origNtQueryInformationProcess = NULL;


static ULONG ValueProcessBreakOnTermination = FALSE;
static bool IsProcessHandleTracingEnabled = false;

DWORD dwExplorerPid = 0;
WCHAR ExplorerProcessName[] = L"explorer.exe";

DWORD GetProcessIdByName(const WCHAR* processName)
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return 0;
	}

	DWORD pid = 0;

	do
	{
		if (!lstrcmpiW((LPCWSTR)pe32.szExeFile, processName))
		{
			pid = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return pid;
}

DWORD GetExplorerProcessId()
{
	if (!dwExplorerPid)
	{
		dwExplorerPid = GetProcessIdByName(ExplorerProcessName);
	}
	return dwExplorerPid;
}

void SetupSetPEB() {
	// Thread Environment Block (TEB)
#if defined(_M_X64) // x64
	PTEB tebPtr = reinterpret_cast<PTEB>(__readgsqword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
#else // x86
	PTEB tebPtr = reinterpret_cast<PTEB>(__readfsdword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
#endif

	// Process Environment Block (PEB)
	PPEB pebPtr = tebPtr->ProcessEnvironmentBlock;
	pebPtr->BeingDebugged = false;
}

static LONG(*g_exceptionHandler)(EXCEPTION_POINTERS*);
static BOOLEAN(*g_origRtlDispatchException)(EXCEPTION_RECORD* record, CONTEXT* context);

static BOOLEAN RtlDispatchExceptionStub(EXCEPTION_RECORD* record, CONTEXT* context)
{
	// anti-anti-anti-anti-debug
	if (IsDebuggerPresentOurs() && (record->ExceptionCode == 0xc0000008/* || record->ExceptionCode == 0xc0000005*/))
	{
		return TRUE;
	}

	BOOLEAN success = g_origRtlDispatchException(record, context);
	//140533ae2
	if (IsDebuggerPresentOurs())
	{
		if (!success) {
			printf("Exception at: %p\n", record->ExceptionAddress);
		}
		return success;
	}

	static bool inExceptionFallback;

	if (!success)
	{
		if (!inExceptionFallback)
		{
			inExceptionFallback = true;

			//AddCrashometry("exception_override", "true");

			EXCEPTION_POINTERS ptrs;
			ptrs.ContextRecord = context;
			ptrs.ExceptionRecord = record;

			if (g_exceptionHandler)
			{
				g_exceptionHandler(&ptrs);
			}

			inExceptionFallback = false;
		}
	}

	return success;
}

void SetupHook()
{
	void* baseAddress = GetProcAddress(GetModuleHandle("ntdll.dll"), "KiUserExceptionDispatcher");

	if (baseAddress)
	{
		void* internalAddress = FindCallFromAddress(baseAddress, UD_Icall, true);

		{
			MH_CreateHook(internalAddress, RtlDispatchExceptionStub, (void**)&g_origRtlDispatchException);
		}
	}

	MH_EnableHook(MH_ALL_HOOKS);
	return;
}

void VCPatcher::PreHooks() {
	SetupSetPEB();
	SetupHook();
}

static void WINAPI ExitProcessReplacement(UINT exitCode)
{
	TerminateProcess(GetCurrentProcess(), exitCode);
}

void* unkArgumentToTP = nullptr;

static void(*g_orig_TransitionClientRunState)(void* a1, int state, void* a3);
static void TransitionClientRunState(void* a1, int state, void* a3) {
	unkArgumentToTP = a1; //Set it
	g_orig_TransitionClientRunState(a1, state, a3);
}

static void(*handleInitException_orig)(void* a1, void* a2, void* a3, void* a4);
static void handleInitException(void* a1, void* a2, void* a3, void* a4) {
	__try
	{
		handleInitException_orig(a1, a2, a3, a4);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		printf_s("handleInitException excepted, caught and returned.\n");
	}
}

static void(*printProfilerCall_orig)(void* a1, char* funcName, int a3);
static void printProfilerCall(void* a1, char* funcName, int a3) {
	char msgout[256];
	sprintf(msgout, "%s - Return Address: %p\n", funcName, _ReturnAddress());
	doSomeLogging(msgout, nullptr);
	printProfilerCall_orig(a1, funcName, a3);
}

static void(*readPayLoad_orig)(void* a1, char* payLoadName, int a3);
static void readPayLoad(void* a1, char* payLoadName, int a3) {
	printf("readPayLoad - payLoadName: %s - Return Address: %p\n", payLoadName, _ReturnAddress());
	readPayLoad_orig(a1, payLoadName, a3);
}

static void(*readPayLoad2_orig)(void* a1, char* payLoadName, int a3);
static void readPayLoad2(void* a1, char* payLoadName, int a3) {
	printf("readPayLoad2 - payLoadName: %s - Return Address: %p\n", payLoadName, _ReturnAddress());
	readPayLoad2_orig(a1, payLoadName, a3);
}

struct IncomingPacket
{
	BYTE gap0[8];
	DWORD packetType;
};

typedef struct {
	char* packetName;
	int		messageID;
} MessageIDStruct;


MessageIDStruct msgIDData[] = {
{ "Missing", 0 },
{ "BaseServerPacket", 1 },
{ "ClientFinishedLoading", 2 },
{ "SendSelfToClient", 3 },
{ "ClientIsReady", 4 },
{ "ZoneDoneSendingInitialData", 5 },
{ "ChatBase", 6 },
{ "ClientLogout", 7 },
{ "TargetClientNotOnline", 8 },
{ "CommandBase", 9 },
{ "AdminBase", 10 },
{ "ClientBeginZoning", 11 },
{ "CombatBase", 12 },
{ "VehicleRaceBase", 13 },
{ "MailBase", 14 },
{ "PlayerUpdateBase", 15 },
{ "AbilityBase", 16 },
{ "ClientUpdateBase", 17 },
{ "MiniGameBase", 18 },
{ "GroupsBase", 19 },
{ "EncounterBase", 20 },
{ "InventoryBase", 21 },
{ "SendZoneDetails", 22 },
{ "ReferenceDataBase", 23 },
{ "ObjectiveBase", 24 },
{ "DebugBase", 25 },
{ "UiBase", 26 },
{ "QuestBase", 27 },
{ "RewardBase", 28 },
{ "GameTimeSync", 29 },
{ "PetBase", 30 },
{ "PointOfInterestDefinitionRequest", 31 },
{ "PointOfInterestDefinitionReply", 32 },
{ "WorldTeleportRequest", 33 },
{ "TradeBase", 34 },
{ "EscrowGivePackage", 35 },
{ "EscrowGotPackage", 36 },
{ "UpdateEncounterDataCommon", 37 },
{ "RecipeBase", 38 },
{ "InGamePurchaseBase", 39 },
{ "QuickChatBase", 40 },
{ "ReportBase", 41 },
{ "LiveGamerBase", 42 },
{ "AcquaintanceBase", 43 },
{ "ClientServerShuttingDown", 44 },
{ "FriendBase", 45 },
{ "BroadcastBase", 46 },
{ "ClientKickedFromServer", 47 },
{ "UpdateClientSessionData", 48 },
{ "BugSubmissionBase", 49 },
{ "WorldDisplayInfo", 50 },
{ "MOTD", 51 },
{ "SetLocale", 52 },
{ "SetClientArea", 53 },
{ "ZoneTeleportRequest", 54 },
{ "TradingCardBase", 55 },
{ "WorldShutdownNotice", 56 },
{ "LoadWelcomeScreen", 57 },
{ "ShipCombatBase", 58 },
{ "AdminMiniGameBase", 59 },
{ "KeepAlive", 60 },
{ "ClientExitLaunchUri", 61 },
{ "ClientPath", 62 },
{ "ClientPendingKickFromServer", 63 },
{ "ClientMembershipActivation", 64 },
{ "LobbyBase", 65 },
{ "LobbyGameDefinitionBase", 66 },
{ "ShowSystemMessage", 67 },
{ "POIChangeMessage", 68 },
{ "ClientMetrics", 69 },
{ "FirstTimeEventBase", 70 },
{ "ClaimBase", 71 },
{ "ClientLog", 72 },
{ "IgnoreBase", 73 },
{ "SnoopedPlayerBase", 74 },
{ "PromotionalBase", 75 },
{ "AddClientPortraitCrc", 76 },
{ "ObjectiveTargetBase", 77 },
{ "CommerceSessionRequest", 78 },
{ "CommerceSessionResponse", 79 },
{ "TrackedEvent", 80 },
{ "ClientLoginFailed", 81 },
{ "LoginToUChat", 82 },
{ "ZoneSafeTeleportRequest", 83 },
{ "RemoteInteractionRequest", 84 },
{ "Missing", 85 },
{ "Missing", 86 },
{ "UpdateCamera", 87 },
{ "GuildBase", 88 },
{ "AdminGuildBase", 89 },
{ "BattleMagesBase", 90 },
{ "WorldToWorldBase", 91 },
{ "PerformAction", 92 },
{ "EncounterMatchmakingBase", 93 },
{ "ClientLuaMetrics", 94 },
{ "RepeatingActivityBase", 95 },
{ "ClientGameSettings", 96 },
{ "ClientTrialProfileUpsell", 97 },
{ "ActivityManagerBase", 98 },
{ "RequestSendItemDefinitionsToClient", 99 },
{ "InspectBase", 100 },
{ "AchievementBase", 101 },
{ "PlayerTitleBase", 102 },
{ "FotomatBase", 103 },
{ "UpdateUserAge", 104 },
{ "LootBase", 105 },
{ "ActionBarManagerBase", 106 },
{ "ClientTrialProfileUpsellRequest", 107 },
{ "PlayerUpdateJump", 108 },
{ "CoinStoreBase", 109 },
{ "InitializationParameters", 110 },
{ "ActivityBase", 111 },
{ "MountBase", 112 },
{ "ClientInitializationDetails", 113 },
{ "ClientAreaTimer", 114 },
{ "LoyaltyRewardBase", 115 },
{ "RatingBase", 116 },
{ "ClientActivityLaunchBase", 117 },
{ "ServerActivityLaunchBase", 118 },
{ "ClientFlashTimer", 119 },
{ "PlayerUpdatePosition", 120 },
{ "InviteAndStartMiniGame", 121 },
{ "PlayerUpdateFlourish", 122 },
{ "QuizBase", 123 },
{ "PlayerUpdatePositionOnPlatform", 124 },
{ "ClientMembershipVipInfo", 125 },
{ "TargetBase", 126 },
{ "GuideStoneBase", 127 },
{ "RaidsBase", 128 },
{ "VoiceBase", 129 },
{ "WeaponBase", 130 },
{ "Missing", 131 },
{ "FacilityBase", 132 },
{ "SkillsBase", 133 },
{ "LoadoutsBase", 134 },
{ "ExperienceBase", 135 },
{ "VehicleBase", 136 },
{ "GriefBase", 137 },
{ "SpotPlayer", 138 },
{ "FactionsBase", 139 },
{ "Synchronization", 140 },
{ "ResourceEventBase", 141 },
{ "CollisionBase", 142 },
{ "LeaderboardBase", 143 },
{ "PlayerUpdateManagedPosition", 144 },
{ "cPlayerUpdatePacketIdUpdateNetworkObjectComponents", 145 },
{ "PlayerUpdateVehicleWeapon", 146 },
{ "ProfileStatsBase", 147 },
{ "EquipmentBase", 148 },
{ "DefinitionFiltersBase", 149 },
{ "ContinentBattleInfo", 150 },
{ "GetContinentBattleInfo", 151 },
{ "GetRespawnLocations", 152 },
{ "WallOfDataBase", 153 },
{ "ThrustPadBase", 154 },
{ "ImplantsBase", 155 },
{ "InGamePurchase", 156 },
{ "MissionsBase", 157 },
{ "EffectsBase", 158 },
{ "RewardBuffsBase", 159 },
{ "AbilitiesBase", 160 },
{ "DeployableBase", 161 },
{ "Security", 162 },
{ "MapRegionBase", 163 },
{ "HudManagerBase", 164 },
{ "ClientPcDataBase", 165 },
{ "AcquireTimersBase", 166 },
{ "UpdateGuildTag", 167 },
{ "WarpgateBase", 168 },
{ "LoginQueueStatus", 169 },
{ "ServerPopulationInfo", 170 },
{ "GetServerPopulationInfo", 171 },
{ "PlayerUpdateVehicleCollision", 172 },
{ "PlayerStop", 173 },
{ "CurrencyBase", 174 },
{ "ItemsBase", 175 },
{ "PlayerUpdateAttachObject", 176 },
{ "PlayerUpdateDetachObject", 177 },
{ "ClientSettings", 178 },
{ "RewardBuffInfo", 179 },
{ "RewardBuffInfo", 180 },
{ "CaisBase", 181 },
{ "ZoneSettingBase", 182 },
{ "RequestPromoEligibilityUpdate", 183 },
{ "PromoEligibilityReply", 184 },
{ "MetaGameEventBase", 185 },
{ "RequestWalletTopupUpdate", 186 },
{ "StationCashActivePromoRequestUpdate", 187 },
{ "CharacterSlotBase", 188 },
{ "Missing", 189 },
{ "Missing", 190 },
{ "OperationBase", 191 },
{ "WordFilterBase", 192 },
{ "StaticFacilityInfoBase", 193 },
{ "ProxiedPlayerBase", 194 },
{ "ResistsBase", 195 },
{ "InGamePurchasingBase", 196 },
{ "BusinessEnvironments", 197 },
{ "EmpireScorePacketBase", 198 },
{ "CharacterSelectSessionRequest", 199 },
{ "CharacterSelectSessionResponse", 200 },
{ "StatsBase", 201 },
{ "ResourcesBase", 202 },
{ "Missing", 203 },
{ "ConstructionBase", 204 },
{ "SkyChanged", 205 },
{ "NavGenBase", 206 },
{ "LocksBase", 207 },
{ "RagdollBase", 208 },
{ "None", 209 },
{ "None", 210 },
{ "None", 211 },
{ "None", 212 },
{ "None", 213 },
{ "None", 214 },
{ "None", 215 },
{ "None", 216 },
{ "None", 217 },
{ "None", 218 },
{ "None", 219 },
{ "None", 220 },
{ "None", 221 },
{ "None", 222 },
{ "None", 223 },
{ "None", 224 },
{ "None", 225 },
{ "None", 226 },
{ "None", 227 },
{ "None", 228 },
{ "None", 229 },
{ "None", 230 },
{ "None", 231 },
{ "None", 232 },
{ "None", 233 },
{ "None", 234 },
{ "None", 235 },
{ "None", 236 },
{ "None", 237 },
{ "None", 238 },
{ "None", 239 },
{ "None", 240 },
{ "None", 241 },
{ "None", 242 },
{ "None", 243 },
{ "None", 244 },
{ "None", 245 },
{ "None", 246 },
{ "None", 247 },
{ "None", 248 },
{ "None", 249 },
{ "None", 250 },
{ "None", 251 },
{ "None", 252 },
{ "None", 253 },
{ "None", 254 },
{ "None", 255 },
};

char* tryPrintDescName(int descID) {
	int arrSize = sizeof(msgIDData) / sizeof(MessageIDStruct);
	if (descID > arrSize || descID < 0) return 0; //Can never be sure
	char* packetName = msgIDData[descID].packetName;
	printf("tryPrintDescName: %s\n", msgIDData[descID].packetName);
	return packetName;
}

static void(*handleIncomingPackets_orig)(void* thisPtr, char* packet, void* data, int dataLen, int a5);
static void handleIncomingPackets(void* thisPtr, char* packet, void* data, int dataLen, int a5) {
	for (int i = 0; i < 10; i++) { printf("\n"); }
	char packetId = *(char*)(packet + 8);

	//Ignore negative packets
	if (packetId > 0) {
		printf("packetType: %d - Return Address: %p\n", packetId, _ReturnAddress());
		printf("Calling hexDump\n");
		hexDump("data dump:", data, dataLen);

		char fPath[256];
		sprintf(fPath, "decrypted\\%s-in-route.bin", tryPrintDescName(packetId));
		FILE* f = fopen(fPath, "wb");
		fwrite(data, dataLen, 1, f);
		fclose(f);
	}

	handleIncomingPackets_orig(thisPtr, packet, data, dataLen, a5);
	for (int i = 0; i < 10; i++) { printf("\n"); }
}

//(void*, /*LuaVM**/ char* funcName, int, int)
static void(*executeLuaFunc_orig)(void* LuaVM, char* funcName, int a3, int a4);
static void executeLuaFuncStub(void* LuaVM, char* funcName, int a3, int a4) {
	void* retAddr = _ReturnAddress();
	if (retAddr != (void*)0x140123F56) {
		printf("executeLuaFuncStub: %s - Return Address: %p\n", funcName, retAddr);
	}
	executeLuaFunc_orig(LuaVM, funcName, a3, a4);
}

static void(*onLoginCompleteStub_orig)(void* thisPtr);
static void onLoginCompleteStub(void* thisPtr) {
	printf("onLoginCompleteStub: Return Address: %p\n", _ReturnAddress());
	onLoginCompleteStub_orig(thisPtr);
}

static void(*handleExternalPackets_orig)(void* a1, void* a2, unsigned int a3);
static void handleExternalPackets(void* a1, void* a2, unsigned int a3) {
	printf("handleExternalPackets: Return Address: %p\n", _ReturnAddress());
	handleExternalPackets_orig(a1, a2, a3);
}

class PacketHistoryEntry
{
public:
	virtual ~PacketHistoryEntry() = 0;

	unsigned char* mBuffer;
	UdpPlatformAddress mIp;
	int mPort;
	int mLen;
};

static void*(*handleExternalLoginPackets_orig)(void* a1, char* messageType, void* a3);
static void* handleExternalLoginPackets(void* a1, char* messageType, void* a3) {
	printf("handleExternalLoginPackets: messagetype: %d, Return Address: %p:\n", messageType, _ReturnAddress());
	return handleExternalLoginPackets_orig(a1, messageType, a3);
}

static void* (*parseElement_orig)(void* a1, char* element);
static void* parseElementStub(void* a1, char* element) {
	printf("parseElementStub: element: %s, Return Address: %p:\n", element, _ReturnAddress());
	return parseElement_orig(a1, element);
}

static void* (*g_origFuckingGarbageGameLoadZone)(void* a1, void* a2, void* a3, void* a4);
static void* fuckingGarbageGameLoadZone(void* a1, void* a2, void* a3, void* a4) {
	//printf("parseElementStub: element: %s, Return Address: %p:\n", element, _ReturnAddress());
	return g_origFuckingGarbageGameLoadZone(a1, a2, a3, a4);
}

static void* (*g_origOurSehFuncZoneload)(void* a1, void* a2, int a3);
static void* OurSehFuncZoneload(void* a1, void* a2, int a3) {
	//printf("parseElementStub: element: %s, Return Address: %p:\n", element, _ReturnAddress());
	void* ret = (void*)true;
	__try
	{
		ret = g_origOurSehFuncZoneload(a1, a2, a3);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		printf_s("OurSehFuncZoneload excepted, caught and returned.\n");
	}
	return ret;
}

static void* (*g_origOurSehFuncZoneload2)(void* a1, void* a2);
static void* OurSehFuncZoneload2(void* a1, void* a2) {
	//printf("parseElementStub: element: %s, Return Address: %p:\n", element, _ReturnAddress());
	void* ret = (void*)true;
	__try
	{
		ret = g_origOurSehFuncZoneload2(a1, a2);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		printf_s("OurSehFuncZoneload2 excepted, caught and returned.\n");
	}
	return ret;
}

static void* (*g_origSpeedTreeRelated)(void* a1, void* a2, intptr_t a3, intptr_t a4);
bool speedTreeRelated(void* a1, void* a2, intptr_t a3, intptr_t a4) {
	bool returnVal = true;
	__try
	{
		returnVal = g_origSpeedTreeRelated(a1, a2, a3, a4);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		printf_s("speedTreeRelated excepted, caught and returned.\n");
	}
	return returnVal;
}

static intptr_t (*g_origWaitForWorldReady)(void* a1);
intptr_t WaitForWorldReady(void* a1) {
	intptr_t returnVal = g_origWaitForWorldReady(a1);
	return 1;
}

static bool(*File__Open_orig)(void* a1, char* filename, int a3, int a4);
bool File__Open(void* a1, char* filename, int a3, int a4) {
	printf("File::Open tried to open %s\n", filename);
	bool open = File__Open_orig(a1, filename, a3, a4);
	return open;
}


static void*(*dx9InitVertex_Orig)(void* a1, int a2, int a3, void* a4);
void* dx9InitVertex(void* a1, int a2, int a3, void* a4) {
	void* address = (void*)true;
	__try
	{
		void* address = dx9InitVertex_Orig(a1, a2, a3, a4);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		printf_s("dx9InitVertex excepted, caught and returned.\n");
	}
	return address;
}

//handleIncomingPackets@<al>(__int64 a1@<rcx>, IncomingPacket *a2@<rdx>, char *a3@<r8>, int a4@<r9d>, float a5@<xmm0>, int reason?)
bool VCPatcher::Init()
{
	tryAllocConsole();

	//hook::return_function_vp(0x140414C62);
	//hook::vp::jump(0x140EC6EA0, ReturnTrue);
	MH_CreateHook((char*)0x140EEDE90, handleIncomingPackets, (void**)&handleIncomingPackets_orig);
	//MH_CreateHook((char*)0x140DBE4B0, logFuncCustom3, (void**)&logFuncCustom_orig3); //Logs clock timeeee

	//Net Routing
	/*
	hook::iat("wsock32.dll", CfxSend, 19);
	hook::iat("wsock32.dll", CfxSendTo, 20);
	hook::iat("wsock32.dll", CfxRecvFrom, 17);
	
	hook::iat("wsock32.dll", CfxBind, 2);
	g_oldSelect = hook::iat("wsock32.dll", CfxSelect, 18);
	hook::iat("wsock32.dll", CfxGetSockName, 6);
	hook::iat("wsock32.dll", CfxWSAStartup, 115);
	*/

	MH_CreateHookApi(L"kernel32.dll", "ExitProcess", ExitProcessReplacement, nullptr);

	MH_EnableHook(MH_ALL_HOOKS);

	return true;
}

/*

static char*(*g_origGetShutdownReasonString)(int index);
static char* GetShutdownReasonString(int index)
{
return g_origGetShutdownReasonString(index);
}

1404CDAE0

*/

bool VCPatcher::PatchResolution(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	pPresentationParameters->Windowed = true;
	pPresentationParameters->Flags = 0;
	pPresentationParameters->FullScreen_RefreshRateInHz = 0;
	//pPresentationParameters->FullScreen_PresentationInterval = 0;

	SetWindowPos(pPresentationParameters->hDeviceWindow, HWND_NOTOPMOST, 0, 0, pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight, SWP_SHOWWINDOW);
	SetWindowLong(pPresentationParameters->hDeviceWindow, GWL_STYLE, WS_POPUP | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE);
	return true;
}

void hexDump(const char* desc, const void* addr, const int len) {
	int i;
	unsigned char buff[17];
	const unsigned char* pc = (const unsigned char*)addr;

	// Output description if given.

	if (desc != NULL)
		printf("%s:\n", desc);

	// Length checks.

	if (len == 0) {
		printf("  ZERO LENGTH\n");
		return;
	}
	else if (len < 0) {
		printf("  NEGATIVE LENGTH: %d\n", len);
		return;
	}

	// Process every byte in the data.

	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Don't print ASCII buffer for the "zeroth" line.

			if (i != 0)
				printf("  %s\n", buff);

			// Output the offset.

			printf("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf(" %02x", pc[i]);

		// And buffer a printable ASCII character for later.

		if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
			buff[i % 16] = '.';
		else
			buff[i % 16] = pc[i];
		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.

	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}

	// And print the final ASCII buffer.

	printf("  %s\n", buff);
}

static struct MhInit
{
	MhInit()
	{
		MH_Initialize();
	}
} mhInit;