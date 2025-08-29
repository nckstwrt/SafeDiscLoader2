#pragma warning(disable: 4731) // Disable warning C4731: 'Decrypt24Function': frame pointer register 'ebx' modified by inline assembly cod
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include "utils.h"

extern HINSTANCE hInstanceAuthServ;
extern HINSTANCE hInstanceSecServ;

DWORD DecryptA5Block(DWORD A5Block_41C3D0, DWORD Block41A31C, DWORD Offset_4198F8, DWORD BaseAddr)
{
	DWORD esi;
	while (true)
	{
		memcpy((void*)Block41A31C, (void*)(((DWORD)BaseAddr) + Offset_4198F8), 0x190);

		DWORD edx = *(DWORD*)(Block41A31C + 0x78);
		esi = *(DWORD*)(Block41A31C);
		esi <<= 4;
		esi ^= *(DWORD*)(Block41A31C + 0x40);
		//log("edx: %08X esi: %08X\n", edx, esi);

		// sub_004014B0
		for (int i = 0x1A; i != 0; i--)
		{
			DWORD eax = *(DWORD*)(Block41A31C);
			eax *= i;
			*(DWORD*)(((DWORD)Block41A31C) + (i * 4)) ^= eax;
		}
		LogKey("Block41A31C", (DWORD)Block41A31C);

		DWORD Offset_419904 = *(DWORD*)(Block41A31C + 0x8);
		DWORD eax = *(DWORD*)(Block41A31C + 0x14);
		eax &= 0x40000000;
		if (eax != 0)
		{
			eax = *(DWORD*)(Block41A31C + 0x50);
			*(DWORD*)(((DWORD)A5Block_41C3D0) + (eax * 4)) = edx; // mov dword ptr ds:[eax*4+41C3D0],edx |   <----- A5 Block
		}
		Offset_4198F8 = Offset_419904;
		if (Offset_4198F8 == 0)
			break;
	}
	return esi;
}

