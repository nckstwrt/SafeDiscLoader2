#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "Utils.h"

FILE* log_file = NULL;
bool loggingEnabled = false;

void SetLogging(bool enable, const char* logFileName)
{
	if (logFileName)
		log_file = fopen(logFileName, /*"a+t"*/"wt");
	loggingEnabled = enable;
}

void logc(WORD col, const char* fmt, ...)
{
	if (!loggingEnabled)
		return;
	va_list va;
	if (log_file)
	{
		va_start(va, fmt);
		vfprintf(log_file, fmt, va);
		va_end(va);
		fflush(log_file);
	}

	if (stdout)
	{
		HANDLE hConsole = CreateFileA("CONOUT$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		DWORD mode = 0;
		GetConsoleMode(hConsole, &mode);
		SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		int r, g, b;

		switch (col)
		{
		case FOREGROUND_WHITE:
			r = 240; g = 240; b = 240;  // Softer white
			break;
		case FOREGROUND_GREY:
			r = 128; g = 128; b = 128;  // Neutral gray
			break;
		case FOREGROUND_BLUE:
			r = 0; g = 120; b = 215;    // Windows 10 accent blue
			break;
		case FOREGROUND_CYAN:
			r = 0; g = 200; b = 200;    // Teal cyan
			break;
		case FOREGROUND_GREEN:
			r = 0; g = 180; b = 0;      // Rich green
			break;
		case FOREGROUND_LIME:
			r = 50; g = 205; b = 50;    // Lime green
			break;
		case FOREGROUND_YELLOW:
			r = 255; g = 215; b = 0;    // Golden yellow
			break;
		case FOREGROUND_ORANGE:
			r = 255; g = 140; b = 0;    // Dark orange
			break;
		case FOREGROUND_PINK:
			r = 255; g = 105; b = 180;  // Hot pink
			break;
		case FOREGROUND_MAGENTA:
			r = 186; g = 85; b = 211;   // Medium orchid
			break;
		case FOREGROUND_PURPLE:
			r = 138; g = 43; b = 226;   // Blue violet
			break;
		case FOREGROUND_RED:
			r = 220; g = 20; b = 60;    // Crimson
			break;
		case FOREGROUND_BROWN:
			r = 139; g = 69; b = 19;    // Saddle brown
			break;
		case FOREGROUND_TURQUOISE:
			r = 64; g = 224; b = 208;   // Turquoise
			break;
		default:
			r = 192; g = 192; b = 192;  // Default to light gray
			break;
		}

		printf("\x1b[38;2;%d;%d;%dm", r, g, b);
		
		va_start(va, fmt);
		vfprintf(stdout, fmt, va);
		va_end(va);

		printf("\x1b[38;2;%d;%d;%dm", 192, 192, 192);  // Set to "white" by default
		SetConsoleMode(hConsole, mode);
		CloseHandle(hConsole);
	}
	/*
	if (stdout)
	{
		//HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		HANDLE hConsole = CreateFileA("CONOUT$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		// Save current text attributes
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		WORD saved_attributes;
		GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
		saved_attributes = consoleInfo.wAttributes;

		// Set color to bright green
		SetConsoleTextAttribute(hConsole, col | FOREGROUND_INTENSITY);  // FOREGROUND_GREEN | FOREGROUND_INTENSITY

		va_start(va, fmt);
		vfprintf(stdout, fmt, va);
		va_end(va);

		SetConsoleTextAttribute(hConsole, saved_attributes);

		CloseHandle(hConsole);
	}
	*/
}

void log(const char* fmt, ...)
{
if (!loggingEnabled)
		return;
	va_list va;
	if (log_file)
	{
		va_start(va, fmt);
		vfprintf(log_file, fmt, va);
		va_end(va);
		fflush(log_file);
	}
	if (stdout)
	{
		va_start(va, fmt);
		vfprintf(stdout, fmt, va);
		va_end(va);
	}
}

void LogKey(const char* keyName, DWORD addr, int keyLength)
{
	if (addr != 0)
	{
		log("%s (%08X): ", keyName, addr);
		if (keyLength > 16)
			log("\n");
		for (int i = 0; i < keyLength; i++)
		{
			if (i != 0 && (i % 16) == 0)
				log("\n");
			log("%02X ", (DWORD) * (((BYTE*)addr) + i));
		}
		log("\n");
	}
}

BYTE hexdigit(char hex)
{
	return (hex <= '9') ? hex - '0' : toupper(hex) - 'A' + 10;
}

BYTE hexbyte(const char* hex)
{
	if (*hex == '?')		// ?? is a wildcard and will be 00 - which means 00 matches with anything
		return 0;
	else
		return (hexdigit(*hex) << 4) | hexdigit(*(hex + 1));
}

DWORD FindHex(DWORD StartAddr, DWORD EndAddr, BYTE* searchHex, DWORD searchSize)
{
	DWORD ret = -1L;
	DWORD i;
	BYTE* ptr = (BYTE*)StartAddr;
	DWORD Length = (EndAddr - StartAddr) - searchSize;
	BYTE* cmpptr = searchHex;
	DWORD matched = 0;
	DWORD mostmatched = 0;
	for (i = 0; i < Length; i++)
	{
		if ((*ptr == *cmpptr) || (*cmpptr == 0))
		{
			cmpptr++;
			matched++;
			if (matched == searchSize)
			{
				ret = ((DWORD)ptr) - (searchSize - 1);
				break;
			}
			if (mostmatched < matched)
				mostmatched = matched;
		}
		else
		{
			ptr -= matched;
			i -= matched;
			matched = 0;
			cmpptr = searchHex;
		}
		ptr++;
	}

	return ret;
}

DWORD FindHexString(DWORD StartAddr, DWORD EndAddr, const char* szHexString, const char* szPurpose)
{
	DWORD i;
	DWORD ret = -1L;

	if (StartAddr < EndAddr)
	{
		BYTE searchSize = (BYTE)(strlen(szHexString) / 2);
		BYTE* searchHex = new BYTE[searchSize];

		for (i = 0; i < searchSize; i++)
		{
			searchHex[i] = hexbyte(&szHexString[i * 2]);
		}

		ret = FindHex(StartAddr, EndAddr, searchHex, searchSize);

		delete[] searchHex;

		if (szPurpose == NULL)
			log("FindHexString(%08X, %08X, \"%s\") == %08X\n", StartAddr, EndAddr, szHexString, ret);
		else
			log("FindHexString(%08X, %08X, \"%s\", \"%s\") == %08X\n", StartAddr, EndAddr, szHexString, szPurpose, ret);
	}
	else
		logc(FOREGROUND_RED, "FindHexString StartAddr < EndAddr failed: %08X < %08X\n", StartAddr, EndAddr);

	return ret;
}

BOOL GetSafeDiscVersionFromBuffer(BYTE* buffer, DWORD dwBufferSize, DWORD* pdwVersion, DWORD* pdwSubVersion, DWORD* pdwRevision)
{
	BOOL bRet = FALSE;
	if (buffer && dwBufferSize > 0)
	{
		// BoG_ *90.0&!! Yy> (alternative: 000001_!!!)
		DWORD offset = 0x20;
		DWORD AddrToVersion = FindHexString((DWORD)buffer, ((DWORD)buffer) + dwBufferSize, "426F475F202A39302E30262121202059793E0000000000000000000000000000", "Version String");
		if (AddrToVersion == -1)
		{
			AddrToVersion = FindHexString((DWORD)buffer, ((DWORD)buffer) + dwBufferSize, "3030303030315F21212100", "Version String v2");
			offset = 11;
		}

		if (AddrToVersion != -1)
		{
			*pdwVersion = *(DWORD*)(AddrToVersion + offset);
			*pdwSubVersion = *(DWORD*)(AddrToVersion + offset + 4);
			*pdwRevision = *(DWORD*)(AddrToVersion + offset + 8);

			log("SafeDisc Version: %d.%02d.%02d\n", *pdwVersion, *pdwSubVersion, *pdwRevision);
			bRet = TRUE;
		}
	}
	return bRet;
}

BOOL GetSafeDiscVersion(const char* szExeFile, DWORD* pdwVersion, DWORD* pdwSubVersion, DWORD* pdwRevision)
{
	BOOL bRet = FALSE;
	HANDLE hFile = CreateFile(szExeFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwFileSize = GetFileSize(hFile, NULL);
		DWORD dwBytesRead;
		BYTE* buffer = new BYTE[dwFileSize];
		ReadFile(hFile, buffer, dwFileSize, &dwBytesRead, NULL);
		bRet = GetSafeDiscVersionFromBuffer(buffer, dwBytesRead, pdwVersion, pdwSubVersion, pdwRevision);
		delete[] buffer;
	}
	CloseHandle(hFile);
	return bRet;
}

PIMAGE_SECTION_HEADER GetSectionByName(DWORD addr, const char* szName)
{
	PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)addr;
	PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);

	PIMAGE_FILE_HEADER pifh = (PIMAGE_FILE_HEADER)&pinh->FileHeader;
	PIMAGE_SECTION_HEADER pish = IMAGE_FIRST_SECTION(pinh);

	for (WORD i = 0; i < pifh->NumberOfSections; i++)
	{
		if (_stricmp((char*)pish->Name, szName) == 0)
			return pish;
		pish++;
	}

	return NULL;
}

