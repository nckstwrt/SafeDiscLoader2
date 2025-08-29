#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include "version.h"
#include <filesystem>
#include "NString.h"
#include <stdio.h>
#include <conio.h>
#include <psapi.h>
#include <eh.h>
#include "minhook/MinHook.h"
#include "winternl.h"
#include "MemHack.h"
#include "Config.h"
#include <map>

// http://redump.org/discs/quicksearch/SafeDisc/protection/only
// 
// Call of Duty (Original): 3.10.20
// Call of Duty (Patched - 1.4): 3.15.11 
// Call of Duty - United Offensive: 3.20.22
// Call of Duty - Modern Warfare: 4.81.00
// The Sims 2: 3.20.20 (has two safedisc cd checks!)
// Call of Duty - World at War: 4.90.10
// Need for Speed: Underground: 2.90.40 (Needs VersionInjector)
// Football Manager 2005 (Original - 5.0): 3.20.22
// Football Manager 2005 (Patched - 5.0.5): 4.00.02
// Harry Potter and the Prisoner of Azkaban: 3.20.22
// Harry Potter and the Chamber of Secrets: 2.80.10 (Needs VersionInjector)
// The Battle for Middle-earth II: 4.60.00 ? Normal version string missing. Need to use VersionInjector on the Launcher.
// The Battle for Middle-earth: 4.00.01 ? Normal version string missing. Need to use VersionInjector on the Launcher. (On Win 11 needs special Options.ini to work)
// Football Manager 2006 (6.0.2): 4.60.00
// Football Manager 2006 (6.2.3): 4.60.00
// Football Manager 2007 (7.0.2): 4.60.00
// Need For Speed Underground 2: 3.05.10 ? Normal version string missing. Still needs CD 2 (but not Safedisc check) - looks for bin.dat off the disc
// Command & Conquer: Renegade: 2.51.21 (Needs VersionInjector - still crashes due to getting system info. Issue with game on modern systems)
// Operation Flashpoint: Cold War Crisis: 2.40.10 (original) 1.46 (2.50.20) - Even it's patchers need safedisc (and therefore VersionInjector) to work.
// Doom 3: 3.20.22
// Far Cry: 3.15.10
// Max Payne (1.0): 2.30.33
// Settlers IV: 2.10.30
// Delta Force - Land Warrior: 2.05.30 (cd check for volume name and CDFS filesystem)
// Eurofighter Typhoon: 2.10.30
// Battlefield 1942: 2.60.52 - checks for the existence of bfdist.vlu on the CDROM
// Kohan: 2.60.52 - Checks the CDROM's Volumename is KOHAN_IS
// Battlefield 2: 4.00.01

#define NTSTATUS int
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)    // ntsubauth
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#define EJECT_DEBUGGER_TIMEOUT (1*1000)
//#define USE_SDLOADER

#include "Utils.h"
#include "Config.h"

bool logCreateFile = false;
Config config;

typedef NTSTATUS(WINAPI* NtDeviceIoControlFile_typedef)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
NtDeviceIoControlFile_typedef NtDeviceIoControlFile_Orig;
static unsigned int ioctlCodeMain = 0xef002407;
BOOL ProcessMainIoctl(LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize);