BYTE* Decrypt24Function() // bOOls eYe code
{
	BYTE* MentalMemory = new BYTE[0x10000];
	DWORD AuthServTxtAddr = ((DWORD)hInstanceAuthServ) + GetSectionByName(((DWORD)hInstanceAuthServ), ".txt")->VirtualAddress; // 971000

	logc(FOREGROUND_GREEN, "Decrypt24Function = %08X\n", Decrypt24Function);
	logc(FOREGROUND_GREEN, "AuthServTxtAddr = %08X\n", AuthServTxtAddr);

	__asm {
		pushad
		mov edi, [MentalMemory]
		push edi
		mov edx, AuthServTxtAddr
		add edx, 0x7AD0
		call edx // call AuthServTxtAddr+0x7AD0 //~de6c29.978AD0 - 971000 = 7AD0
		add esp, 4
		add edi, 0x30
		mov edx, AuthServTxtAddr
		add edx, 0xD230
		call edx // call AuthServTxtAddr + 0xD230 // ~de6c29.97E230 - 971000 = D230
		add eax, 4
		stosd
		mov edx, AuthServTxtAddr
		add edx, 0x4380
		call edx // call AuthServTxtAddr + 0x4380 // ~de6c29.975380 - 971000 = 4380
		mov ebx, eax
		push ebx
		mov edx, AuthServTxtAddr
		add edx, 0x10000
		call edx // call AuthServTxtAddr + 0x10000 // ~de6c29.981000 - 971000 = 10000
		add esp, 4
		stosd
		push ebx
		mov edx, AuthServTxtAddr
		add edx, 0x10110
		call edx // call AuthServTxtAddr + 0x10110 // ~de6c29.981110 - 971000 = 10110
		add esp, 4
		stosd
		push ebx
		mov edx, AuthServTxtAddr
		add edx, 0x10440
		call edx // call AuthServTxtAddr + 0x10440 // ~de6c29.981440 - 971000 = 10440 
		add esp, 4
		stosd
		push ebx
		mov edx, AuthServTxtAddr
		add edx, 0x10220
		call edx // call AuthServTxtAddr + 0x10220 // ~de6c29.981220 - 971000 = 10220
		add esp, 4
		stosd
		popad
	}

	LogKey("FiftyBytesPtr/MentalMemory", (DWORD)MentalMemory, 0x50);

	BYTE ThirtyBytes[0x30];
	DWORD ThirtyBytesPtr = *(DWORD*)(&MentalMemory[0x30]);
	memcpy(ThirtyBytes, (void*)ThirtyBytesPtr, 0x30);
	LogKey("ThirtyBytes", (DWORD)ThirtyBytes, 0x30);

	DWORD KeyPtr = ((DWORD)hInstanceAuthServ) + 0x2B039;
	logc(FOREGROUND_GREEN, "KeyPtr = %08X\n", KeyPtr);

	DWORD TableSrc1 = *((DWORD*)KeyPtr);
	DWORD TableSrc2 = TableSrc1 + 0x555;
	DWORD TableSrc3 = TableSrc2 + 0x555;
	logc(FOREGROUND_GREEN, "TableSrc1 = %08X Data: %08X\n", TableSrc1, ReverseBytes(*((DWORD*)TableSrc1)));
	logc(FOREGROUND_GREEN, "TableSrc2 = %08X Data: %08X\n", TableSrc2, ReverseBytes(*((DWORD*)TableSrc2)));
	logc(FOREGROUND_GREEN, "TableSrc3 = %08X Data: %08X\n", TableSrc3, ReverseBytes(*((DWORD*)TableSrc3)));

	BYTE* DecryptedKeys = new BYTE[(0x400 * 3) + 0x1000];
	BYTE* DecryptedKey1 = DecryptedKeys;
	BYTE* DecryptedKey2 = DecryptedKeys + 0x400;
	BYTE* DecryptedKey3 = DecryptedKeys + (0x400 * 2);
	memcpy(DecryptedKey1, (void*)TableSrc1, 0x400);
	memcpy(DecryptedKey2, (void*)TableSrc2, 0x400);
	memcpy(DecryptedKey3, (void*)TableSrc3, 0x400);

	DWORD edi = 0;

	DWORD xor_key1 = 0x9CFE94E0 ^ 0xD78A3FE9 ^ 0xA167D52E ^ 0xAB3E1BFC ^ 0x3D42A236;
	logc(FOREGROUND_GREEN, "xor_key1 = %08X\n", xor_key1);
	for (int i = 0; i <= 0x7FC; i += 4)
		*(DWORD*)(DecryptedKeys + i) ^= xor_key1;
	LogKey("DecryptedKeys After 1", (DWORD)DecryptedKeys, 0x10);

	DWORD xor_key2 = (*(DWORD*)(MentalMemory + 0x34)) ^ (*(DWORD*)(MentalMemory + 0x38)) ^ (*(DWORD*)(MentalMemory + 0x3C)) ^ (*(DWORD*)(MentalMemory + 0x40));
	logc(FOREGROUND_GREEN, "xor_key2 = %08X\n", xor_key2);
	for (int i = 0; i <= 0x7FC; i += 4)
		*(DWORD*)(DecryptedKeys + i) ^= xor_key2;
	LogKey("DecryptedKeys After 2", (DWORD)DecryptedKeys, 0x10);

	DWORD esi = (DWORD)MentalMemory;
	DWORD xor_key3_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key3_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key3_pt1 = %08X xor_key3_pt2 = %08X\n", xor_key3_pt1, xor_key3_pt2);
	for (int i = 0; i <= 0x3F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key3_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key3_pt2;
	}
	LogKey("DecryptedKeys After 3", (DWORD)DecryptedKeys, 0x10);

	// Destination changes here!
	edi = ((DWORD)DecryptedKey2) - ((DWORD)DecryptedKeys);  // Key 2

	DWORD xor_key4_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key4_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key4_pt1 = %08X xor_key4_pt2 = %08X\n", xor_key4_pt1, xor_key4_pt2);
	for (int i = 0; i <= 0x3F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + edi + i) ^= xor_key4_pt1;
		*(DWORD*)(DecryptedKeys + edi + i + 4) ^= xor_key4_pt2;
	}
	LogKey("DecryptedKeys After 4", (DWORD)DecryptedKeys, 0x10);

	edi = ((DWORD)DecryptedKey1) - ((DWORD)DecryptedKeys); // Key 1

	DWORD xor_key5_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key5_pt1 = %08X xor_key5_pt2 = %08X xor_key5_pt3 = %08X xor_key5_pt4 = %08X\n", xor_key5_pt1, xor_key5_pt2, xor_key5_pt3, xor_key5_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key5_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key5_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key5_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key5_pt4;
	}
	LogKey("DecryptedKeys After 5", (DWORD)DecryptedKeys, 0x10);

	DWORD xor_key6_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key6_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key6_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key6_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key6_pt1 = %08X xor_key6_pt2 = %08X xor_key6_pt3 = %08X xor_key6_pt4 = %08X\n", xor_key6_pt1, xor_key6_pt2, xor_key6_pt3, xor_key6_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key6_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key6_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key6_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key6_pt4;
	}
	LogKey("DecryptedKeys After 6", (DWORD)DecryptedKeys, 0x10);

	esi = (DWORD)ThirtyBytes;

	DWORD xor_key7_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key7_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key7_pt1 = %08X xor_key7_pt2 = %08X\n", xor_key7_pt1, xor_key7_pt2);
	for (int i = 0; i <= 0x7F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key7_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key7_pt2;
	}
	LogKey("DecryptedKeys After 7", (DWORD)DecryptedKeys, 0x10);

	DWORD xor_key8_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key8_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key8_pt1 = %08X xor_key8_pt2 = %08X\n", xor_key8_pt1, xor_key8_pt2);
	for (int i = 0; i <= 0x7F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key8_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key8_pt2;
	}
	LogKey("DecryptedKeys After 8", (DWORD)DecryptedKeys, 0x10);

	DWORD xor_key9_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key9_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key9_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key9_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key9_pt1 = %08X xor_key9_pt2 = %08X xor_key9_pt3 = %08X xor_key9_pt4 = %08X\n", xor_key9_pt1, xor_key9_pt2, xor_key9_pt3, xor_key9_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key9_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key9_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key9_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key9_pt4;
	}
	LogKey("DecryptedKeys After 9", (DWORD)DecryptedKeys, 0x10);

	DWORD xor_key10_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key10_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key10_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key10_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key10_pt1 = %08X xor_key10_pt2 = %08X xor_key10_pt3 = %08X xor_key10_pt4 = %08X\n", xor_key10_pt1, xor_key10_pt2, xor_key10_pt3, xor_key10_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key10_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key10_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key10_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key10_pt4;
	}
	LogKey("DecryptedKey1 After 10", (DWORD)DecryptedKey1, 0x10);
	LogKey("DecryptedKey2 After 10", (DWORD)DecryptedKey2, 0x10);
	LogKey("DecryptedKey3 After 10", (DWORD)DecryptedKey3, 0x10);

	//GetKey(true);

	DWORD A5Block_41C3D0 = ((DWORD)DecryptedKeys) + 0x0C04;
	memset((void*)A5Block_41C3D0, 0xA5, 0x80 * 4);

	BYTE Block41A31C[0x190];

	DWORD Offset_4198F8 = 0x2F3F8;
	esi = DecryptA5Block(A5Block_41C3D0, (DWORD)Block41A31C, Offset_4198F8, (DWORD)hInstanceAuthServ);

	LogKey("A5Block_41C3D0", A5Block_41C3D0, 0x80 * 4);

	log("Copying 0x50 to Block41A31C from esi: %08X\n", esi);
	memcpy(Block41A31C, (void*)(DWORD)esi, 0x50);

	DWORD eax = *(DWORD*)(((DWORD)Block41A31C) + 0x20);
	eax <<= 2;
	DWORD dst = *(DWORD*)(((DWORD)Block41A31C) + 0x44);
	logc(FOREGROUND_GREEN, "Writing A5Block_41C3D0 to %08X (size eax: %08X)\n", dst, eax);
	//GetKey(true);
	UnProtect_memcpy((void*)dst, (void*)A5Block_41C3D0, eax);

	// Blank the A5 Block
	memset((void*)A5Block_41C3D0, 0xA5, 0x80 * 4);

	Offset_4198F8 = 0x5B400;
	esi = DecryptA5Block(A5Block_41C3D0, (DWORD)Block41A31C, Offset_4198F8, (DWORD)hInstanceSecServ);

	LogKey("A5Block_41C3D0 (2nd One)", A5Block_41C3D0, 0x80 * 4);

	log("Copying 0x50 to Block41A31C from esi: %08X (2nd One)\n", esi);
	memcpy(Block41A31C, (void*)(DWORD)esi, 0x50);

	eax = *(DWORD*)(((DWORD)Block41A31C) + 0x20);
	eax <<= 2;
	dst = *(DWORD*)(((DWORD)Block41A31C) + 0x44);
	logc(FOREGROUND_GREEN, "Writing A5Block_41C3D0 to %08X (size eax: %08X) (2nd One)\n", dst, eax);
	UnProtect_memcpy((void*)dst, (void*)A5Block_41C3D0, eax);

	BYTE Block41D5D0[0x200];

	DWORD SecServPtr = ((DWORD)hInstanceSecServ) + 0x65280;
	memcpy(Block41D5D0, (void*)SecServPtr, 4);
	memcpy(Block41D5D0, (void*)*(DWORD*)Block41D5D0, 0x10);
	memcpy(Block41D5D0, (void*)(*(DWORD*)(Block41D5D0 + 0xc) + 0x20), 0x30);

	DWORD DestPtr1 = *(DWORD*)(Block41D5D0 + 4);
	DWORD DestPtr2 = *(DWORD*)(Block41D5D0 + 4 + 0x10);
	DWORD DestPtr3 = *(DWORD*)(Block41D5D0 + 4 + 0x20);

	UnProtect_memcpy((void*)DestPtr1, DecryptedKey1, 0x400);
	UnProtect_memcpy((void*)DestPtr2, DecryptedKey2, 0x400);
	UnProtect_memcpy((void*)DestPtr3, DecryptedKey3, 0x400);

	// GetKey();
	Sleep(10);	// Slight delay required if we aren't logging. Harry Potter and the Philosophers Stone seems to need this because it reloads itself
	//MessageBox(0, "Decrypted!", "Info", MB_OK);

	return MentalMemory;
}