BOOL WriteProtectedDWORD(DWORD Addr, DWORD Value)
{
	BOOL bRet = FALSE;
	DWORD old;
	if (VirtualProtectEx(GetCurrentProcess(), (void*)Addr, 4, PAGE_READWRITE, &old))
	{
		*((DWORD*)Addr) = Value;
		if (VirtualProtectEx(GetCurrentProcess(), (void*)Addr, 4, old, &old))
			bRet = TRUE;
	}

	if (bRet)
		log("WriteProtectedDWORD(%08X, %08X)\n", Addr, Value);
	else
		log("Failed to WriteProtectedDWORD(%08X, %08X) !!!!\n", Addr, Value);
	return bRet;
}

BOOL WriteProtectedBYTE(DWORD Addr, BYTE Value)
{
	BOOL bRet = FALSE;
	DWORD old;
	if (VirtualProtectEx(GetCurrentProcess(), (void*)Addr, 1, PAGE_READWRITE, &old))
	{
		*((BYTE*)Addr) = Value;
		if (VirtualProtectEx(GetCurrentProcess(), (void*)Addr, 1, old, &old))
			bRet = TRUE;
	}

	if (bRet)
		log("WriteProtectedBYTE(%08X, %02X)\n", Addr, (DWORD)Value);
	else
		log("Failed to WriteProtectedBYTE(%08X, %02X) !!!!\n", Addr, (DWORD)Value);
	return bRet;
}