typedef BOOL(WINAPI* CreateProcessA_typedef)(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
CreateProcessA_typedef CreateProcessA_Orig;

typedef BOOL(WINAPI* CreateProcessW_typedef)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
CreateProcessW_typedef CreateProcessW_Orig;

typedef HMODULE(WINAPI* LoadLibraryA_typedef)(LPCSTR lpLibFileName);
LoadLibraryA_typedef LoadLibraryA_Orig;

typedef HANDLE(WINAPI* CreateFileA_typedef)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
CreateFileA_typedef CreateFileA_Orig;
CreateFileA_typedef CreateFileA_Orig_KBase;

typedef HANDLE(WINAPI* CreateFileW_typedef)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
CreateFileW_typedef CreateFileW_Orig;

typedef BOOL(WINAPI* FreeLibrary_typedef)(HMODULE hLibModule);
FreeLibrary_typedef FreeLibrary_Orig;

typedef BOOL(WINAPI* ReadProcessMemory_typedef)(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T * lpNumberOfBytesRead);
ReadProcessMemory_typedef ReadProcessMemory_Orig;

typedef BOOL(WINAPI* WriteProcessMemory_typedef)(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T * lpNumberOfBytesWritten);
WriteProcessMemory_typedef WriteProcessMemory_Orig;

typedef DWORD (WINAPI* GetLogicalDrives_typedef)();
GetLogicalDrives_typedef GetLogicalDrives_Orig;

typedef UINT(WINAPI* GetDriveTypeA_typedef)(LPCSTR lpRootPathName);
GetDriveTypeA_typedef GetDriveTypeA_Orig;

typedef BOOL(WINAPI* GetVolumeInformationA_typedef)(LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize);
GetVolumeInformationA_typedef GetVolumeInformationA_Orig;

HMODULE version_dll;

#define WRAPPER_FUNC(name) o##name = GetProcAddress(version_dll, ###name)

#ifdef _M_AMD64
#pragma warning (disable: 4081)
#define STRINGIFY(name) #name
#define EXPORT_FUNCTION comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
#define WRAPPER_GENFUNC(name) \
    FARPROC o##name; \
    __declspec(dllexport) void WINAPI _##name() \
    { \
        __pragma(STRINGIFY(EXPORT_FUNCTION)); \
        o##name(); \
    }
#else
#define WRAPPER_GENFUNC(name) \
	FARPROC o##name; \
	__declspec(naked) void _##name() \
	{ \
		__asm jmp[o##name] \
	}
#endif

WRAPPER_GENFUNC(GetFileVersionInfoA);
WRAPPER_GENFUNC(GetFileVersionInfoByHandle);
WRAPPER_GENFUNC(GetFileVersionInfoExW);
WRAPPER_GENFUNC(GetFileVersionInfoExA);
WRAPPER_GENFUNC(GetFileVersionInfoSizeA);
WRAPPER_GENFUNC(GetFileVersionInfoSizeExA);
WRAPPER_GENFUNC(GetFileVersionInfoSizeExW);
WRAPPER_GENFUNC(GetFileVersionInfoSizeW);
WRAPPER_GENFUNC(GetFileVersionInfoW);
WRAPPER_GENFUNC(VerFindFileA);
WRAPPER_GENFUNC(VerFindFileW);
WRAPPER_GENFUNC(VerInstallFileA);
WRAPPER_GENFUNC(VerInstallFileW);
WRAPPER_GENFUNC(VerLanguageNameA);
WRAPPER_GENFUNC(VerLanguageNameW);
WRAPPER_GENFUNC(VerQueryValueA);
WRAPPER_GENFUNC(VerQueryValueW);

void load_version() {
	char systemPath[MAX_PATH];
	GetSystemDirectoryA(systemPath, MAX_PATH);
	strcat_s(systemPath, "\\version.dll");
	version_dll = LoadLibraryA(systemPath);	

	if (!version_dll) return;
	
	WRAPPER_FUNC(GetFileVersionInfoA);
	WRAPPER_FUNC(GetFileVersionInfoByHandle);
	WRAPPER_FUNC(GetFileVersionInfoExW);
	WRAPPER_FUNC(GetFileVersionInfoExA);
	WRAPPER_FUNC(GetFileVersionInfoSizeA);
	WRAPPER_FUNC(GetFileVersionInfoSizeExW);
	WRAPPER_FUNC(GetFileVersionInfoSizeExA);
	WRAPPER_FUNC(GetFileVersionInfoSizeW);
	WRAPPER_FUNC(GetFileVersionInfoW);
	WRAPPER_FUNC(VerFindFileA);
	WRAPPER_FUNC(VerFindFileW);
	WRAPPER_FUNC(VerInstallFileA);
	WRAPPER_FUNC(VerInstallFileW);
	WRAPPER_FUNC(VerLanguageNameA);
	WRAPPER_FUNC(VerLanguageNameW);
	WRAPPER_FUNC(VerQueryValueA);
	WRAPPER_FUNC(VerQueryValueW);
}

typedef BOOL(WINAPI* DebugActiveProcess_typedef)(DWORD dwProcessId);
DebugActiveProcess_typedef DebugActiveProcess_Orig;

typedef BOOL(WINAPI* WaitForDebugEvent_typedef)(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds);
WaitForDebugEvent_typedef WaitForDebugEvent_Orig;

HANDLE hDebuggerThread = INVALID_HANDLE_VALUE;
DWORD dwSavedProcessId = 0;

bool IsKeyComboPressed(char c = 'K') 
{
	// VK_CONTROL = Ctrl // VK_MENU = Alt // 'K' = 0x4B
	SHORT ctrlState = GetAsyncKeyState(VK_CONTROL);
	SHORT altState = GetAsyncKeyState(VK_MENU);
	SHORT kState = GetAsyncKeyState(c);
	return (ctrlState & 0x8000) &&		// 0x8000 bit set means key is currently down
		(altState & 0x8000) &&
		(kState & 0x8000);
}

BOOL WINAPI WaitForDebugEvent_Hook(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds)
{
	if (hDebuggerThread != INVALID_HANDLE_VALUE && hDebuggerThread == GetCurrentThread() && dwMilliseconds == INFINITE)
	{
		while (true)
		{
			BOOL ret = WaitForDebugEvent_Orig(lpDebugEvent, EJECT_DEBUGGER_TIMEOUT);
			if (ret)
				return ret;

			if (IsKeyComboPressed('K'))
			{
				MessageBox(0, "We gonna kill that debugger now", "Killer", 0);
				((DebugActiveProcess_typedef)GetProcAddress(LoadLibrary("Kernel32.dll"), "DebugActiveProcessStop"))(dwSavedProcessId);
				ExitProcess(0);
			}
		}
	}
	else
		return WaitForDebugEvent_Orig(lpDebugEvent, dwMilliseconds);
}

BOOL WINAPI DebugActiveProcess_Hook(DWORD dwProcessId)
{
	hDebuggerThread = GetCurrentThread();
	log("DebugActiveProcess: %d (0x%X) by Thread: %X\n", dwProcessId, dwProcessId, (DWORD)hDebuggerThread);

	dwSavedProcessId = dwProcessId;
	return DebugActiveProcess_Orig(dwProcessId);
}

NTSTATUS NTAPI NtDeviceIoControlFile_Hook(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	//log("Entering NtDeviceIoControlFile_Hook\n");
	// all IOCTLs will pass through this function, but it's probably fine since secdrv uses unique control codes
	if (IoControlCode == ioctlCodeMain) {
		//log("Override IoStatusBlock->Status Handle: %d Status: %d\n", (DWORD)FileHandle, IoStatusBlock->Status);
		if (ProcessMainIoctl(InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength))
		{
			IoStatusBlock->Information = OutputBufferLength;
			IoStatusBlock->Status = STATUS_SUCCESS;
		}
		else
			IoStatusBlock->Status = STATUS_UNSUCCESSFUL;
	}
	else if (IoControlCode == 0xCA002813)
	{
	//	logc(FOREGROUND_RED, "IOCTL 0xCA002813 unhandled (please report!)");
		IoStatusBlock->Status = STATUS_UNSUCCESSFUL;
	}
	else
	{
		// not a secdrv request, pass to original function
	//	log("Returning Orig IoStatusBlock->Status Handle: %d ControlCode: %d Status: %d\n", (DWORD)FileHandle, IoControlCode, IoStatusBlock->Status);
		NTSTATUS ret = NtDeviceIoControlFile_Orig(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
	//	log("NtDeviceIoControlFile_Orig Complete\n");
		return ret;
	}
	
	// log("Returning Overriden IoStatusBlock->Status Handle: %d ControlCode: %08X Status: %d\n", (DWORD)FileHandle, IoControlCode, IoStatusBlock->Status);
	return IoStatusBlock->Status;
}

HMODULE hOurModule;

int InjectSelf(DWORD pid)
{
	char szSecDrvEmuDLLPath[MAX_PATH];

	GetModuleFileName(hOurModule, szSecDrvEmuDLLPath, MAX_PATH);

	log("Injecting DLL %s\n", szSecDrvEmuDLLPath);

	// Open Process
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL) // Not INVALID_HANDLE_VALUE...Strangely
	{
		log("Process found, but cannot open handle\n");
		return -1;
	}

	// Get the address of our LoadLibraryA function. This is assuming our address for LoadLibrary will be the same as our target processes 
	LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

	// Get full path name of the target dll
	char szPath[MAX_PATH];
	GetFullPathNameA(szSecDrvEmuDLLPath, MAX_PATH, szPath, NULL);

	// Create Memory in Target Process to hold the DLL's filename
	LPVOID newMemory = (LPVOID)VirtualAllocEx(hProcess, NULL, strlen(szPath) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (newMemory == NULL)
	{
		log("Could not allocate memory inside the target process\n");
		return -1;
	}

	// Write the fullpath filename into the target process
	BOOL bWritten = WriteProcessMemory(hProcess, newMemory, szPath, strlen(szPath) + 1, NULL);
	if (bWritten == 0)
	{
		log("There were no bytes written to the process's address space.\n");
		return -1;
	}

	// Create Remote Thread to run LoadLibrary with our fullpath
	HANDLE hNewThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, newMemory, NULL, NULL);
	if (hNewThread == NULL)
	{
		log("Could not create remote thread in target process\n");
	}

	// Wait for it to run
	WaitForSingleObject(hNewThread, INFINITE);

	// Clean up
	CloseHandle(hNewThread);
	CloseHandle(hProcess);

	log("Injecting into pid %d\n", pid);

	return 0;
}

HANDLE CheckForSecDrv(LPCSTR lpFileName, CreateFileA_typedef CreateFileOrig)
{
	HANDLE ret = INVALID_HANDLE_VALUE;

	if (lpFileName)
	{
		if (!lstrcmpiA(lpFileName, "\\\\.\\Secdrv") || !lstrcmpiA(lpFileName, "\\\\.\\Global\\SecDrv"))// || strstr(lpFileName, "bfdist.vlu") != NULL)
		{
			// we need to return a handle when secdrv is opened, so we just open the null device to get an unused handle
			ret = CreateFileOrig("NUL", GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (ret == INVALID_HANDLE_VALUE)
				log("unable to obtain a dummy handle for secdrv");
			/*else
				log("CreateFileA_Hook - returning NUL for %s Handle: %d\n", lpFileName, (DWORD)dummyHandle);*/
		}
	}

	return ret;
}

HANDLE WINAPI CreateFileA_Hook(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	std::string strFileName;
	if (lpFileName)
	{
		strFileName = config.GetFileMapping(lpFileName);
		lpFileName = strFileName.c_str();
	}

	HANDLE ret = CheckForSecDrv(lpFileName, CreateFileA_Orig);
	if (ret == INVALID_HANDLE_VALUE)
		ret = CreateFileA_Orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (logCreateFile)
		log("CreateFileA_Hook Hook - lpFileName: %s ret: %08X\n", lpFileName == NULL ? "!NULL!" : lpFileName, ret);

	return ret;
}

HANDLE WINAPI CreateFileA_Hook_KBase(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	std::string strFileName;
	if (lpFileName)
	{
		strFileName = config.GetFileMapping(lpFileName);
		lpFileName = strFileName.c_str();
	}
	HANDLE ret = CheckForSecDrv(lpFileName, CreateFileA_Orig_KBase);
	if (ret == INVALID_HANDLE_VALUE)
		ret = CreateFileA_Orig_KBase(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (logCreateFile)
		log("CreateFileA_Hook_KBase Hook - lpFileName: %s ret: %08X\n", lpFileName == NULL ? "!NULL!" : lpFileName, ret);

	return ret;
}

HANDLE WINAPI CreateFileW_Hook(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	if (lpFileName == NULL)
		return INVALID_HANDLE_VALUE;
	
	NString strFileName(lpFileName);
	if (logCreateFile)
		logc(FOREGROUND_BLUE, "CreateFileW_Hook - Testing %s\n", (LPCSTR)strFileName);
	
	return CreateFileW_Orig(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);;
}

BOOL WINAPI CreateProcessA_Hook(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    log("CreateProcessA Hook (%s)\n", (lpApplicationName == NULL) ? "" : lpApplicationName);
	
	// if the process isn't created suspended, set the flag so we can inject hooks
	const DWORD isCreateSuspended = dwCreationFlags & CREATE_SUSPENDED;
	if (!isCreateSuspended) dwCreationFlags |= CREATE_SUSPENDED;

	if (!CreateProcessA_Orig(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
		return FALSE;

	// log("Hooking and calling Process at CreateProcessA_Hook\n");

	InjectSelf(lpProcessInformation->dwProcessId);

	if (!isCreateSuspended)
		ResumeThread(lpProcessInformation->hThread);

	return TRUE;
}


BOOL WINAPI CreateProcessW_Hook(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	log("CreateProcessW Hook\n");

	// if the process isn't created suspended, set the flag so we can inject hooks
	const DWORD isCreateSuspended = dwCreationFlags & CREATE_SUSPENDED;
	if (!isCreateSuspended) dwCreationFlags |= CREATE_SUSPENDED;

	if (!CreateProcessW_Orig(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
		return FALSE;

	InjectSelf(lpProcessInformation->dwProcessId);

	if (!isCreateSuspended)
		ResumeThread(lpProcessInformation->hThread);

	return TRUE;
}

BYTE PatchCDPtr[9] = { 0xB8, 0x50, 0x00, 0x02, 0x01, 0x90, 0x90, 0x90, 0x90 };
DWORD SafeDiscVersion = 0, SafeDiscSubVersion = 0, SafeDiscRevision = 0;
BOOL bLoadeddf394b = FALSE;
enum AuthServState
{
	AuthServState_NotYetLoaded = 0,
	AuthServState_Loaded = 1,
	AuthServState_Unloaded = 2,
	AuthServState_Reloaded = 3
};
AuthServState LoadedAuthServ = AuthServState_NotYetLoaded;
HINSTANCE hInstanceAuthServ = NULL;
HINSTANCE hInstanceSecServ = NULL;

BOOL WINAPI FreeLibrary_Hook(HINSTANCE hModule)
{
	if (hInstanceAuthServ == hModule)
	{
		if (SafeDiscVersion == 2 && SafeDiscSubVersion == 60 && SafeDiscRevision == 52)
		{
			logc(FOREGROUND_RED, "HACK FreeLibrary_Hook: Denying unloading AuthServ.dll\n");
			LoadedAuthServ = AuthServState_Unloaded;
			return TRUE;	// HACK Can't stop it from free'ing without breaking it for Sims 2. But is needed for BF1942 currently.
		}
		else
		{
			logc(FOREGROUND_RED, "FreeLibrary_Hook: Unloading AuthServ.dll\n");
			LoadedAuthServ = AuthServState_Unloaded;
		}
	}
	if (hInstanceSecServ == hModule)
	{
		logc(FOREGROUND_RED, "FreeLibrary_Hook: Unloading SecServ.dll\n");
	}
	return FreeLibrary_Orig(hModule);
}

DWORD DecryptFunc = 0;
DWORD StealCRCTablePtr = 0;
DWORD savedFuncCall = 0;
DWORD HookDecodeTableAddr = -1L, RelativeGrabberCall = 0;;
DWORD FirstCopy = 0, SecondCopy = 0, ThirdCopy = 0;
int amountToCopy = 0;
BYTE* ThirdKey = new BYTE[1024];

void __declspec(naked) WINAPI HookCDCheck()
{
	__asm
	{
		pushad
	}
	log("HookCDCheck Called! %08X\n", (DWORD)HookCDCheck);
	memcpy((void*)FirstCopy, ThirdKey, amountToCopy);
	__asm
	{
		popad;
		mov eax, 0x01020050;	// Should always be this value? 
		ret;
	}
}

void WINAPI Grabber()
{
	__asm 
	{
		mov [StealCRCTablePtr], ecx;
		call [savedFuncCall];
	}

	logc(FOREGROUND_GREEN, "Grabber: StealCRCTablePtr = %08X\n", StealCRCTablePtr);
}

void WINAPI CallDecrypt(DWORD tableNo)
{
	__asm
	{
		push [tableNo];
		call [DecryptFunc]; 
		pop eax;
	}
}

HMODULE WINAPI LoadLibraryA_Hook(LPCSTR lpLibFileName)
{
	static DWORD TableClass = 0;

	HMODULE ret = LoadLibraryA_Orig(lpLibFileName);
	
	if (strstr(lpLibFileName, "~"))
		log("LoadLibraryA_Hook: %s %08X\n", lpLibFileName == NULL ? "NULL" : lpLibFileName, ret);

	if (strstr(lpLibFileName, "df394b.tmp") != NULL && bLoadeddf394b == FALSE)	// SecServ.dll
	{
		bLoadeddf394b = TRUE;
		hInstanceSecServ = ret;
		log("LoadLibraryA_Hook: %s %08X\n", lpLibFileName == NULL ? "NULL" : lpLibFileName, ret);
		PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)ret;
		PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);

		DWORD StartAddr = (DWORD)ret;
		DWORD EndAddr = StartAddr + pinh->OptionalHeader.SizeOfImage;

		DWORD TablesInitAddr = FindHexString(StartAddr, EndAddr, "6A068BC8E8????????EB0233C0A3????????C3");
		log("TablesInitAddr = %08X\n", TablesInitAddr);

		//GetKey();

		if (TablesInitAddr != -1L)
		{
			TableClass = *(DWORD*)(TablesInitAddr + 0xE);
			log("TableClass = %08X\n", TableClass);

			DWORD CdCheckDllLoadAddr = FindHexString(StartAddr, EndAddr, "83C43056FF15????????3BC7A3????????75056633C0EB12", "CdCheckDllLoadAddr");
			log("CdCheckDllLoadAddr = %08X\n", CdCheckDllLoadAddr);

			DWORD HookDecodeTable = FindHexString(StartAddr, EndAddr, "83EC1C8BCC8965FCFF7508E8????????8B0D????????E8????????8BC8", "HookDecodeTable");
			if (HookDecodeTable != -1L)
			{
				HookDecodeTableAddr = HookDecodeTable + 0x1E;
				DecryptFunc = HookDecodeTable - 4;
				logc(FOREGROUND_GREEN, "HookDecodeTable = %08X (call addr bytes: %08X)\n", HookDecodeTable, HookDecodeTableAddr);
				logc(FOREGROUND_GREEN, "Grabber function address: %08X\n", (DWORD)&Grabber);
				logc(FOREGROUND_GREEN, "DecryptFunc = %08X\n", DecryptFunc);

				RelativeGrabberCall = ((DWORD)&Grabber) - HookDecodeTableAddr - 4;

				log("RelativeGrabberCall = %08X\n", RelativeGrabberCall);
		
				if (savedFuncCall == 0)
				{
					savedFuncCall = HookDecodeTableAddr + (*((DWORD*)HookDecodeTableAddr)) + 4;
					log("savedFuncCall = %08X\n", savedFuncCall);
		
					UnProtect_memcpy((void*)HookDecodeTableAddr, (void*)&RelativeGrabberCall, 4);
				}
			}
		}
		
		//GetKey();
	}
	else
	if ((strstr(lpLibFileName, "~de") != NULL || strstr(lpLibFileName, "~ef") != NULL) && EndsInTmp(lpLibFileName) && TableClass != 0)  // AuthServ.dll
	{
		hInstanceAuthServ = ret;

		PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)ret;
		PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);

		DWORD StartAddr = (DWORD)ret;
		DWORD EndAddr = StartAddr + pinh->OptionalHeader.SizeOfImage;
		
		if (LoadedAuthServ == AuthServState_Unloaded)	// It it has been unloaded then we are being used to just do additional CD Checks (so don't need to redo everything else)
		{
			logc(FOREGROUND_GREEN, "LoadLibraryA_Hook: Reloading AuthServ.dll for possible CD Checks\n");
			
			DWORD CdCheck2CallAddr = FindHexString(StartAddr, EndAddr, "52A1????????FF501083C40C8B4DF88901E9", "CdCheck2CallAddr");
			if (CdCheck2CallAddr != -1L)
			{
				UnProtect_memcpy((BYTE*)(CdCheck2CallAddr + 1), PatchCDPtr, 8);
				if (hInstanceSecServ != NULL)
				{
					pidh = (PIMAGE_DOS_HEADER)hInstanceSecServ;
					pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);

					DWORD StartAddrSecServ = (DWORD)hInstanceSecServ;
					DWORD EndAddrSecServ = StartAddrSecServ + pinh->OptionalHeader.SizeOfImage;
					DWORD CdCheck2DataCheckAddr = FindHexString(StartAddrSecServ, EndAddrSecServ, "33C93945FC0F94C18BC15F5E5BC9C3", "CdCheck2DataCheckAddr");
					if (CdCheck2DataCheckAddr != -1L)
					{
						DWORD CdCheck2DataCheckAddrFix =  CdCheck2DataCheckAddr + 5;
						logc(FOREGROUND_GREEN, "CdCheck2DataCheckAddr: %08X (CdCheck2DataCheckAddrFix: %08X)\n", CdCheck2DataCheckAddr, CdCheck2DataCheckAddrFix);
						BYTE PatchEAXTo1[5] = { 0xB8, 0x01, 0x00, 0x00, 0x00 };
						UnProtect_memcpy((BYTE*)CdCheck2DataCheckAddrFix, PatchEAXTo1, 5);

						//GetKey(true);
					}
				}
			}
		}
		else
		if (LoadedAuthServ == AuthServState_NotYetLoaded)
		{
			LoadedAuthServ = AuthServState_Loaded;
			logc(FOREGROUND_GREEN, "CallDecrypt: %08X\n", (DWORD)&CallDecrypt);

			log("LoadLibraryA_Hook: %s %08X\n", lpLibFileName == NULL ? "NULL" : lpLibFileName, ret);
			

			DWORD TableRawKeysAddr_v40 = FindHexString(StartAddr, EndAddr, "750233DB8B0768????????50E8", "TableRawKeysAddr_v40");
			log("TableRawKeysAddr_v40: %08X\n", TableRawKeysAddr_v40);

			DWORD TableRawKeysAddr_v45 = FindHexString(StartAddr, EndAddr, "750233ED8B3B68????????E8", "TableRawKeysAddr_v45");
			log("TableRawKeysAddr_v45: %08X\n", TableRawKeysAddr_v45);

			if (TableRawKeysAddr_v40 != -1L || TableRawKeysAddr_v45 != -1L)
			{
				DWORD RawKeysTable;

				if (TableRawKeysAddr_v40 != -1L)
					RawKeysTable = (*(DWORD*)(TableRawKeysAddr_v40 + 7)) + 0x50;
				else
					RawKeysTable = (*(DWORD*)(TableRawKeysAddr_v45 + 7)) + 0x50;

				log("RawKeysTable: %08X\n", RawKeysTable);

				DWORD TableDWORD1 = *(DWORD*)(RawKeysTable);
				DWORD TableDWORD2 = *(DWORD*)(RawKeysTable + 0x555);
				log("TableDWORD1: %08X (at: %08X)\n", TableDWORD1, RawKeysTable);
				log("TableDWORD2: %08X (at: %08X)\n", TableDWORD2, RawKeysTable + 0x555);

				DWORD xorKey = TableDWORD1 ^ TableDWORD2;
				log("Create() -> xor_key = : %08X\n", xorKey);

				DWORD Table3Offset;
			
				switch (SafeDiscVersion)
				{
				default:
				case 4:
					switch (SafeDiscSubVersion)
					{
					case 81:
						Table3Offset = 0xAA6;
						break;
					default:
						Table3Offset = 0xAAA;
						break;
					}
					break;
				case 3:
					Table3Offset = 0xA82;
					break;
				case 2:
					Table3Offset = 0xA86;
					break;
				}

				DWORD TableDWORD3 = *(DWORD*)(RawKeysTable + Table3Offset);
				log("TableDWORD3: %08X (at: %08X) (%X)\n", TableDWORD3, RawKeysTable + Table3Offset, Table3Offset);

				log("MorphTableM() -> KeyN[0]DW = %08X\n", TableDWORD3 ^ xorKey);

				// GetKey();

				DWORD TableClassAddr = *(DWORD*)TableClass;
				logc(FOREGROUND_GREEN, "TableClassAddr: %08X\n", TableClassAddr);

				if (TableRawKeysAddr_v40 != -1L)
				{
					DWORD TablePtr = *(DWORD*)(TableClassAddr + 0xc);
					log("TablePtr = %08X\n", TablePtr);

					FirstCopy = *(DWORD*)(TablePtr + 0x34);
					SecondCopy = *(DWORD*)(TablePtr + 0x24);
					ThirdCopy = *(DWORD*)(TablePtr + 0x44);
					amountToCopy = 1024;
				}

				if (HookDecodeTableAddr != -1L)
				{
					BYTE HookTableBackup[4];
					UnProtect_memcpy(HookTableBackup, (void*)HookDecodeTableAddr, 4);
					UnProtect_memcpy((void*)HookDecodeTableAddr, (void*)&RelativeGrabberCall, 4);

					CallDecrypt(3);
					logc(FOREGROUND_GREEN, "Called CallDecrypt(3): %08X\n", StealCRCTablePtr);
					FirstCopy = *(DWORD*)(StealCRCTablePtr + 4);
					CallDecrypt(2);
					logc(FOREGROUND_GREEN, "Called CallDecrypt(2): %08X\n", StealCRCTablePtr);;
					SecondCopy = *(DWORD*)(StealCRCTablePtr + 4);
					CallDecrypt(4);
					logc(FOREGROUND_GREEN, "Called CallDecrypt(4): %08X\n", StealCRCTablePtr);
					ThirdCopy = *(DWORD*)(StealCRCTablePtr + 4);

					UnProtect_memcpy((void*)HookDecodeTableAddr, HookTableBackup, 4);

					amountToCopy = 1014;
				}
				logc(FOREGROUND_GREEN, "FirstCopy = %08X SecondCopy = %08X ThirdCopy = %08X\n", FirstCopy, SecondCopy, ThirdCopy);

				if (FirstCopy == 0 || SecondCopy == 0 || ThirdCopy == 0)
				{
					log("Failed to get FirstCopy, SecondCopy or ThirdCopy\n");
					return ret;
				}
			
#ifndef USE_SDLOADER	
				// Create the important key - but do not write it here. In Football Manager 2006 it is detected if you write it too early
				// Then it changes the version to 6.2.3, currency to Thai Baht, saving to daily, etc.
				// Instead hook the CD Check function and write it there instead
				memcpy(ThirdKey, (void*)(RawKeysTable + Table3Offset), 1024);
				for (int i = 0; i < 1024; i += 4)
				{
					*((DWORD*)&ThirdKey[i]) ^= xorKey;
				}

				memcpy((void*)SecondCopy, (void*)(RawKeysTable + Table3Offset), amountToCopy);
				memcpy((void*)ThirdCopy, (void*)(RawKeysTable + Table3Offset), amountToCopy);
			
#endif
				DWORD CdCheckCallAddr = FindHexString(StartAddr, EndAddr, "EB0E8B0D????????FF51048B55F88902E9", "CdCheckCallAddr_v1");

				if (CdCheckCallAddr != -1L)
				{
					logc(FOREGROUND_GREEN, "CdCheckCallAddr_v1: %08X\n", CdCheckCallAddr);
#ifndef USE_SDLOADER	
					//UnProtect_memcpy((BYTE*)(CdCheckCallAddr + 2), PatchCDPtr, 9);
					DWORD RelativeCDCheckHook = ((DWORD)&HookCDCheck) - CdCheckCallAddr - (5 + 2);
					WriteProtectedBYTE((CdCheckCallAddr + 2), 0xE8);
					WriteProtectedDWORD((CdCheckCallAddr + 3), RelativeCDCheckHook);
					WriteProtectedDWORD((CdCheckCallAddr + 3 + 4), 0x90909090);
#endif
				}
				else
				{
					CdCheckCallAddr = FindHexString(StartAddr, EndAddr, "EB108B15????????8B4204FFD08B4DF88901", "CdCheckCallAddr_v2");
					logc(FOREGROUND_GREEN, "CdCheckCallAddr_v2: %08X\n", CdCheckCallAddr);
				
					// We hook for CdCheckCallAddr_v2 but we could just write the Third key (Call of Duty World at War is an example)
					// memcpy((void*)FirstCopy, ThirdKey, amountToCopy);

#ifndef USE_SDLOADER	
					if (CdCheckCallAddr != -1L)
					{
						//UnProtect_memcpy((BYTE*)(CdCheckCallAddr + 8), PatchCDPtr, 5);
						DWORD RelativeCDCheckHook = ((DWORD)&HookCDCheck) - CdCheckCallAddr - (5 + 8);
						WriteProtectedBYTE((CdCheckCallAddr + 8), 0xE8);
						WriteProtectedDWORD((CdCheckCallAddr + 9), RelativeCDCheckHook);
					}
					else
						logc(FOREGROUND_RED, "Can't find CdCheck Call Address!!\n");
#endif
					
				}
				//GetKey();
			}
			
		}
	}

	return ret;
}

BOOL WINAPI ReadProcessMemory_Hook(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
	BOOL ret = ReadProcessMemory_Orig(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
	if (nSize >= 4)
		logc(FOREGROUND_MAGENTA, "ReadProcessMemory: Process: %08X From: %08X To: %08X Size: %04X Data: %08X\n", hProcess, lpBaseAddress, lpBuffer, nSize, ReverseBytes(*(DWORD*)lpBuffer));
	else
		logc(FOREGROUND_MAGENTA, "ReadProcessMemory: Process: %08X From: %08X To: %08X Size: %04X\n", hProcess, lpBaseAddress, lpBuffer, nSize);
	return ret;
}

BOOL WINAPI WriteProcessMemory_Hook(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
	if (nSize == 2)
		logc(FOREGROUND_MAGENTA, "WriteProcessMemory: Process: %08X To: %08X From: %08X Size: %04X Data: %04X\n", hProcess, lpBaseAddress, lpBuffer, nSize, *(WORD*)lpBuffer);
	else
	if (nSize == 4)
		logc(FOREGROUND_MAGENTA, "WriteProcessMemory: Process: %08X To: %08X From: %08X Size: %04X Data: %08X\n", hProcess, lpBaseAddress, lpBuffer, nSize, *(DWORD*)lpBuffer);
	else
	if (nSize > 4)
		logc(FOREGROUND_MAGENTA, "WriteProcessMemory: Process: %08X To: %08X From: %08X Size: %04X Data: %08X%08X%08X%08X\n", hProcess, lpBaseAddress, lpBuffer, nSize, ReverseBytes(*(DWORD*)(lpBuffer)), ReverseBytes(*((DWORD*)(lpBuffer)+4)), ReverseBytes(*((DWORD*)(lpBuffer)+8)), ReverseBytes(*((DWORD*)(lpBuffer)+12)));
	else
		logc(FOREGROUND_MAGENTA, "WriteProcessMemory: Process: %08X To: %08X From: %08X Size: %04X\n", hProcess, lpBaseAddress, lpBuffer, nSize);
	
	return WriteProcessMemory_Orig(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

DWORD WINAPI GetLogicalDrives_Hook()
{
	DWORD ret = GetLogicalDrives_Orig();
	const char* CDROMDriveLetter = config.GetValue("CDROMDriveLetter");
	if (CDROMDriveLetter)
	{
		char driveLetter = toupper(CDROMDriveLetter[0]);
		if (driveLetter >= 'A' && driveLetter <= 'Z')
		{
			ret |= 1 << (driveLetter - 'A');
			logc(FOREGROUND_GREEN, "GetLogicalDrives_Hook: Adding %c as a valid drive\n", driveLetter);
		}
	}
	return ret;
}

UINT WINAPI GetDriveTypeA_Hook(LPCSTR lpRootPathName)
{
	const char* CDROMDriveLetter = config.GetValue("CDROMDriveLetter");
	if (CDROMDriveLetter && lpRootPathName && toupper(lpRootPathName[0]) == toupper(CDROMDriveLetter[0]))
	{
		logc(FOREGROUND_GREEN, "GetDriveTypeA_Hook = %s IS A CDROM!\n", lpRootPathName ? lpRootPathName : "NULL");
		return DRIVE_CDROM;
	}
	log("GetDriveTypeA_Hook = %s\n", lpRootPathName ? lpRootPathName : "NULL");
	return GetDriveTypeA_Orig(lpRootPathName);
}

BOOL WINAPI GetVolumeInformationA_Hook(LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize)
{
	logc(FOREGROUND_BLUE, "GetVolumeInformationA_Hook: lpRootPathName: %s\n", lpRootPathName ? lpRootPathName : "NULL");
	BOOL ret = GetVolumeInformationA_Orig(lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber, lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);
	NString CDROMDriveLetter = config.GetValue("CDROMDriveLetter");
	const char *CDROMVolumeName = config.GetValue("CDROMVolumeName");
	if (CDROMVolumeName && CDROMDriveLetter.GetLength() > 0 && lpVolumeNameBuffer && strlen(CDROMVolumeName) < nVolumeNameSize && lpRootPathName[0] == CDROMDriveLetter[0])
	{
		strcpy(lpVolumeNameBuffer, CDROMVolumeName);
		logc(FOREGROUND_BLUE, "GetVolumeInformationA_Hook: Replacing VolumeName with: %s\n", lpVolumeNameBuffer);

		if (lpFileSystemNameBuffer && strlen("CDFS") < nFileSystemNameSize)
		{
			strcpy(lpFileSystemNameBuffer, "CDFS");
			logc(FOREGROUND_BLUE, "GetVolumeInformationA_Hook: Replacing FileSystemName with: %s\n", lpFileSystemNameBuffer);
		}
	}
	return ret;
}

HMODULE WINAPI LoadLibraryA_26_Hook(LPCSTR lpLibFileName);
HMODULE WINAPI LoadLibraryA_27and28_Hook(LPCSTR lpLibFileName);
void HookOEP(HINSTANCE hModule);

DWORD WINAPI Load(LPVOID lpParam) 
{
	config.LoadConfig("version.json");

	if (config.GetBool("logging"))
	{
		CreateConsole();
		HideConsoleCursor();
		NString logFile = config.GetValue("logFile");
		SetLogging(true, logFile.IsEmpty() ? NULL : (LPCSTR)logFile.Replace("ProcessID", NString::Format("%d", GetCurrentProcessId())));
		logCreateFile = config.GetBool("logCreateFile");
	}

	load_version();
	if (!version_dll)
		return 0;	

	hOurModule = (HMODULE)lpParam;
    HINSTANCE hModule = (HINSTANCE)lpParam;

	char szExeFile[MAX_PATH];
	GetModuleFileNameA(NULL, szExeFile, MAX_PATH);
	NString csExeFile = szExeFile;
	NString csCommandLine = GetCommandLine();

	MH_STATUS status = MH_Initialize();

	DisableThreadLibraryCalls(hModule);

	log("Version.DLL Loaded!\n");
	log("Loaded by .exe: %s\n", (LPCSTR)csExeFile);
	log("CommandLine: %s\n", (LPCSTR)csCommandLine);

	if (GetSafeDiscVersion(csExeFile, &SafeDiscVersion, &SafeDiscSubVersion, &SafeDiscRevision))
	{
		logc(FOREGROUND_BLUE, "SafeDisc Version: %d.%02d.%02d\n", SafeDiscVersion, SafeDiscSubVersion, SafeDiscRevision);
	}

	//SafeDiscVersion = 2; SafeDiscSubVersion = 5; SafeDiscRevision = 30;

	if (config.GetBool("HookOEP") && csExeFile.ToLower().EndsWith(".exe"))
		HookOEP(GetModuleHandle(NULL));

	if (status != MH_OK)
	{
		log("Minhook init failed!\n");
		return 0;
	}

	if ((status = MH_CreateHookApi(L"kernel32", "CreateProcessA", &CreateProcessA_Hook, reinterpret_cast<LPVOID*>(&CreateProcessA_Orig))) != MH_OK)
	{
		log("Unable to hook CreateProcessA: %d\n", status);
		GetKey(true);
		return 0;
	}

	if (MH_CreateHookApi(L"kernel32", "CreateProcessW", &CreateProcessW_Hook, reinterpret_cast<LPVOID*>(&CreateProcessW_Orig)) != MH_OK)
	{
		log("Unable to hook CreateProcessW\n");
		GetKey(true);
		return 0;
	}
	
	if (MH_CreateHookApi(L"ntdll", "NtDeviceIoControlFile", &NtDeviceIoControlFile_Hook, reinterpret_cast<LPVOID*>(&NtDeviceIoControlFile_Orig)) != MH_OK)
	{
		log("Unable to hook NtDeviceIoControlFile\n");
		GetKey(true);
		return 0;
	}

	FARPROC pCreateFileA_K32 = NULL, pCreateFileA_KBase = NULL;
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	HMODULE hKernelBase = GetModuleHandleA("kernelbase.dll");

	pCreateFileA_K32 = GetProcAddress(hKernel32, "CreateFileA");
	if (hKernelBase)
		pCreateFileA_KBase = GetProcAddress(hKernelBase, "CreateFileA");

	if (pCreateFileA_K32 != pCreateFileA_KBase) 
	{
		if (MH_CreateHook(pCreateFileA_K32, CreateFileA_Hook, (LPVOID*)&CreateFileA_Orig) != MH_OK)
		{
			log("Unable to hook CreateFileA from kernel32.dll\n");
			return 0;
		}
		if (MH_CreateHook(pCreateFileA_KBase, CreateFileA_Hook_KBase, (LPVOID*)&CreateFileA_Orig_KBase) != MH_OK)
		{
			log("Unable to hook CreateFileA from kernelbase.dll\n");
			return 0;
		}
	}
	else 
	{
		if (MH_CreateHook(pCreateFileA_K32, CreateFileA_Hook, (LPVOID*)&CreateFileA_Orig) != MH_OK)
		{
			log("Unable to hook CreateFileA from kernel32.dll\n");
			return 0;
		}
	}
	
	if (MH_CreateHookApi(L"kernel32", "CreateFileW", &CreateFileW_Hook, reinterpret_cast<LPVOID*>(&CreateFileW_Orig)) != MH_OK)
	{
		log("Unable to hook CreateFileW\n");
		return 0;
	}

	auto LoadLibrayHook = &LoadLibraryA_Hook;	// For Safedisc 2.9 and above
	if (SafeDiscVersion == 2)
	{ 
		if (SafeDiscSubVersion == 70 || SafeDiscSubVersion == 80)
			LoadLibrayHook = &LoadLibraryA_27and28_Hook;
		else
			LoadLibrayHook = &LoadLibraryA_26_Hook;	// Made for 2.6 but works also does lower
	}
	
	if (MH_CreateHookApi(L"kernel32", "LoadLibraryA", LoadLibrayHook, reinterpret_cast<LPVOID*>(&LoadLibraryA_Orig)) != MH_OK)
	{
		log("Unable to hook LoadLibraryA\n");
		return false;
	}
	
	if (config.GetBool("ejectDebugger"))
	{
		logc(FOREGROUND_CYAN, "Ejecting Debugger Option Selected!\n");
		if (MH_CreateHookApi(L"kernel32", "DebugActiveProcess", &DebugActiveProcess_Hook, reinterpret_cast<LPVOID*>(&DebugActiveProcess_Orig)) != MH_OK)
		{
			log("Unable to hook DebugActiveProcess\n");
			return false;
		}

		if (MH_CreateHookApi(L"kernel32", "WaitForDebugEvent", &WaitForDebugEvent_Hook, reinterpret_cast<LPVOID*>(&WaitForDebugEvent_Orig)) != MH_OK)
		{
			log("Unable to hook WaitForDebugEvent\n");
			return false;
		}
	}

	if (config.GetBool("hookMemoryReadWrites"))
	{
		if (MH_CreateHookApi(L"kernel32", "ReadProcessMemory", &ReadProcessMemory_Hook, reinterpret_cast<LPVOID*>(&ReadProcessMemory_Orig)) != MH_OK)
		{
			log("Unable to hook ReadProcessMemory\n");
			return false;
		}

		if (MH_CreateHookApi(L"kernel32", "WriteProcessMemory", &WriteProcessMemory_Hook, reinterpret_cast<LPVOID*>(&WriteProcessMemory_Orig)) != MH_OK)
		{
			log("Unable to hook WriteProcessMemory\n");
			return false;
		}
	}

	if (MH_CreateHookApi(L"kernel32", "FreeLibrary", &FreeLibrary_Hook, reinterpret_cast<LPVOID*>(&FreeLibrary_Orig)) != MH_OK)
	{
		log("Unable to hook FreeLibrary\n");
		return false;
	}
	
	if (MH_CreateHookApi(L"kernel32", "GetLogicalDrives", &GetLogicalDrives_Hook, reinterpret_cast<LPVOID*>(&GetLogicalDrives_Orig)) != MH_OK)
	{
		log("Unable to hook GetLogicalDrives\n");
		return false;
	}

	if (MH_CreateHookApi(L"kernel32", "GetDriveTypeA", &GetDriveTypeA_Hook, reinterpret_cast<LPVOID*>(&GetDriveTypeA_Orig)) != MH_OK)
	{
		log("Unable to hook GetDriveTypeA\n");
		return false;
	}

	if (MH_CreateHookApi(L"kernel32", "GetVolumeInformationA", &GetVolumeInformationA_Hook, reinterpret_cast<LPVOID*>(&GetVolumeInformationA_Orig)) != MH_OK)
	{
		log("Unable to hook GetVolumeInformationA\n");
		return false;
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		log("Enable Hooks Failed!\n");
		return false;
	}
	
	log("Hooks Complete!\n");

#ifdef USE_SDLOADER	
	// Cheat and just use Reloaded :)
	if (LoadLibraryA("SDLoader.dll") == NULL)
		log("Failed to load SDLoader.dll!\n");
	else
		log("SDLoader.dll Loaded!\n");
#endif

	//GetKey(true);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2.6 Code - Very similiar approach to 2.7 and 2.8 - referenced bOOLs eYe loader from Safedisc 2 Cleaner v1.2.0
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD TablePtr = 0;
DWORD AuthServDataAddr;
DWORD AuthServDataEndAddr;
DWORD MagicCRCTable;

struct TableOffset
{
	DWORD Offset;
	DWORD ValueLocation;
};
TableOffset tableOffsets_2_60_52[] = {
{ 0x000, 0x470 },
{ 0x020, 0x15F8 },
{ 0x024, 0x1178 },
{ 0x028, 0xED0 },
{ 0x030, 0x1068 },
{ 0x034, 0xF58 },
{ 0x038, 0xFE0 },
{ 0x03C, 0xE48 },
{ 0x040, 0xB10 },
{ 0x044, 0xB98 },
{ 0x048, 0xC20 },
{ 0x04C, 0xCA8 },
{ 0x050, 0xD30 },
{ 0x054, 0xDB8 },
{ 0x05C, 0x1840 },
{ 0x060, 0x1B70 },
{ 0x070, 0x14E8 },
{ 0x078, 0x1C8 },
{ 0x07C, 0x250 },
{ 0x080, 0x2D8 },
{ 0x088, 0x360 },
{ 0x09C, 0x3E8 },
{ 0x0E8, 0x34C8 },
{ 0x114, 0x18C8 },
{ 0x128, 0x3F98 } };

// Run this from a debugger once the CD has done its authentication to get the offsets above
void GetMagicOffsets()
{
	logc(FOREGROUND_MAGENTA, "Grabbing Magic Offsets Table!\n");
	logc(FOREGROUND_MAGENTA, "MagicCRCTable = %08X AuthServDataAddr: %08X AuthServDataEndAddr: %08X\n", MagicCRCTable, AuthServDataAddr, AuthServDataEndAddr);
	int CRCTableLength = 0x014C;
	BYTE *MagicCRCTablePtr = (BYTE*)MagicCRCTable;
	for (int i = 0; i < CRCTableLength; i+=4)
	{
		if ((*((DWORD*)MagicCRCTablePtr)) != 0 && (*((DWORD*)MagicCRCTablePtr)) != 0xA5A5A5A5)
		{
			logc(FOREGROUND_MAGENTA, "Testing: loc: %08X val: %08X\n", MagicCRCTablePtr, (*((DWORD*)MagicCRCTablePtr)));
			DWORD FoundAddr = FindHex(AuthServDataAddr, AuthServDataEndAddr, MagicCRCTablePtr, 4);
			if (FoundAddr != -1L)
			{
				logc(FOREGROUND_BLUE, "{ 0x%03X, 0x%03X },", i, FoundAddr - AuthServDataAddr);
			}
			else
				break;
		}
		MagicCRCTablePtr += 4;
	}
	//GetKey(true);
}

int ApplyCount = 0;
void WINAPI ApplyOffsets()
{
	// We apply to the first table we get to. But leave the next table as all A5s
	log("Attempting to ApplyOffsets with MagicCRCTable to %08X (Apply Count: %d)\n", MagicCRCTable, ApplyCount);
	if (*((DWORD*)MagicCRCTable) == 0xA5A5A5A5)
	{
		TableOffset* offsets = NULL;
		int offset_count;
		bool supported = false;

		if (SafeDiscVersion == 2 && SafeDiscSubVersion == 60 && SafeDiscRevision == 52)
		{
			offsets = tableOffsets_2_60_52;
			offset_count = sizeof(tableOffsets_2_60_52) / sizeof(TableOffset);
			supported = true;
		}

		if (supported)
		{
			if (ApplyCount == 0)
			{
				// Write CRC Table
				for (int i = 0; i < 16 * 23; i++)
					*(BYTE*)(MagicCRCTable + i) = 0xA5;

				for (int i = 0; i < offset_count; i++)
				{
					DWORD Offset = MagicCRCTable + offsets[i].Offset;
					DWORD ValueLocation = AuthServDataAddr + offsets[i].ValueLocation;
					*((DWORD*)Offset) = *((DWORD*)ValueLocation);
				}
			}
		}
		else
		{
			logc(FOREGROUND_RED, "WE DONT SUPPORT THIS VERSION!!!!");
			//GetMagicOffsets();
		}

		ApplyCount++;
	}
}

DWORD StealCRCTable_2ndFunc;
DWORD StealCRCTable_JMPBack;
__declspec(naked) void StealCRCTable26()
{
	__asm
	{
		mov[MagicCRCTable], edx

		pushad
#ifdef USE_CRC_GRABBER
		call GetMagicOffsets;
#else
		call ApplyOffsets;
#endif
		popad
		call [StealCRCTable_2ndFunc]
		push [StealCRCTable_JMPBack]
		retn;
	}
}

DWORD DecryptA5Block(DWORD A5Block_41C3D0, DWORD Block41A31C, DWORD Offset_4198F8, DWORD BaseAddr);
void Decrypt20Function();
void Decrypt21Function();
BYTE* Decrypt23Function();
BYTE* Decrypt24Function();
BYTE* Decrypt25Function();

HMODULE WINAPI LoadLibraryA_26_Hook(LPCSTR lpLibFileName)
{
	HMODULE ret = LoadLibraryA_Orig(lpLibFileName);

	if (lpLibFileName)
	{
		if (strstr(lpLibFileName, "~"))
			log("LoadLibraryA_26_Hook: %s %08X\n", lpLibFileName == NULL ? "NULL" : lpLibFileName, ret);

		if (strstr(lpLibFileName, "df394b.tmp") != NULL && bLoadeddf394b == FALSE)	// SecServ.dll
		{
			bLoadeddf394b = TRUE;
			hInstanceSecServ = ret;
			log("LoadLibraryA_26_Hook: %s %08X\n", lpLibFileName == NULL ? "NULL" : lpLibFileName, ret);
			PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)ret;
			PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);

			DWORD StartAddr = (DWORD)ret;
			DWORD EndAddr = StartAddr + pinh->OptionalHeader.SizeOfImage;

			DWORD TableLocationPtr = FindHexString(StartAddr, EndAddr, "9090A1????????555683F8FF57", "TableLocationPtr");
			if (TableLocationPtr != -1L)
			{
				DWORD TableLocation = *(DWORD*)(TableLocationPtr + 3);
				TablePtr = *(DWORD*)(TableLocation);
				logc(FOREGROUND_GREEN, "TableLocationPtr = %08X\n", TableLocationPtr);
				logc(FOREGROUND_GREEN, "TableLocation = %08X\n", TableLocation);
				logc(FOREGROUND_GREEN, "TablePtr = %08X\n", TablePtr);
			}

			if (SafeDiscVersion == 2 && SafeDiscSubVersion == 60 && SafeDiscRevision == 52)
			{
				DWORD MagicCRCTableGrabberFunction = FindHexString(StartAddr, EndAddr, "8B55F089118B45F4894104", "MagicCRCTableGrabberFunction");
				if (MagicCRCTableGrabberFunction != -1L)
				{
					logc(FOREGROUND_GREEN, "MagicCRCTableGrabberFunction = %08X\n", MagicCRCTableGrabberFunction);
					DWORD CallLocation = MagicCRCTableGrabberFunction + 17;
					logc(FOREGROUND_GREEN, "CallLocation = %08X\n", CallLocation);

					StealCRCTable_2ndFunc = CallLocation + (*(DWORD*)(CallLocation+1)) + 5;
					logc(FOREGROUND_GREEN, "StealCRCTable_2ndFunc = %08X\n", StealCRCTable_2ndFunc);

					StealCRCTable_JMPBack = CallLocation + 5;
					logc(FOREGROUND_GREEN, "StealCRCTable_JMPBack = %08X\n", StealCRCTable_JMPBack);

					UnProtectAddress((BYTE*)CallLocation, 1);
					*(BYTE*)CallLocation = 0xE9; // Make it a JMP
					WriteProtectedDWORD(CallLocation + 1, (((DWORD)StealCRCTable26) - CallLocation) - 5);
				}
			}

			// Version 2.10 had an annoying thing to find ZwQuerySystemInformation by ordinal and would end up with NtQuerySecurityAttributesToken instead as it wouldn't handle the ordinal base of 8 correctly
			// Let's just NOP the ordinal base handling all together
			if (SafeDiscVersion == 2 && SafeDiscSubVersion <= 10)
			{
				DWORD OrdinalBaseDisable = FindHexString(StartAddr, EndAddr, "2BC866898DDCFEFFFF", "OrdinalBaseDisable");
				if (OrdinalBaseDisable != -1L)
				{
					BYTE NOPPatch[7] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
					UnProtect_memcpy((void*)(OrdinalBaseDisable + 2), NOPPatch, 7);
				}
			}

			// Int 1 antidebug trick removal
			//DWORD Int1DebugTrick = FindHexString(StartAddr, EndAddr, "78EE79EC??CD0176??90", "Int1DebugTrick");
			DWORD Txt2StartAddr = StartAddr + GetSectionByName(StartAddr, ".txt2")->VirtualAddress;
			DWORD Txt2EndAddr = Txt2StartAddr + GetSectionByName(StartAddr, ".txt2")->SizeOfRawData;
			DWORD Int1DebugTrickCheck = FindHexString(StartAddr, EndAddr, "33D2813D????????050000C00F95C2", "Int1DebugTrickCheck");
			DWORD Int1DebugTrick_PatchOffset = 0;
			//DWORD Int1DebugTrick = FindHexString(Txt2StartAddr, Txt2EndAddr, "CD01", "Int1DebugTrick");	 // int 1 - hard to find and can give false positives
			//if (Int1DebugTrickCheck == -1L || (Int1DebugTrick != -1L && FindHexString(Int1DebugTrick+1, Txt2EndAddr, "CD01", "Int1DebugTrick_recursive") != -1))
				//Int1DebugTrick = -1l; // If we found a second CD01 after the first one, then just ignore this as it's hard to find the right one (just NOP if need be)
			
			DWORD Int1DebugTrick = FindHexString(Txt2StartAddr, Txt2EndAddr, "??EE??EC??CD01", "Int1DebugTrick");
			Int1DebugTrick_PatchOffset = 5;
			
			logc(FOREGROUND_GREEN, "Int1DebugTrick = %08X Int1DebugTrickCheck = %08X\n", Int1DebugTrick, Int1DebugTrickCheck);

			//GetKey(true);

			if (Int1DebugTrick != -1L)
			{
				// Int 1 antidebug trick removal 2.05.30
				//1000E869 | CD 01 | int 1 |
				BYTE* int1ptr = (BYTE*)(Int1DebugTrick+Int1DebugTrick_PatchOffset);
				UnProtectAddress(int1ptr, 2);
				int1ptr[0] = 0x90;
				int1ptr[1] = 0x90;
			}

			if (Int1DebugTrickCheck != -1L)
			{
				// 1000E773 | 0F95C2 | setne dl (sets 1 if no c0000005 access violation)
				BYTE* checkptr = (BYTE*)(Int1DebugTrickCheck + 12);
				UnProtectAddress(checkptr, 3);
				checkptr[0] = 0xB2;  // mov dl, 0
				checkptr[1] = 0x00;
				checkptr[2] = 0x90;
			}
		}
		else
		if ((strstr(lpLibFileName, "~de") != NULL || strstr(lpLibFileName, "~ef") != NULL) && EndsInTmp(lpLibFileName) && TablePtr != 0 && hInstanceAuthServ == 0)  // AuthServ.dll
		{
			hInstanceAuthServ = ret;

			PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)ret;
			PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);

			DWORD StartAddr = (DWORD)ret;
			DWORD EndAddr = StartAddr + pinh->OptionalHeader.SizeOfImage;

			DWORD TableDestOffset = 0;
			DWORD A5TableOffset = 0;
			if (SafeDiscVersion == 2 && SafeDiscSubVersion == 60 && SafeDiscRevision == 52)
			{
				TableDestOffset = 0x19B88;
				A5TableOffset = 0x1A510;
			}
			if (SafeDiscVersion == 2 && SafeDiscSubVersion == 40 && SafeDiscRevision == 10)
			{
				TableDestOffset = 0x1438;
			}

			DWORD Table1DestPtr = TablePtr + TableDestOffset;
			DWORD Table2DestPtr = TablePtr + TableDestOffset + 0x10;
			DWORD Table3DestPtr = TablePtr + TableDestOffset + 0x20;
			DWORD A5Table = TablePtr + A5TableOffset;

			logc(FOREGROUND_GREEN, "Table1DestPtr = %08X Table2DestPtr = %08X Table3DestPtr = %08X\n", Table1DestPtr, Table2DestPtr, Table3DestPtr);

			AuthServDataAddr = StartAddr + GetSectionByName(StartAddr, ".data")->VirtualAddress;
			AuthServDataEndAddr = AuthServDataAddr + GetSectionByName(StartAddr, ".data")->SizeOfRawData;

			DWORD Table1SrcPtr = AuthServDataAddr + 0x4560;
			DWORD Table2SrcPtr = AuthServDataAddr + 0x4AB5;
			DWORD Table3SrcPtr = AuthServDataAddr + 0x500A;

			logc(FOREGROUND_GREEN, "Table1SrcPtr = %08X (%08X) Table2SrcPtr = %08X (%08X) Table3SrcPtr = %08X (%08X)\n", Table1SrcPtr, ReverseBytes(*((DWORD*)Table1SrcPtr)), Table2SrcPtr, ReverseBytes(*((DWORD*)Table2SrcPtr)), Table3SrcPtr, ReverseBytes(*((DWORD*)Table3SrcPtr)));

			DWORD xor_key = (*(DWORD*)Table1SrcPtr) ^ (*(DWORD*)Table2SrcPtr);
			logc(FOREGROUND_GREEN, "xor_key = %08X\n", xor_key);
			logc(FOREGROUND_GREEN, "MorphTableM() -> KeyN[0]DW = %08X\n", (*(DWORD*)Table3SrcPtr) ^ xor_key);
			logc(FOREGROUND_GREEN, "MorphTableM() -> KeyN[1]DW = %08X\n", (*(DWORD*)(Table3SrcPtr+4)) ^ xor_key);
			DWORD CDCheckAddr = FindHexString(StartAddr, EndAddr, "68????????E8????????83C40485C075", "CDCheckCall");

			//GetKey(true);
			
			// Weird CreateFileMappingA test? If it gets error code B7 (ERROR_ALREADY_EXISTS) then it won't set ESI and will fail ?? NOP the jmp
			DWORD CreateFileMappingA_AntiDebug = FindHexString(StartAddr, EndAddr, "7412FF15????????3DB70000007405", "CreateFileMappingA_AntiDebug");
			if (CreateFileMappingA_AntiDebug != -1L)
			{
				BYTE PatchCreateFileMappingA[2] = { 0x90, 0x90 };
				UnProtect_memcpy((BYTE*)(CreateFileMappingA_AntiDebug + 13), PatchCreateFileMappingA, 2);
			}

			if (CDCheckAddr != -1L)
			{
				DWORD CDCheckAddrToHook = CDCheckAddr + 5;

				// 2.05.30
				if (SafeDiscVersion == 2 && SafeDiscSubVersion == 5 && SafeDiscRevision == 30)
				{
					logc(FOREGROUND_GREEN, "CDCheckAddrToHook = %08X Decrypt20Function = %08X\n", CDCheckAddrToHook, (DWORD)Decrypt20Function);
					WriteProtectedDWORD(CDCheckAddrToHook + 1, (((DWORD)Decrypt20Function) - CDCheckAddrToHook) - 5);
				}

				// 2.10.30
				if (SafeDiscVersion == 2 && SafeDiscSubVersion == 10 && SafeDiscRevision == 30)
				{
					logc(FOREGROUND_GREEN, "CDCheckAddrToHook = %08X Decrypt21Function = %08X\n", CDCheckAddrToHook, (DWORD)Decrypt21Function);
					WriteProtectedDWORD(CDCheckAddrToHook + 1, (((DWORD)Decrypt21Function) - CDCheckAddrToHook) - 5);
				}

				// 2.30.33 - code and decryption is only slightly less f**king mental
				if (SafeDiscVersion == 2 && SafeDiscSubVersion == 30 && SafeDiscRevision == 33)
				{
					logc(FOREGROUND_GREEN, "CDCheckAddrToHook = %08X Decrypt23Function = %08X\n", CDCheckAddrToHook, (DWORD)Decrypt23Function);
					WriteProtectedDWORD(CDCheckAddrToHook + 1, (((DWORD)Decrypt23Function) - CDCheckAddrToHook) - 5);
				}

				// 2.40.10 - code and decryption is f**king mental
				if (SafeDiscVersion == 2 && SafeDiscSubVersion == 40 && SafeDiscRevision == 10)
				{
					logc(FOREGROUND_GREEN, "CDCheckAddrToHook = %08X Decrypt24Function = %08X\n", CDCheckAddrToHook, (DWORD)Decrypt24Function);
					WriteProtectedDWORD(CDCheckAddrToHook+1, (((DWORD)Decrypt24Function) - CDCheckAddrToHook) - 5);
				}

				// 2.51.21 - code and decryption is similarly f**king mental
				if (SafeDiscVersion == 2 && SafeDiscSubVersion == 51 && (SafeDiscRevision == 21 || SafeDiscRevision == 20))
				{
					logc(FOREGROUND_GREEN, "CDCheckAddrToHook = %08X Decrypt25Function = %08X\n", CDCheckAddrToHook, (DWORD)Decrypt25Function);
					WriteProtectedDWORD(CDCheckAddrToHook + 1, (((DWORD)Decrypt25Function) - CDCheckAddrToHook) - 5);
				}
			}

			//GetKey(true);

			if (SafeDiscVersion == 2 && SafeDiscSubVersion == 60 && SafeDiscRevision == 52)
			{
				DWORD Table1Dest = *(DWORD*)Table1DestPtr;
				DWORD Table2Dest = *(DWORD*)Table2DestPtr;
				DWORD Table3Dest = *(DWORD*)Table3DestPtr;
				logc(FOREGROUND_GREEN, "Table1Dest = %08X Table2Dest = %08X Table3Dest = %08X\n", Table1Dest, Table2Dest, Table3Dest);
				logc(FOREGROUND_GREEN, "A5Table = %08X\n", A5Table);
				logc(FOREGROUND_GREEN, "Our GetMagicOffsets Func = %08X (call this from a debugger after a good cd check if you want the offsets)\n", GetMagicOffsets);


				BYTE A5Block[0x200];
				memset(A5Block, 0xA5, 0x200);
				memcpy((void*)A5Table, A5Block, 0x158);

				memcpy((void*)Table1Dest, (void*)Table3SrcPtr, 0x400);

				for (int i = 0; i < 0x400/4; i++)
				{
					*(((DWORD*)(Table2Dest+(i*4)))) = (*((DWORD*)(Table3SrcPtr+(i*4)))) ^ xor_key;
				}

				memcpy((void*)Table3Dest, (void*)Table3SrcPtr, 0x400);
			
				if (CDCheckAddr != -1L)
				{
					DWORD CDCheckAddrToHook = CDCheckAddr + 5;
					logc(FOREGROUND_GREEN, "CDCheckAddrToHook = %08X\n", CDCheckAddrToHook);
					BYTE PatchCDPtr[5] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
					UnProtect_memcpy((BYTE*)CDCheckAddrToHook, PatchCDPtr, 5);
				}

				
				// LoadLibraryA_Orig("DCEAPIHook.dll");

				//GetKey(true);
			}
		}
	}

	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// My Old 2.7 and 2.8 code
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MINIMUM_SAFEDISC_SUBVERSION 70
BOOL Patch2728Installed = FALSE;
BOOL PatchCheckFailed = FALSE;
DWORD ItemA = 0;
DWORD ItemE_1 = 0;
DWORD ItemE_2 = 0;
DWORD ItemE_3 = 0;

TableOffset tableOffsets_2_80_00[] = {
{ 0x00, 0x470 },
{ 0x08, 0x608 },
{ 0x0C, 0x580 },
{ 0x2C, 0x1A08 },
{ 0x34, 0x1398 },
{ 0x38, 0x10F0 },
{ 0x40, 0x1288 },
{ 0x44, 0x1178 },
{ 0x48, 0x1200 },
{ 0x4C, 0x1068 },
{ 0x50, 0xD30 },
{ 0x54, 0xDB8 },
{ 0x58, 0xE40 },
{ 0x5C, 0xEC8 },
{ 0x60, 0xF50 },
{ 0x64, 0xFD8 },
{ 0x6C, 0x1CE8 },
{ 0x70, 0x2018 },
{ 0x80, 0x1760 },
{ 0x8C, 0x1C8 },
{ 0x90, 0x250 },
{ 0x94, 0x2D8 },
{ 0x9C, 0x360 },
{ 0xB4, 0x3E8 },
{ 0x100, 0x3960 },
{ 0x130, 0x1D70 },
{ 0x144, 0x4510 } };

TableOffset tableOffsets_2_72_00[] = {
{ 0x000, 0x470 },
{ 0x008, 0x608 },
{ 0x00C, 0x580 },
{ 0x02C, 0x19E0 },
{ 0x034, 0x14D8 },
{ 0x038, 0x1230 },
{ 0x040, 0x13C8 },
{ 0x044, 0x12B8 },
{ 0x048, 0x1340 },
{ 0x04C, 0x11A8 },
{ 0x050, 0xE70 },
{ 0x054, 0xEF8 },
{ 0x058, 0xF80 },
{ 0x05C, 0x1008 },
{ 0x060, 0x1090 },
{ 0x064, 0x1118 },
{ 0x06C, 0x1CB0 },
{ 0x070, 0x1FE0 },
{ 0x080, 0x1848 },
{ 0x08C, 0x1C8 },
{ 0x090, 0x250 },
{ 0x094, 0x2D8 },
{ 0x09C, 0x360 },
{ 0x0B0, 0x3E8 },
{ 0x0FC, 0x3938 },
{ 0x128, 0x1D38 },
{ 0x13C, 0x42C8 } };

TableOffset tableOffsets_2_70_30[] = {
{ 0x000, 0x470 },
{ 0x008, 0x608 },
{ 0x00C, 0x580 },
{ 0x02C, 0x19E8 },
{ 0x034, 0x14D8 },
{ 0x038, 0x1230 },
{ 0x040, 0x13C8 },
{ 0x044, 0x12B8 },
{ 0x048, 0x1340 },
{ 0x04C, 0x11A8 },
{ 0x050, 0xE70 },
{ 0x054, 0xEF8 },
{ 0x058, 0xF80 },
{ 0x05C, 0x1008 },
{ 0x060, 0x1090 },
{ 0x064, 0x1118 },
{ 0x06C, 0x1CC8 },
{ 0x070, 0x1FF8 },
{ 0x080, 0x1850 },
{ 0x08C, 0x1C8 },
{ 0x090, 0x250 },
{ 0x094, 0x2D8 },
{ 0x09C, 0x360 },
{ 0x0B0, 0x3E8 },
{ 0x0FC, 0x3950 },
{ 0x128, 0x1D50 },
{ 0x13C, 0x42E0 } };

DWORD AuthServStartAddr;
DWORD AuthServEndAddr;
DWORD CRCTableAddress;
BOOL SetCRCTableHasBeenCalled = FALSE;

void SetCRCTable()
{
	if (SetCRCTableHasBeenCalled == FALSE)
	{
		int i;
		BYTE searchBytes[16 * 22];
		TableOffset* offsets = NULL;
		int offset_count;

		if (SafeDiscVersion == 2 && SafeDiscSubVersion == 80)
		{
			offsets = tableOffsets_2_80_00;
			offset_count = sizeof(tableOffsets_2_80_00) / sizeof(TableOffset);
		}
		if (SafeDiscVersion == 2 && SafeDiscSubVersion == 72)
		{
			offsets = tableOffsets_2_72_00;
			offset_count = sizeof(tableOffsets_2_72_00) / sizeof(TableOffset);
		}

		if (SafeDiscVersion == 2 && SafeDiscSubVersion == 70 && SafeDiscRevision == 30)
		{
			offsets = tableOffsets_2_70_30;
			offset_count = sizeof(tableOffsets_2_70_30) / sizeof(TableOffset);
		}

		if (offsets == NULL)
		{
			logc(FOREGROUND_RED, "SetCRCTable: No offsets found for Version %d.%02d.%02d\n", SafeDiscVersion, SafeDiscSubVersion, SafeDiscRevision);
			PatchCheckFailed = TRUE;
			return;
		}

		// Default is all 0xA5s
		memset(searchBytes, 0xA5, 16 * 22);

		DWORD CRCOffset = CRCTableAddress;
		log("CRCOffset: %08X\n", CRCOffset);

		if (*((DWORD*)CRCOffset) == 0xA5A5A5A5)
		{
			log("Applying Table to %08X\n", CRCOffset);

			// Write CRC Table
			for (i = 0; i < 16 * 23; i++)
				*(BYTE*)(CRCOffset + i) = 0xA5;

			for (i = 0; i < offset_count; i++)
			{
				DWORD Offset = CRCOffset + offsets[i].Offset;
				DWORD ValueLocation = AuthServDataAddr + offsets[i].ValueLocation;
				*((DWORD*)Offset) = *((DWORD*)ValueLocation);
			}
		}
		SetCRCTableHasBeenCalled = TRUE;
	}
}

void WINAPI OurCopyFunction()
{
	// Let's emulate the CopyFunction
	DWORD ItemA_AddressPointedTo = *(DWORD*)ItemA;
	DWORD ItemA_AddressPointedTo2 = *(DWORD*)(ItemA_AddressPointedTo + 0xc);
	DWORD ItemA_1Address = *(DWORD*)(ItemA_AddressPointedTo2 + 0x24);
	DWORD ItemA_2Address = *(DWORD*)(ItemA_AddressPointedTo2 + 0x34);
	DWORD ItemA_3Address = *(DWORD*)(ItemA_AddressPointedTo2 + 0x44);
	//log("ItemA_1Address: %08X ItemA_2Address: %08X ItemA_3Address: %08X\n", ItemA_1Address, ItemA_2Address, ItemA_3Address);

	memcpy((DWORD*)ItemA_1Address, (DWORD*)ItemE_3, 1024);		// The first key should really end up as the 3rd key
	// memcpy((DWORD*)ItemA_2Address, (DWORD*)ItemE_2, 1024);   // Second key will already be set and go through the CRCs 
	memcpy((DWORD*)ItemA_3Address, (DWORD*)ItemE_3, 1024);
}

__declspec(naked) void StealCRCTable()
{
	__asm
	{
		mov edx, dword ptr ss : [esp]
		mov eax, dword ptr ds : [edx + 0x18]
		mov[CRCTableAddress], eax

#ifdef USE_CRC_GRABBER
		call Grabber;
#else
		call SetCRCTable;
#endif

		mov eax, [StealCRCTable_2ndFunc]
			call eax
			push[StealCRCTable_JMPBack]
			retn;
	}
}

HMODULE WINAPI LoadLibraryA_27and28_Hook(LPCSTR lpLibFileName)
{
	static DWORD TableClass = 0;

	HMODULE ret = LoadLibraryA_Orig(lpLibFileName);
	if (lpLibFileName)
	{
		if (PatchCheckFailed == FALSE && Patch2728Installed == FALSE && strstr(lpLibFileName, ".tmp"))
		{
			log("Loaded DLL: %s\n", lpLibFileName);

			// Check for SecServ.dll
			if (strstr(lpLibFileName, "~df394b.tmp"))
			{
				logc(FOREGROUND_BLUE, "LoadLibraryA_27and28_Hook - SafeDisc Version: %d.%02d.%02d\n", SafeDiscVersion, SafeDiscSubVersion, SafeDiscRevision);
				if (SafeDiscVersion == 2 && SafeDiscSubVersion < 90 && SafeDiscSubVersion >= MINIMUM_SAFEDISC_SUBVERSION)
				{
					PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)ret;
					PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);

					DWORD StartAddr = (DWORD)ret;
					DWORD EndAddr = StartAddr + pinh->OptionalHeader.SizeOfImage;

					// Item A
					log("------------\n");
					log("Item A\n");
					log("------------\n");

					DWORD firstMatch = FindHexString(StartAddr, EndAddr, "FF5014C3");
					DWORD secondMatch = FindHexString(firstMatch + 1, EndAddr, "FF5014C3");
					log("FirstMatch: %X SecondMatch: %X\n", firstMatch, secondMatch);
					ItemA = *(DWORD*)(secondMatch + 6);
					log("Item A: %08X (Key Offset to be referenced and used by our copy function)\n", ItemA);

					// Item C
					log("\n------------\n");
					log("Item C\n");
					log("------------\n");

					DWORD ItemCLocation = FindHexString(StartAddr, EndAddr, "66B80100C9C21000E900");
					log("ItemCLocation: %X\n", ItemCLocation);
					DWORD ItemCLocation1stAddr = *((DWORD*)(ItemCLocation - 0xf));
					DWORD ItemCLocation2ndAddr = *((DWORD*)(ItemCLocation - 4));
					log("Item C - 1st Addr: %08X (Memory Location that needs to be made 0 so the cmp check works)\n", ItemCLocation1stAddr);
					log("Item C - 2nd Addr: %08X (Memory Location to the function pointer that will be changed to point to our copy function)\n", ItemCLocation2ndAddr);

					DWORD CRCHookLocation = FindHexString(StartAddr, EndAddr, "83661000E810000000");
					if (CRCHookLocation != -1L)
					{
						log("CRCHookLocation: %08X\n", CRCHookLocation);
						WriteProtectedDWORD(CRCHookLocation + 4, 0xE9);
						DWORD StealCRCTableLocation = (DWORD)&StealCRCTable;
						WriteProtectedDWORD(CRCHookLocation + 5, StealCRCTableLocation - (CRCHookLocation + 4 + 5));

						StealCRCTable_2ndFunc = CRCHookLocation + 0x19;
						StealCRCTable_JMPBack = CRCHookLocation + 0x9;
					}
					else
						log("Cannot find CRC Hook Location\n");

					WriteProtectedDWORD(ItemCLocation1stAddr, 0);
					WriteProtectedDWORD(ItemCLocation2ndAddr, (DWORD)&OurCopyFunction);
				}
				else
				{
					logc(FOREGROUND_RED, "PatchCheckFailed !");
					PatchCheckFailed = TRUE;
				}
			}
			else
			{
				if (SafeDiscVersion == 2 && SafeDiscSubVersion < 90 && SafeDiscSubVersion >= MINIMUM_SAFEDISC_SUBVERSION && PatchCheckFailed == FALSE && Patch2728Installed == FALSE)
				{
					// The only other tmp dll that gets loaded is AuthServ.dll
					PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)ret;
					PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);

					DWORD StartAddr = (DWORD)ret;
					DWORD EndAddr = StartAddr + pinh->OptionalHeader.SizeOfImage;

					// Item D
					log("------------\n");
					log("Item D\n");
					log("------------\n");

					DWORD ItemDLocationPart1 = FindHexString(StartAddr, EndAddr, "EB05B85000????8BE55DC3");
					DWORD ItemDLocationPart2 = FindHexString(StartAddr, EndAddr, "EB0E8B0D????????FF51048B55F88902E9");

					// Part 1
					ItemDLocationPart1 += 2;
					log("ItemDLocationPart1: %08X (Address of function to return the right result code)\n", ItemDLocationPart1);
					log("Need to find a version of it that is just mov then ret\n");
					BYTE SearchBuffer[6];
					memcpy(SearchBuffer, (BYTE*)ItemDLocationPart1, 5);
					SearchBuffer[5] = 0xC3;
					ItemDLocationPart1 = FindHex(StartAddr, EndAddr, SearchBuffer, 6);
					log("New ItemDLocationPart1: %08X\n", ItemDLocationPart1);

					// Part 2
					ItemDLocationPart2 += 4;	//
					log("ItemDLocationPart2: %08X (Code Location that has a pointer to the data that has a pointer to the cd check function we will redirect)\n", ItemDLocationPart2);

					DWORD ItemDLocationPart2_AddressFromTheCode = *((DWORD*)ItemDLocationPart2);
					log("Item D Part 2 - Address from the code: %08X\n", ItemDLocationPart2_AddressFromTheCode);
					DWORD ItemDLocationPart2_LocationAtAddressFromTheCode = *((DWORD*)ItemDLocationPart2_AddressFromTheCode);
					log("Item D Part 2 - LocationAddress from the code: %08X\n", ItemDLocationPart2_AddressFromTheCode);
					DWORD ItemDLocationPart2_LocationAtAddressFromTheCodePlus4 = ItemDLocationPart2_LocationAtAddressFromTheCode + 4;

					// Removes all CD Checks
					WriteProtectedDWORD(ItemDLocationPart2_LocationAtAddressFromTheCodePlus4, ItemDLocationPart1);	// Redirect the CD Checks to give the right result

					// Item E - TODO: Currently hardcoded
					if (SafeDiscVersion == 2 && SafeDiscSubVersion == 90)
						ItemE_3 = ((DWORD)ret) + 0x34BAA;

					if (SafeDiscVersion == 2)
					{
						if (SafeDiscSubVersion == 80)	// 2.80.10 == HPCOS + FIFA 2003 + Bond Nightfire
						{
							ItemE_1 = ((DWORD)ret) + 0x32AC8;   // Key 1 - Final Key before processing
							ItemE_2 = ((DWORD)ret) + 0x3301D;   // Key 2 - The difficult key
							ItemE_3 = ((DWORD)ret) + 0x33572;   // 0x32AC8;//0x3301D;	// Key 3 (Final Key)
						}
						if (SafeDiscSubVersion == 72 && SafeDiscRevision == 0)  // Madden 2003
						{
							ItemE_1 = ((DWORD)ret) + 0x32878;
							ItemE_2 = ((DWORD)ret) + 0x32DCD;
							ItemE_3 = ((DWORD)ret) + 0x33322;
						}
						if (SafeDiscSubVersion == 70 && SafeDiscRevision == 30)	// Mafia, MS Combat Flight Simulator 3
						{
							ItemE_1 = ((DWORD)ret) + 0x32890;
							ItemE_2 = ((DWORD)ret) + 0x32DE5;
							ItemE_3 = ((DWORD)ret) + 0x3333A;
						}

						if (ItemE_1 != 0 && ItemE_2 != 0 && ItemE_3 != 0)
						{
							LogKey("First Key", ItemE_1);
							LogKey("Second Key", ItemE_2);
							LogKey("Third Key", ItemE_3);

							for (int i = 0; i < 1024; i++)
							{
								*((BYTE*)(ItemE_2 + i)) ^= *((BYTE*)(ItemE_1 + i)) ^ *((BYTE*)(ItemE_3 + i));
							}

							LogKey("New Second Key", ItemE_2);
						}

						// GetKey();

						DWORD ItemA_AddressPointedTo = *(DWORD*)ItemA;
						DWORD ItemA_AddressPointedTo2 = *(DWORD*)(ItemA_AddressPointedTo + 0xc);
						DWORD ItemA_1Address = *(DWORD*)(ItemA_AddressPointedTo2 + 0x24);
						DWORD ItemA_2Address = *(DWORD*)(ItemA_AddressPointedTo2 + 0x34);
						DWORD ItemA_3Address = *(DWORD*)(ItemA_AddressPointedTo2 + 0x44);
						log("ItemA_1Address: %08X ItemA_2Address: %08X ItemA_3Address: %08X\n", ItemA_1Address, ItemA_2Address, ItemA_3Address);

						// Only set the new Second Key here (it will then go through the 3 CRC processes)
						if (ItemE_2 != 0)
							memcpy((DWORD*)ItemA_2Address, (DWORD*)ItemE_2, 1024);

						AuthServDataAddr = StartAddr + GetSectionByName(StartAddr, ".data")->VirtualAddress;
						AuthServStartAddr = StartAddr;
						AuthServEndAddr = EndAddr;
					}

					//infloop:
					//goto infloop;
					//DebugBreak();
					Patch2728Installed = TRUE;
				}
			}
		}
	}

	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OEP Hook
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD OEPAddress = 0;
void OEPHookFunction()
{
	logc(FOREGROUND_BLUE, "At the OEP\n");
	GetKey(true);
	__asm 
	{
		push [OEPAddress]
		ret
	}
}

void HookOEP(HINSTANCE hModule)
{
	DWORD StartAddr = (DWORD)hModule;
	if (GetSectionByName(StartAddr, ".text") == NULL)
	{
		logc(FOREGROUND_RED, "HookOEP: .text section not found! Not hooking OEP\n");
		return;
	}
    PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);
	DWORD AEP = StartAddr + pinh->OptionalHeader.AddressOfEntryPoint;
	DWORD EndAddr = StartAddr + pinh->OptionalHeader.SizeOfImage;
	DWORD TextStartAddr = StartAddr + GetSectionByName(StartAddr, ".text")->VirtualAddress;
	DWORD TextEndAddr = TextStartAddr + GetSectionByName(StartAddr, ".text")->SizeOfRawData;

	log("StartAddr: %08X EndAddr: %08X\n", StartAddr, EndAddr);
	log("AddressOfEntryPoint: %08X\n", AEP);
	log(".text Section Start: %08X End: %08X\n", TextStartAddr, TextEndAddr);

	DWORD SearchStart = AEP;
	for (int i = 0; i < 5; i++)
	{
		DWORD OEPJmpLocation = FindHexString(SearchStart, EndAddr, "E9");
		if (OEPJmpLocation != -1L)
		{
			OEPAddress = (OEPJmpLocation + (*(DWORD*)(OEPJmpLocation + 1))) + 5;
			if (OEPAddress >= TextStartAddr && OEPAddress <= TextEndAddr)
			{
				log("OEPJmpLocation: %08X\n", OEPJmpLocation);
				logc(FOREGROUND_GREEN, "OEPAddress: %08X\n", OEPAddress);
				logc(FOREGROUND_GREEN, "OEPHookFunction: %08X\n", (DWORD)OEPHookFunction);
				logc(FOREGROUND_GREEN, "Detected OEP is in the .text section, patching it\n");
				WriteProtectedDWORD(OEPJmpLocation + 1, (((DWORD)OEPHookFunction) - OEPJmpLocation) - 5);
				break;
			}
			SearchStart = OEPJmpLocation + 1;
		}
		else
			break;
	}
	
	//GetKey(true);
}