BYTE* Decrypt25Function() // bOOls eYe code
{
	BYTE* MentalMemory = new BYTE[0x10000];
	DWORD AuthServTxtAddr = ((DWORD)hInstanceAuthServ) + GetSectionByName(((DWORD)hInstanceAuthServ), ".txt")->VirtualAddress; // 971000

	logc(FOREGROUND_GREEN, "Decrypt25Function = %08X\n", Decrypt25Function);
	logc(FOREGROUND_GREEN, "AuthServTxtAddr = %08X\n", AuthServTxtAddr);

	__asm {
		pushad
		mov edi, [MentalMemory]
		push edi
		mov edx, AuthServTxtAddr
		add edx, 0x7D40
		call edx //  call ~de9588.998D40 - 991000 =  7D40
		add esp, 4
		add edi, 0x60  // 30 -> 60
		mov edx, AuthServTxtAddr
		add edx, 0xD4A0
		call edx // call ~de9588.99E4A0 - 991000 = D4A0
		add eax, 4
		stosd
		mov edx, AuthServTxtAddr
		add edx, 0x4380
		call edx // call AuthServTxtAddr + 0x4380 // ~de6c29.975380 - 971000 = 4380
		mov ebx, eax
		push ebx
		mov edx, AuthServTxtAddr
		add edx, 0x10000
		call edx // call AuthServTxtAddr + 0x10000 // ~de6c29.981000 - 971000 = 10000
		add esp, 4
		stosd
		push ebx
		mov edx, AuthServTxtAddr
		add edx, 0x10110
		call edx // call AuthServTxtAddr + 0x10110 // ~de6c29.981110 - 971000 = 10110
		add esp, 4
		stosd
		push ebx
		mov edx, AuthServTxtAddr
		add edx, 0x10440
		call edx // call AuthServTxtAddr + 0x10440 // ~de6c29.981440 - 971000 = 10440 
		add esp, 4
		stosd
		push ebx
		mov edx, AuthServTxtAddr
		add edx, 0x10220
		call edx // call AuthServTxtAddr + 0x10220 // ~de6c29.981220 - 971000 = 10220
		add esp, 4
		stosd
		popad
	}

	LogKey("MentalMemory", (DWORD)MentalMemory, 0x80);

	BYTE ThirtyBytes[0x30];
	DWORD ThirtyBytesPtr = *(DWORD*)(&MentalMemory[0x60]);
	memcpy(ThirtyBytes, (void*)ThirtyBytesPtr, 0x30);
	LogKey("ThirtyBytes", (DWORD)ThirtyBytes, 0x30);

	DWORD KeyPtr = ((DWORD)hInstanceAuthServ) + 0x2B039;
	logc(FOREGROUND_GREEN, "KeyPtr = %08X\n", KeyPtr);

	DWORD TableSrc1 = *((DWORD*)KeyPtr);
	DWORD TableSrc2 = TableSrc1 + 0x555;
	DWORD TableSrc3 = TableSrc2 + 0x555;
	logc(FOREGROUND_GREEN, "TableSrc1 = %08X Data: %08X\n", TableSrc1, ReverseBytes(*((DWORD*)TableSrc1)));
	logc(FOREGROUND_GREEN, "TableSrc2 = %08X Data: %08X\n", TableSrc2, ReverseBytes(*((DWORD*)TableSrc2)));
	logc(FOREGROUND_GREEN, "TableSrc3 = %08X Data: %08X\n", TableSrc3, ReverseBytes(*((DWORD*)TableSrc3)));

	BYTE* DecryptedKeys = new BYTE[(0x400 * 3) + 0x1000];
	BYTE* DecryptedKey1 = DecryptedKeys;
	BYTE* DecryptedKey2 = DecryptedKeys + 0x400;
	BYTE* DecryptedKey3 = DecryptedKeys + (0x400 * 2);
	memcpy(DecryptedKey1, (void*)TableSrc1, 0x400);
	memcpy(DecryptedKey2, (void*)TableSrc2, 0x400);
	memcpy(DecryptedKey3, (void*)TableSrc3, 0x400);

	DWORD edi = 0;

	DWORD xor_key1 = 0x9CFE94E0 ^ 0xD78A3FE9 ^ 0xA167D52E ^ 0xAB3E1BFC ^ 0x3D42A236;		// Same as 24
	logc(FOREGROUND_GREEN, "xor_key1 = %08X\n", xor_key1);
	for (int i = 0; i <= 0x7FC; i += 4)
		*(DWORD*)(DecryptedKeys + i) ^= xor_key1;
	LogKey("DecryptedKey1 After 1", (DWORD)DecryptedKeys, 0x10);

	DWORD xor_key2 = (*(DWORD*)(MentalMemory + 0x64)) ^ (*(DWORD*)(MentalMemory + 0x68)) ^ (*(DWORD*)(MentalMemory + 0x6C)) ^ (*(DWORD*)(MentalMemory + 0x70));
	logc(FOREGROUND_GREEN, "xor_key2 = %08X\n", xor_key2);
	for (int i = 0; i <= 0x7FC; i += 4)
		*(DWORD*)(DecryptedKeys + i) ^= xor_key2;
	LogKey("DecryptedKey1 After 2", (DWORD)DecryptedKeys, 0x10);

	// Destination changes here! (changed after xor 4 in 2.40)
	edi = ((DWORD)DecryptedKey2) - ((DWORD)DecryptedKeys);  // Key 2

	DWORD esi = (DWORD)MentalMemory;
	DWORD xor_key3_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key3_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key3_pt1 = %08X xor_key3_pt2 = %08X\n", xor_key3_pt1, xor_key3_pt2);
	for (int i = 0; i <= 0x3F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + edi + i) ^= xor_key3_pt1;
		*(DWORD*)(DecryptedKeys + edi + i + 4) ^= xor_key3_pt2;
	}
	LogKey("DecryptedKey2 After 3", (DWORD)DecryptedKey2, 0x10);

	DWORD xor_key4_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key4_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key4_pt1 = %08X xor_key4_pt2 = %08X\n", xor_key4_pt1, xor_key4_pt2);
	for (int i = 0; i <= 0x3F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + edi + i) ^= xor_key4_pt1;
		*(DWORD*)(DecryptedKeys + edi + i + 4) ^= xor_key4_pt2;
	}
	LogKey("DecryptedKey2 After 4", (DWORD)DecryptedKey2, 0x10);

	edi = 0; // Key 1

	DWORD xor_key5_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key5_pt1 = %08X xor_key5_pt2 = %08X xor_key5_pt3 = %08X xor_key5_pt4 = %08X\n", xor_key5_pt1, xor_key5_pt2, xor_key5_pt3, xor_key5_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key5_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key5_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key5_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key5_pt4;
	}
	LogKey("DecryptedKey1 After 5", (DWORD)DecryptedKeys, 0x10);

	DWORD xor_key6_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key6_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key6_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key6_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key6_pt1 = %08X xor_key6_pt2 = %08X xor_key6_pt3 = %08X xor_key6_pt4 = %08X\n", xor_key6_pt1, xor_key6_pt2, xor_key6_pt3, xor_key6_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key6_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key6_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key6_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key6_pt4;
	}
	LogKey("DecryptedKey1 After 6", (DWORD)DecryptedKeys, 0x10);

	edi = ((DWORD)DecryptedKey2) - ((DWORD)DecryptedKeys);  // Key 2

	DWORD xor_key7_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key7_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key7_pt1 = %08X xor_key7_pt2 = %08X\n", xor_key7_pt1, xor_key7_pt2);
	for (int i = 0; i <= 0x3F8; i += 8)     // Different Size to 2.40
	{
		*(DWORD*)(DecryptedKeys + i + edi) ^= xor_key7_pt1;
		*(DWORD*)(DecryptedKeys + i + edi + 4) ^= xor_key7_pt2;
	}
	LogKey("DecryptedKey2 After 7", (DWORD)DecryptedKey2, 0x10);

	DWORD xor_key8_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key8_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key8_pt1 = %08X xor_key8_pt2 = %08X\n", xor_key8_pt1, xor_key8_pt2);
	for (int i = 0; i <= 0x3F8; i += 8)	   // Different Size to 2.40
	{
		*(DWORD*)(DecryptedKeys + i + edi) ^= xor_key8_pt1;
		*(DWORD*)(DecryptedKeys + i + edi + 4) ^= xor_key8_pt2;
	}
	LogKey("DecryptedKey1 After 8", (DWORD)DecryptedKey1, 0x10);
	LogKey("DecryptedKey2 After 8", (DWORD)DecryptedKey2, 0x10);

	edi = 0; // Key 1

	DWORD xor_key9_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key9_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key9_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key9_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key9_pt1 = %08X xor_key9_pt2 = %08X xor_key9_pt3 = %08X xor_key9_pt4 = %08X\n", xor_key9_pt1, xor_key9_pt2, xor_key9_pt3, xor_key9_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key9_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key9_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key9_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key9_pt4;
	}
	LogKey("DecryptedKey1 After 9", (DWORD)DecryptedKey1, 0x10);
	LogKey("DecryptedKey2 After 9", (DWORD)DecryptedKey2, 0x10);

	DWORD xor_key10_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key10_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key10_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key10_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key10_pt1 = %08X xor_key10_pt2 = %08X xor_key10_pt3 = %08X xor_key10_pt4 = %08X\n", xor_key10_pt1, xor_key10_pt2, xor_key10_pt3, xor_key10_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key10_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key10_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key10_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key10_pt4;
	}
	LogKey("DecryptedKey1 After 10", (DWORD)DecryptedKey1, 0x10);
	LogKey("DecryptedKey2 After 10", (DWORD)DecryptedKey2, 0x10);
	LogKey("DecryptedKey3 After 10", (DWORD)DecryptedKey3, 0x10);

	esi = (DWORD)ThirtyBytes;

	DWORD xor_key11_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key11_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key11_pt1 = %08X xor_key11_pt2 = %08X\n", xor_key11_pt1, xor_key11_pt2);
	for (int i = 0; i <= 0x7F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key11_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key11_pt2;
	}
	LogKey("DecryptedKey1 After 11", (DWORD)DecryptedKey1, 0x10);

	DWORD xor_key12_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key12_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key12_pt1 = %08X xor_key12_pt2 = %08X\n", xor_key12_pt1, xor_key12_pt2);
	for (int i = 0; i <= 0x7F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key12_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key12_pt2;
	}
	LogKey("DecryptedKey1 After 12", (DWORD)DecryptedKey1, 0x10);

	DWORD xor_key13_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key13_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key13_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key13_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key13_pt1 = %08X xor_key13_pt2 = %08X xor_key13_pt3 = %08X xor_key13_pt4 = %08X\n", xor_key13_pt1, xor_key13_pt2, xor_key13_pt3, xor_key13_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key13_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key13_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key13_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key13_pt4;
	}
	LogKey("DecryptedKey1 After 13", (DWORD)DecryptedKey1, 0x10);

	DWORD xor_key14_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key14_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key14_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key14_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key14_pt1 = %08X xor_key14_pt2 = %08X xor_key14_pt3 = %08X xor_key14_pt4 = %08X\n", xor_key14_pt1, xor_key14_pt2, xor_key14_pt3, xor_key14_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key14_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key14_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key14_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key14_pt4;
	}
	LogKey("DecryptedKey1 After 14", (DWORD)DecryptedKey1, 0x10);
	LogKey("DecryptedKey2 After 14", (DWORD)DecryptedKey2, 0x10);
	LogKey("DecryptedKey3 After 14", (DWORD)DecryptedKey3, 0x10);

	DWORD A5Block_41C3D0 = ((DWORD)DecryptedKeys) + 0x0C04;
	memset((void*)A5Block_41C3D0, 0xA5, 0x80 * 4);

	BYTE Block41A31C[0x190];

	DWORD Offset_4198F8 = 0x2F3F8;
	esi = DecryptA5Block(A5Block_41C3D0, (DWORD)Block41A31C, Offset_4198F8, (DWORD)hInstanceAuthServ);

	LogKey("A5Block_41C3D0", A5Block_41C3D0, 0x80 * 4);

	log("Copying 0x50 to Block41A31C from esi: %08X\n", esi);
	memcpy(Block41A31C, (void*)(DWORD)esi, 0x50);

	DWORD eax = *(DWORD*)(((DWORD)Block41A31C) + 0x20);
	eax <<= 2;
	DWORD dst = *(DWORD*)(((DWORD)Block41A31C) + 0x44);
	logc(FOREGROUND_GREEN, "Writing A5Block_41C3D0 to %08X (size eax: %08X)\n", dst, eax);
	//GetKey(true);
	UnProtect_memcpy((void*)dst, (void*)A5Block_41C3D0, eax);

	// Blank the A5 Block
	memset((void*)A5Block_41C3D0, 0xA5, 0x80 * 4);

	Offset_4198F8 = 0x72720;  // Different offset that 2.40
	esi = DecryptA5Block(A5Block_41C3D0, (DWORD)Block41A31C, Offset_4198F8, (DWORD)hInstanceSecServ);

	LogKey("A5Block_41C3D0 (2nd One)", A5Block_41C3D0, 0x80 * 4);

	log("Copying 0x50 to Block41A31C from esi: %08X (2nd One)\n", esi);
	memcpy(Block41A31C, (void*)(DWORD)esi, 0x50);

	eax = *(DWORD*)(((DWORD)Block41A31C) + 0x20);
	eax <<= 2;
	dst = *(DWORD*)(((DWORD)Block41A31C) + 0x44);
	logc(FOREGROUND_GREEN, "Writing A5Block_41C3D0 to %08X (size eax: %08X) (2nd One)\n", dst, eax);
	UnProtect_memcpy((void*)dst, (void*)A5Block_41C3D0, eax);

	BYTE Block41D5D0[0x200];

	DWORD SecServPtr = ((DWORD)hInstanceSecServ) + 0x7D42C;		// Different to 2.40
	memcpy(Block41D5D0, (void*)SecServPtr, 4);
	memcpy(Block41D5D0, (void*)*(DWORD*)Block41D5D0, 0x10);
	memcpy(Block41D5D0, (void*)(*(DWORD*)(Block41D5D0 + 0xc) + 0x20), 0x30);

	DWORD DestPtr1 = *(DWORD*)(Block41D5D0 + 4);
	DWORD DestPtr2 = *(DWORD*)(Block41D5D0 + 4 + 0x10);
	DWORD DestPtr3 = *(DWORD*)(Block41D5D0 + 4 + 0x20);

	UnProtect_memcpy((void*)DestPtr1, DecryptedKey1, 0x400);
	UnProtect_memcpy((void*)DestPtr2, DecryptedKey2, 0x400);
	UnProtect_memcpy((void*)DestPtr3, DecryptedKey3, 0x400);

	logc(FOREGROUND_GREEN, "Finished Decrypt25Function\n");
	//GetKey(true);

	Sleep(10);

	return MentalMemory;
}