HRESULT PatchIat(HMODULE Module, PSTR ImportedModuleName, PSTR ImportedProcName, PVOID AlternateProc, PVOID* OldProc)
{
#define PtrFromRva( base, rva ) ( ( ((DWORD)( PBYTE ) base) ) + ((DWORD)rva) )
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Module;
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
	UINT Index;

	NtHeader = (PIMAGE_NT_HEADERS)PtrFromRva(DosHeader, DosHeader->e_lfanew);

	if (IMAGE_NT_SIGNATURE != NtHeader->Signature)
		return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);

	ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)PtrFromRva(DosHeader, NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for (Index = 0; ImportDescriptor[Index].Characteristics != 0; Index++)
	{
		PSTR dllName = (PSTR)PtrFromRva(DosHeader, ImportDescriptor[Index].Name);

		if (_strcmpi(dllName, ImportedModuleName) == 0)
		{
			PIMAGE_THUNK_DATA Thunk;
			PIMAGE_THUNK_DATA OrigThunk;

			if (!ImportDescriptor[Index].FirstThunk || !ImportDescriptor[Index].OriginalFirstThunk)
				return E_INVALIDARG;

			Thunk = (PIMAGE_THUNK_DATA)PtrFromRva(DosHeader, ImportDescriptor[Index].FirstThunk);
			OrigThunk = (PIMAGE_THUNK_DATA)PtrFromRva(DosHeader, ImportDescriptor[Index].OriginalFirstThunk);

			for (; OrigThunk->u1.Function != NULL; OrigThunk++, Thunk++)
			{
				if (OrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
					continue;

				PIMAGE_IMPORT_BY_NAME import = (PIMAGE_IMPORT_BY_NAME)PtrFromRva(DosHeader, OrigThunk->u1.AddressOfData);

				if (strcmp(ImportedProcName, (char*)import->Name) == 0)
				{
					DWORD junk;
					MEMORY_BASIC_INFORMATION thunkMemInfo;

					VirtualQuery(Thunk, &thunkMemInfo, sizeof(MEMORY_BASIC_INFORMATION));

					if (!VirtualProtect(thunkMemInfo.BaseAddress, thunkMemInfo.RegionSize, PAGE_EXECUTE_READWRITE, &thunkMemInfo.Protect))
						return HRESULT_FROM_WIN32(GetLastError());

					if (OldProc)
						*OldProc = (PVOID)(DWORD)Thunk->u1.Function;

					Thunk->u1.Function = (DWORD)AlternateProc;

					if (!VirtualProtect(thunkMemInfo.BaseAddress, thunkMemInfo.RegionSize, thunkMemInfo.Protect, &junk))
						return HRESULT_FROM_WIN32(GetLastError());

					return S_OK;
				}
			}

			return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
		}
	}

	return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

// Only useful as SafeDisc sometimes loops through to find the DLLs real address (rather than the hooked Shim address or IAT jmp)
DWORD FindRealAddress(const char* szDLLName, const char* szProcName, void* pChangeAddressTo, void** OriginalFunction)
{
	DWORD ret = -1L;
	DWORD dwChangeAddressTo = (DWORD)pChangeAddressTo;
	HMODULE lib = LoadLibraryEx(szDLLName, NULL, DONT_RESOLVE_DLL_REFERENCES);
	PIMAGE_NT_HEADERS header = (PIMAGE_NT_HEADERS)((BYTE*)lib + ((PIMAGE_DOS_HEADER)lib)->e_lfanew);
	PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)lib + header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	DWORD* names = (DWORD*)((int)lib + exports->AddressOfNames);
	WORD* ords = (WORD*)((int)lib + exports->AddressOfNameOrdinals);
	DWORD* funcs = (DWORD*)((int)lib + exports->AddressOfFunctions);
	for (DWORD i = 0; i < exports->NumberOfNames; i++)
	{
		char* szFoundName = (char*)lib + (DWORD)names[i];
		if (szFoundName && _stricmp(szProcName, szFoundName) == 0)
		{
			// TODO: Worry about Ordinalbase ???
			ret = ((DWORD)lib) + (DWORD)funcs[ords[i]];

			log("Found Export: %s at %08X (FuncPtr: %08X)\n", szFoundName, ret, (DWORD)&funcs[ords[i]]);

			if (dwChangeAddressTo != 0)
			{
				WriteProtectedDWORD((DWORD)&funcs[ords[i]], dwChangeAddressTo - ((DWORD)lib));
			}

			break;
		}
	}
	if (OriginalFunction && ret != -1L)
		*OriginalFunction = (void*)ret;
	return ret;
}

void EnableDebugPriv()
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, false, &tkp, sizeof(tkp), NULL, NULL);

	CloseHandle(hToken);
}

