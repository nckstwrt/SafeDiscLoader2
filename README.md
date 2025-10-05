# SafeDiscLoader2
Allows playing SafeDisc (version 2.0 to the latest 4.9) protected games on modern Windows without the need of the CD/DVD. 

## Download
[Releases](https://github.com/nckstwrt/SafeDiscLoader2/releases)

## Usage (For SafeDisc version 3 or higher)
SafeDiscLoader2 is primarily just "version.dll". For any game using SafeDisc 3 or higher you can just place version.dll in the same directory as the game's main executable and the game can then be run normally. It will then automatically use version.dll without any further changes required to bypass SafeDisc.

## Usage (For SafeDisc version 2)
Old games do not automatically load version.dll - so it needs to be injected first using VersionInjector.exe

### Just Double-Click VersionInjector.exe
From here a window will appear allowing you to select your game's SafeDisc .exe file
### Drag your SafeDisc exe onto VersionInjector.exe
That is equivalent to running VersionInjector.exe with the dragged SafeDisc .exe file as the first argument. VersionInjector will attempt to pass on any additional arguments provided. i.e. `Sims2.exe -w` will load The Sims 2 in windowed mode
### Create a SafeDiscLoader.ini file
If a SafeDiscLoader.ini file is found in the same directory as SafeDiscLoader.exe it will use its contents as if it were commandline arguments passed. i.e. Create a text file called SafeDiscLoader.ini with just `Sims2.exe -w` to load The Sims 2
### Using it with non-SafeDisc executables
You can also use VersionInjector.exe to make SafeDisc utilities work on modern Windows. e.g. Safedisc2Cleaner which can unwrap Safedisc executables below version 2.7.

## Configuration
SafeDiscLoader2 can be configured to bypass additional CD checks and log lots of debug information.  
Find out more here: [Example Config Files](https://github.com/nckstwrt/SafeDiscLoader2/tree/main/Example%20Config%20Files)

## Credits
*  Reloaded for their Universal SafeDisc Loader which had great logging to help me reverse engineer what it was doing
*  SafeDiscCleaner (bOOls eYe, r!sc, etc) nice clean assembly to read for how to patch SafeDisc v2.0 - v2.6
*  [RibShark](https://twitter.com/RibShark) for his secdrv.sys [SafeDiscShim emulation code](https://github.com/RibShark/SafeDiscShim)
*  [version-proxy](https://github.com/BitCrackers/version-proxy) from BitCrackers for proxying the Windows version.dll code and approach
*  [MinHooks](https://github.com/TsudaKageyu/minhook) for API Hooking
*  SafeDisc 2.7 + 2.8 Loader code written by me

## Background
[My initial version](https://github.com/nckstwrt/SafeDiscLoader) used Reloaded's SDLoader.dll to bypass SafeDisc. Sadly this was picked up by many virus checkers as being suspicious purely because reloaded's code is so obfuscated. So this version is a fully reverse engineered version of SDLoader.dll and SafeDiscCleaner to support all versions of SafeDisc 2.0 and higher.

## Tested Games
* Call of Duty (Original): 3.10.20
* Call of Duty (Patched - 1.4): 3.15.11 
* Call of Duty - United Offensive: 3.20.22
* Call of Duty - Modern Warfare: 4.81.00
* The Sims 2: 3.20.20 (has two safedisc cd checks!)
* Call of Duty - World at War: 4.90.10
* Need for Speed: Underground: 2.90.40 (Needs VersionInjector)
* Football Manager 2005 (Original - 5.0): 3.20.22
* Football Manager 2005 (Patched - 5.0.5): 4.00.02
* Harry Potter and the Prisoner of Azkaban: 3.20.22
* Harry Potter and the Chamber of Secrets: 2.80.10 (Needs VersionInjector)
* The Battle for Middle-earth II: 4.60.00 ? Normal version string missing. Need to use VersionInjector on the Launcher.
* The Battle for Middle-earth: 4.00.01 ? Normal version string missing. Need to use VersionInjector on the Launcher. (On Win 11 needs special Options.ini to work)
* Football Manager 2006 (6.0.2): 4.60.00
* Football Manager 2006 (6.2.3): 4.60.00
* Football Manager 2007 (7.0.2): 4.60.00
* Need For Speed Underground 2: 3.05.10 Normal version string missing. Still needs CD 2 (but not a Safedisc check) - looks for bin.dat from the disc
* Command & Conquer: Renegade: 2.51.21 (Needs VersionInjector - still crashes due to getting system info. Issue with game on modern systems)
* Operation Flashpoint: Cold War Crisis: 2.40.10 (original) 1.46 (2.50.20) - Even it's patchers need safedisc (and therefore VersionInjector) to work.
* Doom 3: 3.20.22
* Far Cry: 3.15.10
* Max Payne (1.0): 2.30.33
* Settlers IV: 2.10.30
* Delta Force - Land Warrior: 2.05.30 (cd check for volume name and CDFS filesystem)
* Eurofighter Typhoon: 2.10.30
* Battlefield 1942: 2.60.52 - checks for the existence of bfdist.vlu on the CDROM
* Kohan: 2.60.52 - Checks the CDROM's Volumename is KOHAN_IS
* Battlefield 2: 4.00.01

 ## Notices
> [!IMPORTANT]
> This is not intended for Piracy but for users to exercise their Fair Use rights for the games they actually own and can no longer play due to draconian and unsupported copy protection

> [!CAUTION]
> Antivirus software that blocks code injection could block this dll or loader, so make sure to greenlist the included exe and dll or disable your antivirus software if required.