DWORD DecryptA5BlockFor23(DWORD A5Block_41C3D0, DWORD Block41A31C, DWORD Offset_4198F8, DWORD BaseAddr)
{
	while (true)
	{
		memcpy((void*)Block41A31C, (void*)(((DWORD)BaseAddr) + Offset_4198F8), 0x190);

		DWORD edx = *(DWORD*)(Block41A31C + 0x54);

		// sub_004014B0
		for (int i = 0x1A; i != 0; i--)
		{
			DWORD eax = *(DWORD*)(Block41A31C);
			eax *= i;
			*(DWORD*)(((DWORD)Block41A31C) + (i * 4)) ^= eax;
		}

		DWORD Offset_419904 = *(DWORD*)(Block41A31C + 0x8);
		DWORD eax = *(DWORD*)(Block41A31C + 0x14);
		eax &= 0x40000000;
		if (eax != 0)
		{
			eax = *(DWORD*)(Block41A31C + 0x2c);
			*(DWORD*)(((DWORD)A5Block_41C3D0) + (eax * 4)) = edx; // mov dword ptr ds:[eax*4+41C3D0],edx |   <----- A5 Block
		}
		Offset_4198F8 = Offset_419904;
		if (Offset_4198F8 == 0)
			break;
	}
	return *(DWORD*)(((DWORD)Block41A31C) + 0x68);
}

