#pragma once
#include <windows.h>
#include "minhook/MinHook.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _M_X64
#define PPEB __readgsqword(0x60)
#else
#define PPEB __readfsdword(0x30)
#endif

uintptr_t GetBaseAddress()
{
	return (uintptr_t)(*(PVOID*)(PPEB + 0x10));
}

bool ByteSearch(const BYTE* data, const BYTE* pattern, const bool* wildcard, int patternSize)
{
	bool matched = true;
	for (int i = 0; i < patternSize; i++)
	{
		if (wildcard && wildcard[i])
			continue;
		if (data[i] != pattern[i])
		{
			matched = false;
			break;
		}
	}
	return matched;
}

BYTE* ByteSearch(BYTE* data, const int dataSize, const BYTE* pattern, const bool* wildcard, int patternSize)
{
	BYTE* ret = NULL;

	for (int i = 0; i < dataSize-patternSize; i++)
	{
		if (ByteSearch(data + i, pattern, wildcard, patternSize))
		{
			ret = data + i;
			break;
		}
	}

	return ret;
}

BYTE* ByteSearch(BYTE* data, const int dataSize, const char* szPattern)
{
	int patternSize = (int)((strlen(szPattern) + 1) / 3);
	char* szPatternDupe = _strdup(szPattern);
	char* strPtr = szPatternDupe;
	BYTE* pattern = new BYTE[patternSize]();
	bool* wildcard = new bool[patternSize]();
	for (int i = 0; i < patternSize; i++)
	{
		strPtr[2] = 0;
		if (_stricmp(strPtr, "??") == 0)
			wildcard[i] = true;
		else
			pattern[i] = (BYTE)strtol(strPtr, NULL, 16);
		strPtr += 3;
	}

	BYTE* ret = ByteSearch(data, dataSize, pattern, wildcard, patternSize);

	free(szPatternDupe);
	delete[] pattern;
	delete[] wildcard;

	return ret;
}

bool CreateMinHook(LPVOID pTarget, LPVOID pDetour, LPVOID* result, bool enabled = true)
{
	MH_STATUS status = MH_Initialize();

	if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
		return false;

	if ((status = MH_CreateHook(pTarget, pDetour, result)) != MH_OK)
	{
		printf("MH_CreateHook Failed = %d\n", status);
		return (status == MH_OK);
	}

	if (enabled)
		status = MH_EnableHook(pTarget);

	return (status == MH_OK);
}

void CreateConsole()
{
	AllocConsole();

	FILE* fDummy;
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
}

void ClearConsole()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD consoleSize;
	DWORD charsWritten;
	COORD coordScreen = { 0, 0 };

	// Get the number of character cells in the current buffer
	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;

	consoleSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks
	FillConsoleOutputCharacter(hConsole, ' ', consoleSize, coordScreen, &charsWritten);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, consoleSize, coordScreen, &charsWritten);

	// Put the cursor at the top-left corner
	SetConsoleCursorPosition(hConsole, coordScreen);
}

void HideConsoleCursor() 
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);

	cursorInfo.bVisible = FALSE;  // Set visibility to false
	SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void DumpToFile(const char* szFilename, BYTE* data, int dataSize)
{
	FILE* fout = fopen(szFilename, "wb");
	if (fout)
	{
		fwrite(data, 1, dataSize, fout);
		fclose(fout);
	}
	else
		printf("Could not DumpToFile: %s\n", szFilename);
}

bool GetExecutableDirectory(char* outPath, DWORD size) 
{
	char fullPath[MAX_PATH];

	// Get full path to the executable
	DWORD length = GetModuleFileNameA(NULL, fullPath, MAX_PATH);
	if (length == 0 || length == MAX_PATH) 
		return false; // Failed or path was truncated

	// Copy to output buffer
	strncpy(outPath, fullPath, size);
	outPath[size - 1] = '\0';

	char* lastBackslash = strrchr(outPath, '\\');
	if (lastBackslash != NULL) 
	{
		*lastBackslash = '\0';
		return true; 
	}

	return false;
}