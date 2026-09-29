This git repo includes the source code for _Thick as Thieves_, but not the content. The content can found on the [Releases Page](https://github.com/othersideentertainment/thick-as-thieves/releases).

The open source version of _Thick as Thieves_ is not completely identical to the Steam version. See [ThirdPartyRemovals.md]([ThirdPartyRemovals.md) for some details.
## Engine Setup
### Unreal Prerequisites
Building Unreal 5.5.4 from source requires:
1. Visual Studio 2022 17.8 or later, 17.10 recommended (Default)
2. Windows SDK 10.0.19041.0 or newer
See https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.5
### Patching the engine
Thick as Thieves uses a modified version of Unreal 5.5.4, so you will need to apply the  `0001-OtherSide-Thick-as-Thieves-Engine-Changes.patch` patch to that version of the source code

(All snippets assume powershell, but would be roughly similar in cmd)
```
git clone --depth 1 --branch 5.5.4-release https://github.com/EpicGames/UnrealEngine.git
cd UnrealEngine
git apply 0001-OtherSide-Thick-as-Thieves-Engine-Changes.patch --ignore-whitespace

reg add "HKCU\Software\Epic Games\Unreal Engine\Builds" ^
  /v "UE_554_SRC" ^
  /t REG_SZ ^
  /d "C:\UnrealEngine" ^
  /f
```
* Run `Setup.bat` in the UnrealEngine folder
### Building the Editor and Game

NOTE: There are several ways to organize and build an unreal project using an engine built from source. This is just one of them. Other ways can also work fine as long as it is with the custom patched version of 5.5.4. Once the engine is setup, you can also use the right click extension on `TAT.uproject`
* Change TAT.uproject EngineAssociation to UE_554_SRC 
```
$UEROOT="D:\prj\UnrealEngine"
$TATROOT="C:\prj\tat_oss"
&$UEROOT\Engine\Build\BatchFiles\Build.bat -Target="TATEditor Win64 Development" -Project="$TATROOT\TAT\TAT.uproject" -FromMSBuild -NoAdaptiveUnity -MaxParallelActions=8

&$UEROOT\Engine\Build\BatchFiles\RunUAT BuildCookRun -buildmachine -nop4 -project="$TATROOT\TAT\TAT.uproject" -noclient -server -servertargetplatform=Win64 -serverconfig=Development -utf8output -build -cook -pak -stage -package -nocodesign -archive -archivedirectory="C:\prj\tat_oss\LocalBuilds" -crashreporter -incremental -MaxParallelActions=8

&"$UEROOT\Engine\Build\BatchFiles\RunUAT" BuildCookRun -buildmachine -nop4 -project="$TATROOT\TAT\TAT.uproject" -target=TAT -game -server -targetplatform=Win64 -clientconfig=Development -serverconfig=Development -utf8output -build -cook -pak -stage -package -nocodesign -archive -archivedirectory="$TATROOT\LocalBuilds" -crashreporter -incremental -MaxParallelActions=8
```
## Test Commands
```
$UEROOT="D:\prj\UnrealEngine"
$TATROOT="D:\prj\testbuild"
# Update this with the real repository URL
$TATREPO="D:\prj\tat_oss_nosrc"

git clone --depth 1 $TATREPO $TATROOT
git clone --depth 1 --branch 5.5.4-release https://github.com/EpicGames/UnrealEngine.git $UEROOT
pushd $UEROOT
git apply $TATROOT/0001-OtherSide-Thick-as-Thieves-Engine-Changes.patch


pushd $TATROOT
# You can build initially without any of the binary content
# Wwise will be in null audio mode if you do that
&$UEROOT\Engine\Build\BatchFiles\Build.bat -Target="TATEditor Win64 Development" -Project="$TATROOT\TAT\TAT.uproject" -FromMSBuild -NoAdaptiveUnity -MaxParallelActions=8

&"C:\Program Files\7-Zip\7z.exe" x Binaries.7z.001 -o"$TATROOT"
"$UEROOT\Engine\Build\BatchFiles\RunUAT" BuildCookRun -buildmachine -nop4 -project="$TATROOT\TAT\TAT.uproject" -target=TAT -game -server -targetplatform=Win64 -clientconfig=Development -serverconfig=Development -utf8output -build -cook -pak -stage -package -nocodesign -archive -archivedirectory="$TATROOT\LocalBuilds" -crashreporter -incremental -MaxParallelActions=8
&"$UEROOT\Engine\Build\BatchFiles\RunUAT" BuildCookRun -buildmachine -nop4 -project="$TATROOT\TAT\TAT.uproject" -target=TAT  -targetplatform=Win64 -utf8output -build -cook -pak -stage -package -crashreporter -nocodesign -iostore -archive -archivedirectory="$TATROOT\LocalBuilds" -clientconfig=Shipping -serverconfig=Shipping -MaxParallelActions=8
```

## Content Bundle Editing

Download the multipart 7zip archives.

```
$ARCHIVENAME="Binaries"
$DOWNDIR="D:\tatdownloads"
$EDITDIR="D:\tatcurrentbin"
$NEWDIR="D:\tatnewarchive"

mkdir "$EDITDIR"
&"C:\Program Files\7-Zip\7z.exe" x "$DOWNDIR\$ARCHIVENAME.7z.001" -o"$EDITDIR"
echo "Delete the files you don't want from $EDITDIR"
pause
mkdir "$NEWDIR"
pushd "$EDITDIR"
&"C:\Program Files\7-Zip\7z.exe" a -v1800m -mmt=on -spf "$NEWDIR\$ARCHIVENAME.7z" .
```

## Wwise Audio

2024.1.4.8780 (2024.1.4.8780.3614 for unreal integration)

https://www.audiokinetic.com/

Go to the Audiokinetic Website to download the Audiokinetic Launcher:
https://www.audiokinetic.com/en/download/ 

Once downloaded you can begin installing the current version of wwise under the “Wwise”.

Any artifact that includes the Wwise components may *only* be distributed after acquiring a valid license from Audiokinetic.

# Licensing

Source code provided by OtherSide Entertainment is licensed under the MIT license. Assets provided by OtherSide Entertainment are licensed under the CC BY 4.0 license.

Game trademarks, and the OtherSide Entertainment name/logo are not licensed and remain (c) 2026 OtherSide Entertainment, Inc. All rights reserved.

These licenses apply solely to the original source code and assets contained in this repository. It does not grant any rights to Unreal Engine or other Epic Games software. Use of Unreal Engine is governed exclusively by Epic Games' End User License Agreement (https://www.unrealengine.com/eula).

Other portions of this project are licensed under compatible terms, but are not re-licensed under the MIT license. These portions of the project are noted below. 


## Unreal Engine Requirements

This project requires Unreal Engine (version 5.5.4), developed by
Epic Games, Inc. Use of this project is subject to compliance with the
[Unreal Engine End User License Agreement](https://www.unrealengine.com/eula).

Unreal Engine is not included in this repository and must be obtained
separately from Epic Games.

## Unreal Starter Assets

Other assets licensed under the UE EULA

```
/Content/Art/Effects/LensEffects/Common/SM_Camera_Mesh
/Content/Art/Effects/LensEffects/Common/T_Simple_Gradients_M
/Content/Art/Effects/Monocular_Vision/T_Water_M
/Content/Art/Effects/Steady_State_HotWire/t_Water-Wire
/Content/Art/VFX/Textures/Tile/Organic/T_TilingNoise04
/Content/Art/VFX/Textures/Tile/Organic/T_TilingNoise11
/Content/Art/VFX/Textures/Tile/Organic/T_WaterFlow_01_Foam_Tiled_4Real
/Content/Test/Art/Primitives/SM_Shape_Cone
/Content/Test/Art/Primitives/SM_Shape_Pipe
/Content/Test/Art/Primitives/SM_Shape_Tube
```

## Lyra Starter Game

Some content from the Lyra Starter Game was used. It has been modified to add value by targetting new skeletons. These are smaller components of a larger set of components that do not constitute the primary focus of this project.

```
/Content/Art/Animation/Base/Anim/Lyra
/Content/Art/Animation/ThiefNisha/Anim/LyraFem
```

The use of these assets is covered by:

- UE-Only Content - Licensed for Use Only with Unreal Engine-based Products
- The Fab Standard License

Original content may be acquired at https://www.fab.com/listings/93faede1-4434-47c0-85f1-bf27c0820ad0 from the the Lyra Starter Game.

## Third Party Fonts

The following fonts are in use under compatible license terms, but are not being relicensed by this project.


### SIL Open Font License

```
/Content/Art/UI/Fonts/Abel-Regular
/Content/Art/UI/Fonts/Abel-Regular_Font
/Content/Art/UI/Fonts/AdventPro-ExtraLight
/Content/Art/UI/Fonts/AdventPro-ExtraLight_Font
/Content/Art/UI/Fonts/AdventPro-Light
/Content/Art/UI/Fonts/AdventPro-Light_Font
/Content/Art/UI/Fonts/AdventPro-Thin
/Content/Art/UI/Fonts/AdventPro-Thin_Font
/Content/Art/UI/Fonts/Alata-Regular
/Content/Art/UI/Fonts/Alata-Regular_Font
/Content/Art/UI/Fonts/Balthazar-Regular
/Content/Art/UI/Fonts/Balthazar-Regular_Font
/Content/Art/UI/Fonts/Caveat-Bold
/Content/Art/UI/Fonts/Caveat-Bold_Font
/Content/Art/UI/Fonts/Caveat-Medium
/Content/Art/UI/Fonts/Caveat-Medium_Font
/Content/Art/UI/Fonts/Caveat-Regular
/Content/Art/UI/Fonts/Caveat-Regular_Font
/Content/Art/UI/Fonts/Caveat-SemiBold
/Content/Art/UI/Fonts/Caveat-SemiBold_Font
/Content/Art/UI/Fonts/CF_Balthazar
/Content/Art/UI/Fonts/CF_Caveat
/Content/Art/UI/Fonts/CF_FacultyGlyphic
/Content/Art/UI/Fonts/CF_Raleway
/Content/Art/UI/Fonts/CrimsonText-Bold
/Content/Art/UI/Fonts/CrimsonText-Bold_Font
/Content/Art/UI/Fonts/CrimsonText-Regular
/Content/Art/UI/Fonts/CrimsonText-Regular_Font
/Content/Art/UI/Fonts/FacultyGlyphic-Regular
/Content/Art/UI/Fonts/FacultyGlyphic-Regular_Font
/Content/Art/UI/Fonts/Faustina-Bold
/Content/Art/UI/Fonts/Faustina-Bold_Font
/Content/Art/UI/Fonts/Faustina-BoldItalic
/Content/Art/UI/Fonts/Faustina-BoldItalic_Font
/Content/Art/UI/Fonts/Faustina-ExtraBold
/Content/Art/UI/Fonts/Faustina-ExtraBold_Font
/Content/Art/UI/Fonts/Faustina-ExtraBoldItalic
/Content/Art/UI/Fonts/Faustina-ExtraBoldItalic_Font
/Content/Art/UI/Fonts/Faustina-Italic
/Content/Art/UI/Fonts/Faustina-Italic_Font
/Content/Art/UI/Fonts/Faustina-Light
/Content/Art/UI/Fonts/Faustina-Light_Font
/Content/Art/UI/Fonts/Faustina-LightItalic
/Content/Art/UI/Fonts/Faustina-LightItalic_Font
/Content/Art/UI/Fonts/Faustina-Medium
/Content/Art/UI/Fonts/Faustina-Medium_Font
/Content/Art/UI/Fonts/Faustina-MediumItalic
/Content/Art/UI/Fonts/Faustina-MediumItalic_Font
/Content/Art/UI/Fonts/Faustina-Regular
/Content/Art/UI/Fonts/Faustina-Regular_Font
/Content/Art/UI/Fonts/Faustina-SemiBold
/Content/Art/UI/Fonts/Faustina-SemiBold_Font
/Content/Art/UI/Fonts/Faustina-SemiBoldItalic
/Content/Art/UI/Fonts/Faustina-SemiBoldItalic_Font
/Content/Art/UI/Fonts/JosefinSlab-Bold
/Content/Art/UI/Fonts/JosefinSlab-Bold_Font
/Content/Art/UI/Fonts/JosefinSlab-BoldItalic
/Content/Art/UI/Fonts/JosefinSlab-BoldItalic_Font
/Content/Art/UI/Fonts/JosefinSlab-ExtraLight
/Content/Art/UI/Fonts/JosefinSlab-ExtraLight_Font
/Content/Art/UI/Fonts/JosefinSlab-ExtraLightItalic
/Content/Art/UI/Fonts/JosefinSlab-ExtraLightItalic_Font
/Content/Art/UI/Fonts/JosefinSlab-Italic
/Content/Art/UI/Fonts/JosefinSlab-Italic_Font
/Content/Art/UI/Fonts/JosefinSlab-Light
/Content/Art/UI/Fonts/JosefinSlab-Light_Font
/Content/Art/UI/Fonts/JosefinSlab-LightItalic
/Content/Art/UI/Fonts/JosefinSlab-LightItalic_Font
/Content/Art/UI/Fonts/JosefinSlab-ThinItalic
/Content/Art/UI/Fonts/JosefinSlab-ThinItalic_Font
/Content/Art/UI/Fonts/Platypi-Bold
/Content/Art/UI/Fonts/Raleway-Black
/Content/Art/UI/Fonts/Raleway-Black_Font
/Content/Art/UI/Fonts/Raleway-BlackItalic
/Content/Art/UI/Fonts/Raleway-BlackItalic_Font
/Content/Art/UI/Fonts/Raleway-Bold
/Content/Art/UI/Fonts/Raleway-Bold_Font
/Content/Art/UI/Fonts/Raleway-BoldItalic
/Content/Art/UI/Fonts/Raleway-BoldItalic_Font
/Content/Art/UI/Fonts/Raleway-ExtraBold
/Content/Art/UI/Fonts/Raleway-ExtraBold_Font
/Content/Art/UI/Fonts/Raleway-ExtraBoldItalic
/Content/Art/UI/Fonts/Raleway-ExtraBoldItalic_Font
/Content/Art/UI/Fonts/Raleway-ExtraLight
/Content/Art/UI/Fonts/Raleway-ExtraLight_Font
/Content/Art/UI/Fonts/Raleway-ExtraLightItalic
/Content/Art/UI/Fonts/Raleway-ExtraLightItalic_Font
/Content/Art/UI/Fonts/Raleway-Italic
/Content/Art/UI/Fonts/Raleway-Italic_Font
/Content/Art/UI/Fonts/Raleway-Light
/Content/Art/UI/Fonts/Raleway-Light_Font
/Content/Art/UI/Fonts/Raleway-LightItalic
/Content/Art/UI/Fonts/Raleway-LightItalic_Font
/Content/Art/UI/Fonts/Raleway-Medium
/Content/Art/UI/Fonts/Raleway-Medium_Font
/Content/Art/UI/Fonts/Raleway-MediumItalic
/Content/Art/UI/Fonts/Raleway-MediumItalic_Font
/Content/Art/UI/Fonts/Raleway-Regular
/Content/Art/UI/Fonts/Raleway-Regular_Font
/Content/Art/UI/Fonts/Raleway-SemiBold
/Content/Art/UI/Fonts/Raleway-SemiBold_Font
/Content/Art/UI/Fonts/Raleway-SemiBoldItalic
/Content/Art/UI/Fonts/Raleway-SemiBoldItalic_Font
/Content/Art/UI/Fonts/Raleway-Thin
/Content/Art/UI/Fonts/Raleway-Thin_Font
/Content/Art/UI/Fonts/Raleway-ThinItalic
/Content/Art/UI/Fonts/Raleway-ThinItalic_Font
/Content/Art/UI/Fonts/Raleway-VariableFont_wght
/Content/Art/UI/Fonts/Raleway-VariableFont_wght_Font
/Content/Art/UI/Fonts/SourceSerifPro-Black
/Content/Art/UI/Fonts/SourceSerifPro-Black_Font
/Content/Art/UI/Fonts/SourceSerifPro-BlackIt
/Content/Art/UI/Fonts/SourceSerifPro-BlackIt_Font
/Content/Art/UI/Fonts/SourceSerifPro-Bold
/Content/Art/UI/Fonts/SourceSerifPro-Bold_Font
/Content/Art/UI/Fonts/SourceSerifPro-BoldIt
/Content/Art/UI/Fonts/SourceSerifPro-BoldIt_Font
/Content/Art/UI/Fonts/SourceSerifPro-ExtraLight
/Content/Art/UI/Fonts/SourceSerifPro-ExtraLight_Font
/Content/Art/UI/Fonts/SourceSerifPro-ExtraLightIt
/Content/Art/UI/Fonts/SourceSerifPro-ExtraLightIt_Font
/Content/Art/UI/Fonts/SourceSerifPro-It
/Content/Art/UI/Fonts/SourceSerifPro-It_Font
/Content/Art/UI/Fonts/SourceSerifPro-Light
/Content/Art/UI/Fonts/SourceSerifPro-Light_Font
/Content/Art/UI/Fonts/SourceSerifPro-LightIt
/Content/Art/UI/Fonts/SourceSerifPro-LightIt_Font
/Content/Art/UI/Fonts/SourceSerifPro-Regular
/Content/Art/UI/Fonts/SourceSerifPro-Regular_Font
/Content/Art/UI/Fonts/SourceSerifPro-Semibold
/Content/Art/UI/Fonts/SourceSerifPro-Semibold_Font
/Content/Art/UI/Fonts/SourceSerifPro-SemiboldIt
/Content/Art/UI/Fonts/SourceSerifPro-SemiboldIt_Font
```

### Apache v2

```
/Content/Art/UI/Fonts/CF_PermanentMarker
/Content/Art/UI/Fonts/CF_specialElite
/Content/Art/UI/Fonts/PermanentMarker-Regular
/Content/Art/UI/Fonts/PermanentMarker-Regular_Font
/Content/Art/UI/Fonts/SpecialElite-RegularSpecialElite-Regula
/Content/Art/UI/Fonts/SpecialElite-Regular_Font
```

### Free for any use 

Daniel Midgley

This font is free for anything you'd like to do with it, commercial or not. No need to ask -- just enjoy.

```
/Content/Art/UI/Fonts/Daniel-Bold
/Content/Art/UI/Fonts/Daniel-Bold_Font
```

## Third Party Software

Dear ImGui
Copyright (c) 2014-2026 Omar Cornut
MIT License.
https://github.com/ocornut/imgui

Dear ImGui for Unreal Engine
Copyright (c) 2023 Ves Georgiev
MIT License.
See third_party/imgui/LICENSE.txt for full text.
https://github.com/VesCodes/ImGui

GenericGraph
(c) 2016 jinyuliao
MIT License
ttps://github.com/jinyuliao/GenericGraph 

AUDIOKINETIC Wwise Technology.
2024.1.4.8780 (3614 for unreal integration)
https://www.audiokinetic.com/

mGear
Copyright (c) 2011-2018 Jeremie Passerin, Miquel Campos - 2018-2022 The mGear-Dev Organization
MIT License

## No Claim to Unauthorized Material

This project may inadvertently include source code, assets, or other
content that the maintainers did not have full rights or authorization
to license under the terms stated in this repository. The maintainers
make no claim of ownership or licensing authority over any such
material, and its inclusion does not constitute a representation that
it has been properly licensed.

If you are a rights holder and believe content in this repository has
been included without proper authorization, please contact
Lloyd.Plenty@aonic.co. Upon verification of such a claim,
the maintainers will promptly:

  (a) remove the material, or
  (b) update its license notice/attribution to reflect the correct
      terms, as appropriate.

This does not affect the validity of the license grant for the
remainder of the project's original content.

## Severability

If any provision of this license is held to be invalid or
unenforceable, such provision shall be struck and the remaining
provisions shall remain in full force and effect.

## Mixed Provenance

### TAT\Source\TAT\Private\AI\StateTrees\Tasks\TATStateTreeRunEnvQueryTask.cpp

Modified originally from Unreal Engine directly. Only original additions covered by the MIT license. Any usage must adhere to the UE EULA.

https://github.com/EpicGames/UnrealEngine/blob/585df42eb3a391efd295abd231333df20cddbcf3/Engine/Plugins/Runtime/StateTree/Source/StateTreeModule/Public/StateTreePropertyRef.h


### TAT\Source\TAT\Public\AI\StateTrees\Tasks\TATStateTreeRunEnvQueryTask.h

Modified originally from Unreal Engine directly. Only original additions covered by the MIT license. Any usage must adhere to the UE EULA.

https://github.com/EpicGames/UnrealEngine/blob/585df42eb3a391efd295abd231333df20cddbcf3/Engine/Plugins/Runtime/GameplayStateTree/Source/GameplayStateTreeModule/Public/Tasks/StateTreeRunEnvQueryTask.h

### OSE Perception Plugin

Only available under the UE EULA as it is a partial rewrite of the Unreal Engine Perception system rooted in that code. 

All OtherSide Entertainment original portions licensed under the MIT license.

### TAT\Plugins\OSEGenericGraph\Source\OSEGenericGraphEditor\Private\OSEGenericGraphNodeHandleCustomization.cpp

Copied WriteGuidToProperty function from Engine/Source/Editor/DetailCustomizations/Private/GuidStructCustomization.cpp

### TAT\Plugins\OSECore\Source\OSECore\Private\UI\Slate\SOSEAnimatedSwitcher.cpp

Adapted from CommonUI/Private/Slate/SCommonAnimatedSwitcher.cpp

### TAT\Plugins\OSECore\Source\OSECore\Public\UI\Slate\SOSEAnimatedSwitcher.h

Adapted from CommonUI/Public/Slate/SCommonAnimatedSwitcher.h

### TAT\Plugins\OSECore\Source\OSECore\Public\UI\OSEAnimatedSwitcher.h

Adapted from CommonUI/Public/CommonAnimatedSwitcher.h

### TAT\Plugins\OSECore\Source\OSECore\Private\UI\OSEAnimatedSwitcher.cpp

Adapted from CommonUI/Private/CommonAnimatedSwitcher.cpp

### UOSEAbilitiesGameplayEffectComponent

Fork of UAbilitiesGameplayEffectComponent which integrates custom abilities input and requirements.

TAT\Plugins\OSECore\Source\OSECore\Public\Abilities\Effects\OSEAbilitiesGameplayEffectComponent.h
TAT\Plugins\OSECore\Source\OSECore\Private\Abilities\Effects\OSEAbilitiesGameplayEffectComponent.cpp

Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Public\GameplayEffectComponents\AbilitiesGameplayEffectComponent.h
Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Private\GameplayEffectComponents\AbilitiesGameplayEffectComponent.cpp


### USkeletalMeshComponentOffsetToBone

Partial fork of USkeletalMeshComponent in order to change the way bounds are calculated.

TAT\Plugins\OSECore\Source\OSECore\Public\Graphics\Mesh\SkeletalMeshComponentOffsetToBone.h
TAT\Plugins\OSECore\Source\OSECore\Private\Graphics\Mesh\SkeletalMeshComponentOffsetToBone.cpp

Engine\Source\Runtime\Engine\Private\Components\SkeletalMeshComponent.cpp

### TAT\Source\TAT\Private\Audio\TATWwiseUtils.cpp

Adapts portions of the AUDIOKINETIC Wwise Technology game integration package.

All OtherSide Entertainment original portions licensed under the MIT license; however, in order to distribute anything based on this file one must acquire a valid license from Audiokinetic and adhere to the terms of the UE EULA.