BYTE* Decrypt23Function() // bOOls eYe code
{
	BYTE* MentalMemory = new BYTE[0x10000];

	//GetKey(true);

	BYTE *ThirtyBytes = new BYTE[0x30];		// 41B34C

	__asm
	{
		mov eax, [ThirtyBytes]
		push eax
		mov eax, [hInstanceAuthServ]
		add eax, 0x8480
		call eax
		add esp, 4
	}

	//DWORD DWORD_40F467 = 0x35000;
	//memcpy(ThirtyBytes, (void*)(((DWORD)hInstanceAuthServ)+DWORD_40F467), 0x30);
		
	LogKey("ThirtyBytes", (DWORD)ThirtyBytes, 0x30);

	//GetKey(true);

	DWORD KeyPtr = ((DWORD)hInstanceAuthServ) + 0x26039;
	logc(FOREGROUND_GREEN, "KeyPtr = %08X\n", KeyPtr);

	DWORD TableSrc1 = *((DWORD*)KeyPtr);
	DWORD TableSrc2 = TableSrc1 + 0x555;
	DWORD TableSrc3 = TableSrc2 + 0x555;
	logc(FOREGROUND_GREEN, "TableSrc1 = %08X Data: %08X\n", TableSrc1, ReverseBytes(*((DWORD*)TableSrc1)));
	logc(FOREGROUND_GREEN, "TableSrc2 = %08X Data: %08X\n", TableSrc2, ReverseBytes(*((DWORD*)TableSrc2)));
	logc(FOREGROUND_GREEN, "TableSrc3 = %08X Data: %08X\n", TableSrc3, ReverseBytes(*((DWORD*)TableSrc3)));

	BYTE* DecryptedKeys = new BYTE[(0x400 * 3) + 0x1000];
	BYTE* DecryptedKey1 = DecryptedKeys;
	BYTE* DecryptedKey2 = DecryptedKeys + 0x400;
	BYTE* DecryptedKey3 = DecryptedKeys + (0x400 * 2);
	memcpy(DecryptedKey1, (void*)TableSrc1, 0x400);
	memcpy(DecryptedKey2, (void*)TableSrc2, 0x400);
	memcpy(DecryptedKey3, (void*)TableSrc3, 0x400);

	DWORD edi = 0;
	DWORD esi = (DWORD)ThirtyBytes;

	DWORD xor_key1 = 0x59416F75;
	logc(FOREGROUND_GREEN, "xor_key1 = %08X\n", xor_key1);
	for (int i = 0; i <= 0x7FC; i += 4)
		*(DWORD*)(DecryptedKeys + i) ^= xor_key1;
	LogKey("DecryptedKey1 After 1", (DWORD)DecryptedKey1, 0x10);

	DWORD xor_key2_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key2_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key2_pt1 = %08X xor_key2_pt2 = %08X\n", xor_key2_pt1, xor_key2_pt2);
	for (int i = 0; i <= 0x3F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + edi + i) ^= xor_key2_pt1;
		*(DWORD*)(DecryptedKeys + edi + i + 4) ^= xor_key2_pt2;
	}
	LogKey("DecryptedKey1 After 2", (DWORD)DecryptedKey1, 0x10);

	edi = ((DWORD)DecryptedKey2) - ((DWORD)DecryptedKeys);  // Key 2

	DWORD xor_key3_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key3_pt2 = *(DWORD*)(esi); esi += 4;
	log("xor_key3_pt1 = %08X xor_key3_pt2 = %08X\n", xor_key3_pt1, xor_key3_pt2);
	for (int i = 0; i <= 0x3F8; i += 8)
	{
		*(DWORD*)(DecryptedKeys + edi + i) ^= xor_key3_pt1;
		*(DWORD*)(DecryptedKeys + edi + i + 4) ^= xor_key3_pt2;
	}
	LogKey("DecryptedKey2 After 3", (DWORD)DecryptedKey2, 0x10);

	edi = 0; // Key 1

	DWORD xor_key4_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key4_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key4_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key4_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key4_pt1 = %08X xor_key4_pt2 = %08X xor_key4_pt3 = %08X xor_key4_pt4 = %08X\n", xor_key4_pt1, xor_key4_pt2, xor_key4_pt3, xor_key4_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key4_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key4_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key4_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key4_pt4;
	}
	LogKey("DecryptedKey1 After 4", (DWORD)DecryptedKeys, 0x10);


	DWORD xor_key5_pt1 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt2 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt3 = *(DWORD*)(esi); esi += 4;
	DWORD xor_key5_pt4 = *(DWORD*)(esi); esi += 4;
	log("xor_key5_pt1 = %08X xor_key5_pt2 = %08X xor_key5_pt3 = %08X xor_key5_pt4 = %08X\n", xor_key5_pt1, xor_key5_pt2, xor_key5_pt3, xor_key5_pt4);
	for (int i = 0; i <= 0x7F0; i += 16)
	{
		*(DWORD*)(DecryptedKeys + i) ^= xor_key5_pt1;
		*(DWORD*)(DecryptedKeys + i + 4) ^= xor_key5_pt2;
		*(DWORD*)(DecryptedKeys + i + 8) ^= xor_key5_pt3;
		*(DWORD*)(DecryptedKeys + i + 12) ^= xor_key5_pt4;
	}
	LogKey("DecryptedKey1 After 5", (DWORD)DecryptedKeys, 0x10);

	//GetKey(true);

	BYTE* Block41D5D0 = new BYTE[0x200];
	BYTE* Block41A31C = new BYTE[0x190];
	BYTE* A5Block_41C3D0 = new BYTE[0x80 * 4];
	memset((void*)A5Block_41C3D0, 0xA5, 0x80 * 4);

	DWORD Offset_4198F8 = *(DWORD*)(0x2E900 + ((DWORD)hInstanceAuthServ));

	DWORD eax = DecryptA5BlockFor23((DWORD)A5Block_41C3D0, (DWORD)Block41A31C, Offset_4198F8, (DWORD)hInstanceAuthServ);

	LogKey("A5Block_41C3D0", (DWORD)A5Block_41C3D0, 0x80 * 4);

	memcpy(Block41A31C, (void*)(DWORD)eax, 0x50);
	memcpy((void*)*(DWORD*)(Block41A31C + 0x44), A5Block_41C3D0, 0x100);

	DWORD SecServPtr = ((DWORD)hInstanceSecServ) + 0x63200;
	memcpy(Block41D5D0, (void*)SecServPtr, 4);
	memcpy(Block41D5D0, (void*)*(DWORD*)Block41D5D0, 0x10);
	memcpy(Block41D5D0, (void*)(*(DWORD*)(Block41D5D0 + 0xc) + 0x20), 0x30);

	DWORD DestPtr1 = *(DWORD*)(Block41D5D0 + 0xc);
	DWORD DestPtr2 = *(DWORD*)(Block41D5D0 + 0xc + 0x10);
	DWORD DestPtr3 = *(DWORD*)(Block41D5D0 + 0xc + 0x20);

	logc(FOREGROUND_BLUE, "Block41D5D0 = %08X DestPtr1 = %08X DestPtr2 = %08X DestPtr3 = %08X\n", Block41D5D0, DestPtr1, DestPtr2, DestPtr3);

	//GetKey(true);

	LogKey("DecryptedKey1", (DWORD)DecryptedKey1);
	LogKey("DecryptedKey2", (DWORD)DecryptedKey2);
	LogKey("DecryptedKey3", (DWORD)DecryptedKey3);

	UnProtect_memcpy((void*)DestPtr1, DecryptedKey1, 0x400);
	UnProtect_memcpy((void*)DestPtr2, DecryptedKey2, 0x400);
	UnProtect_memcpy((void*)DestPtr3, DecryptedKey3, 0x400);
	
	// 0x13 = 19
	//           1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
	// 0040F453  60 68 F0 4F 06 00 6A 02 E8 1D 03 00 00 83 C4 08  `hðO..j.è.....Ä.
	//           17 18 19
	// 0040F463  61 EB 3A 68 00 50 03 00 E8 96 9C FF FF 83 C4 04  aë:h.P..è..ÿÿ.Ä.
	// 00409D4A | C705 55F44000 F04F0600 | mov dword ptr ds : [40F455] , 64FF0 |

	DWORD dst = ((DWORD)hInstanceSecServ) + 0x2BE86;
	BYTE BLOCK_40F453[0x13] = { 0x60, 0x68, 0xF0, 0x4F, 0x06, 0x00, 0x6A, 0x02, 0xE8, 0x1D, 0x03, 0x00, 0x00, 0x83, 0xC4, 0x08, 0x61, 0xEB, 0x3A };
	DWORD* DWORD_40F455 = (DWORD*)&BLOCK_40F453[2];
	*DWORD_40F455 += ((DWORD)hInstanceSecServ);

	logc(FOREGROUND_BLUE, "Writing to %08X (size 0x13)\n", dst);
	UnProtect_memcpy((void*)dst, BLOCK_40F453, 0x13);

	logc(FOREGROUND_GREEN, "Decrypt23Function Complete\n");

	Sleep(10);

	//GetKey(true);

	return MentalMemory;
}

