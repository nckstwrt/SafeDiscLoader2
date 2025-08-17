#ifndef _UTILS_H_
#define _UTILS_H_

#include <windows.h>
#include <stdio.h>

extern FILE* log_file;

void SetLogging(bool enable, const char* logFileName = NULL);
void logc(WORD col, const char* fmt, ...);
void log(const char* fmt, ...);
void LogKey(const char *keyName, DWORD addr, int keyLength = 16);
BYTE hexdigit(char hex);
BYTE hexbyte(const char* hex);
DWORD FindHex(DWORD StartAddr, DWORD EndAddr, BYTE *searchHex, DWORD searchSize);
DWORD FindHexString(DWORD StartAddr, DWORD EndAddr, const char *szHexString, const char *szPurpose = NULL);
BOOL GetSafeDiscVersionFromBuffer(BYTE *buffer, DWORD dwBufferSize, DWORD *pdwVersion, DWORD *pdwSubVersion, DWORD *pdwRevision);
BOOL GetSafeDiscVersion(const char *szExeFile, DWORD *pdwVersion, DWORD *pdwSubVersion, DWORD *pdwRevision);
PIMAGE_SECTION_HEADER GetSectionByName(DWORD addr, const char *szName);
BOOL WriteProtectedDWORD(DWORD Addr, DWORD Value);
HRESULT PatchIat(HMODULE Module, PSTR ImportedModuleName, PSTR ImportedProcName, PVOID AlternateProc, PVOID *OldProc);
DWORD FindRealAddress(const char *szDLLName, const char *szProcName, void* pChangeAddressTo = 0, void **OriginalFunction = NULL);
void EnableDebugPriv();
void InjectDCEAPIHook(DWORD pid);
bool EndsInTmp(const char* szFilename);
DWORD ReverseBytes(DWORD value);
bool UnProtectAddress(BYTE* Addr, int size);
void UnProtect_memcpy(void* _Dst, void const* _Src, size_t _Size);
char GetKey(bool bForce = false);

#ifdef FOREGROUND_BLUE
#undef FOREGROUND_BLUE
#undef FOREGROUND_GREEN
#undef FOREGROUND_RED
#endif

enum ForegroundColor
{
	FOREGROUND_WHITE,
	FOREGROUND_BLUE,
	FOREGROUND_GREEN,
	FOREGROUND_RED,
	FOREGROUND_GREY,
	FOREGROUND_CYAN,
	FOREGROUND_LIME,
	FOREGROUND_YELLOW,
	FOREGROUND_ORANGE,
	FOREGROUND_PINK,
	FOREGROUND_MAGENTA,
	FOREGROUND_PURPLE,
	FOREGROUND_BROWN,
	FOREGROUND_TURQUOISE
};

#endif