void InjectDCEAPIHook(DWORD pid)
{
	if (GetFileAttributes("DCEAPIHook.dll") != -1L)
	{
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
		char szPath[MAX_PATH];
		GetFullPathNameA("DCEAPIHook.dll", MAX_PATH, szPath, NULL);
		LPVOID newMemory = (LPVOID)VirtualAllocEx(hProcess, NULL, strlen(szPath) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		WriteProcessMemory(hProcess, newMemory, szPath, strlen(szPath) + 1, NULL);
		HANDLE hNewThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, newMemory, NULL, NULL);
		WaitForSingleObject(hNewThread, INFINITE);
		CloseHandle(hNewThread);
		CloseHandle(hProcess);
	}
}

bool EndsInTmp(const char* szFilename)
{
	bool ret = false;
	if (szFilename != NULL)
	{
		const char* dot = strrchr(szFilename, '.');
		if (_stricmp(dot, ".tmp") == 0)
			ret = true;
	}
	return ret;
}

DWORD ReverseBytes(DWORD value) { return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) | ((value << 24) & 0xFF000000); }

bool UnProtectAddress(BYTE* Addr, int size)
{
	bool ret = true;
	DWORD oldProtect;
	if (!VirtualProtect((LPVOID)Addr, size, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		printf("Failed to unprotect addr:  0x%X\n", (uintptr_t)Addr);
		ret = false;
	}
	return ret;
}

void UnProtect_memcpy(void* _Dst, void const* _Src, size_t _Size)
{
	DWORD oldProtect, oldProtect2;
	if (VirtualProtect((LPVOID)_Dst, _Size, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		memcpy(_Dst, _Src, _Size);
		VirtualProtect((LPVOID)_Dst, _Size, oldProtect, &oldProtect2);
		logc(FOREGROUND_RED | FOREGROUND_BLUE, "UnProtect_memcpy From: %X8 To %08X (Size: %X)\n", (uintptr_t)_Src, (uintptr_t)_Dst, _Size);
	}
	else
		logc(FOREGROUND_RED, "Failed to unprotect addr:  0x%X\n", (uintptr_t)_Dst);
}

BOOL bFirstGetch = TRUE;
char GetKey(bool bForce)
{
	char ret = 0;
	if (loggingEnabled && (bFirstGetch || bForce))
	{
		logc(FOREGROUND_RED, "Press any key...\n");
		ret = _getch();
		bFirstGetch = FALSE;
	}
	return ret;
}