void DecryptTEAKey(BYTE* Block4105C0, BYTE* Block4105E0)
{
	LogKey("DecryptTEAKey - Block4105C0 Before", (DWORD)Block4105C0, 0x10);

	DWORD edi = (DWORD)Block4105C0;
	DWORD eax = 0x56AC42FE;
	*(DWORD*)(edi) ^= eax;
	*(DWORD*)(edi + 0x4) ^= eax;
	*(DWORD*)(edi + 0x8) ^= eax;
	*(DWORD*)(edi + 0xC) ^= eax;

	DWORD esi = (DWORD)Block4105E0;

	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi) ^= eax;
	*(DWORD*)(edi + 0x8) ^= eax;

	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi + 0x4) ^= eax;
	*(DWORD*)(edi + 0xC) ^= eax;

	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi) ^= eax;
	*(DWORD*)(edi + 0x8) ^= eax;

	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi + 0x4) ^= eax;
	*(DWORD*)(edi + 0xC) ^= eax;

	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi) ^= eax;
	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi + 0x4) ^= eax;
	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi + 0x8) ^= eax;
	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi + 0xC) ^= eax;

	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi) ^= eax;
	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi + 0x4) ^= eax;
	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi + 0x8) ^= eax;
	eax = *(DWORD*)(esi); esi += 4;
	*(DWORD*)(edi + 0xC) ^= eax;

	LogKey("DecryptTEAKey - Block4105C0 After", (DWORD)Block4105C0, 0x10);
}

void Decrypt21Function() // bOOls eYe code
{
	BYTE Block40F42C[0x27] = { 0xFF, 0x35, 0xF8, 0x44, 0x03, 0x00, 0x68, 0x00, 0x10, 0x00, 0x00, 0x68, 0x68, 0xF7, 0x02, 0x00, 
							   0xE8, 0x00, 0x00, 0x00, 0x00, 0x83, 0xC4, 0x0C, 0x68, 0x20, 0x45, 0x03, 0x00, 0xE8, 0x60, 0x90,
							   0xFF, 0xFF, 0x83, 0xC4, 0x04, 0xC3, 0x90 };

	
	DWORD *DWORD_40F42E = (DWORD*)&Block40F42C[0x2];
	DWORD* DWORD_40F445 = (DWORD*)&Block40F42C[0x19];
	DWORD* DWORD_40F438 = (DWORD*)&Block40F42C[0xc];

	*DWORD_40F42E += ((DWORD)hInstanceAuthServ);
	*DWORD_40F445 += ((DWORD)hInstanceAuthServ);
	*DWORD_40F438 += ((DWORD)hInstanceAuthServ);

	DWORD ebx = ((DWORD)hInstanceAuthServ) + 0xF7FE + 0x15;
	DWORD ecx = ((DWORD)hInstanceSecServ) + 0x4D02;
	ecx -= ebx;
	DWORD* DWORD_40F43D = (DWORD*)&Block40F42C[0x11];
	*DWORD_40F43D = ecx;

	BYTE SavedBytes[0x27];
	DWORD dst27Bytes = ((DWORD)hInstanceAuthServ) + 0xF7FE;
	logc(FOREGROUND_BLUE, "Writing to %08X (size 0x27)\n", dst27Bytes);
	memcpy(SavedBytes, (void*)dst27Bytes, 0x27);
	UnProtect_memcpy((void*)dst27Bytes, Block40F42C, 0x27);

	void(__stdcall * decrypt)() = (void(__stdcall*)())(dst27Bytes);
	logc(FOREGROUND_BLUE, "Calling decrypt function at %08X\n", dst27Bytes);
	//GetKey(true);
	decrypt();

	BYTE* Block4105E0 = new BYTE[0x100];
	memcpy(Block4105E0, (void*)(*DWORD_40F445), 0x100);
	LogKey("Block4105E0", (DWORD)Block4105E0, 0x30);

	DWORD DWORD_419BEC;
	memcpy(&DWORD_419BEC, (void*)(((DWORD)hInstanceSecServ) + 0x4C56C), 4);

	BYTE* Block4105C0 = new BYTE[0x100];
	memcpy(Block4105C0, (void*)DWORD_419BEC, 0x10);

	DecryptTEAKey(Block4105C0, Block4105E0);

	UnProtect_memcpy((void*)DWORD_419BEC, Block4105C0, 0x10);

	memset(SavedBytes, 0x90, 0x27);
	UnProtect_memcpy((void*)dst27Bytes, SavedBytes, 0x27);

	BYTE JMPPatch[0x2] = { 0xEB, 0x08 };
	UnProtect_memcpy((void*)(((DWORD)hInstanceAuthServ) + 0xF7FE + (0x27 - 2)), JMPPatch, 0x2);
	
	// UnProtect_memcpy((void*)dst27Bytes, SavedBytes, 0x27);

	//BYTE JMPPatch[6] = { 0xEB, 0x2D, 0x90, 0x90, 0x90, 0x90 };
	//UnProtect_memcpy((void*)(((DWORD)hInstanceAuthServ) + 0xF7FE), JMPPatch, 6);

	logc(FOREGROUND_GREEN, "Decrypt21Function Complete\n");
	Sleep(10);

	//GetKey(true);
}

void Decrypt20Function() // bOOls eYe code
{
	BYTE Block40F410[0xF] = { 0x68, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x29, 0x95, 0xFF, 0xFF, 0x83, 0xC4, 0x04, 0xC3, 0x90 };

	DWORD eax = ((DWORD)hInstanceAuthServ) + 0x2D680;
	DWORD ebx = ((DWORD)hInstanceAuthServ) + 0xE80D;
	memcpy(&Block40F410[1], &eax, 4);

	UnProtect_memcpy((void*)ebx, Block40F410, 0xF);

	void(__stdcall * decrypt)() = (void(__stdcall*)())(ebx);
	logc(FOREGROUND_BLUE, "Calling decrypt function at %08X\n", ebx);
	//GetKey(true);
	decrypt();

	ebx = ((DWORD)hInstanceAuthServ) + 0x2D660;

	BYTE *Block4105C0 = new BYTE[0x100];
	memcpy(Block4105C0, (void*)ebx, 0x100);

	BYTE *Block4105E0 = &Block4105C0[0x20];

	DecryptTEAKey(Block4105C0, Block4105E0);

	UnProtect_memcpy((void*)(((DWORD)hInstanceSecServ) + 0x500F0), Block4105C0, 0x10);

	ebx = ((DWORD)hInstanceAuthServ) + 0xE80D;
	ebx += (0xF - 2);
	BYTE FinalPatch[2] = { 0x85, 0xC0 };
	UnProtect_memcpy((void*)ebx, FinalPatch, 0x2);

	logc(FOREGROUND_GREEN, "Decrypt20Function Complete\n");
	Sleep(10);

	//GetKey(true);
}