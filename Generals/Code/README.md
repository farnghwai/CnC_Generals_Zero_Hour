# Objective
Able to compile using Visual Studio 2022

# Progress Overview
Completion: 44% (12 / 27)

## Work-in-Progress
Project | Path | Status | Remarks
--------|------|--------|---------
buildVersionUpdate | Tools\buildVersionUpdate\buildVersionUpdate.vcxproj | Done 
Compress | Tools\Compress\Compress.vcxproj | Done
Compression | Libraries\Source\Compression\Compression.vcxproj | Done | See note 1
DatGen | Tools\Launcher\DatGen\DatGen.vcxproj | Done* | See note 2
DebugWindow | doneTools\DebugWindow\DebugWindow.vcxproj | Done
EABrowserDispatch | doneLibraries\Source\EABrowserDispatch\EABrowserDispatch.vcxproj | Done
GameEngine | GameEngine\GameEngine.vcxproj | Not Started
GameEngineDevice | GameEngineDevice\GameEngineDevice.vcxproj | Not Started
GUIEdit | Tools\GUIEdit\GUIEdit.vcxproj | Not Started
ImagePacker | Tools\ImagePacker\ImagePacker.vcxproj | Not Started
launcher | Tools\Launcher\launcher.vcxproj | Not Started
MapCacheBuilder | Tools\MapCacheBuilder\MapCacheBuilder.vcxproj | Not Started
max2w3d | Tools\WW3D\max2w3d\max2w3d.vcxproj | Not Started
ParticleEditor | Tools\ParticleEditor\ParticleEditor.vcxproj | Not Started
pluglib | Tools\WW3D\pluglib\pluglib.vcxproj | Not Started
textureCompress | Tools\textureCompress\textureCompress.vcxproj | Not Started
versionUpdate | Tools\versionUpdate\versionUpdate.vcxproj | Done
WorldBuilder | Tools\WorldBuilder\WorldBuilder.vcxproj | Not Started
ww3d2 | Libraries\Source\WWVegas\WW3D2\ww3d2.vcxproj | Not Started
wwdebug | Libraries\Source\WWVegas\WWDebug\wwdebug.vcxproj | Done
WWDownload | Libraries\Source\WWVegas\WWDownload\WWDownload.vcxproj | Done
wwlib | Libraries\Source\WWVegas\WWLib\wwlib.vcxproj | Not Started
wwmath | ibraries\Source\WWVegas\WWMath\wwmath.vcxproj | Done* | See note 3
wwsaveload | Libraries\Source\WWVegas\WWSaveLoad\wwsaveload.vcxproj | Done*
wwutil | Libraries\Source\WWVegas\Wwutil\wwutil.vcxproj | Done*
RTS | RTS.vcxproj | Not Started
Solution | RTS.sln | Not Started

## Excluded
Project | Path |
--------|------|
Benchmark | LIBRARIES\SOURCE\Benchmark\Benchmark.dsp
GameSpyHTTP | Libraries\Source\GameSpy\GameSpy\ghttp\GameSpyHTTP.dsp
GameSpyPatching | Libraries\Source\GameSpy\GameSpy\pt\GameSpyPatching.dsp
GameSpyPeer | Libraries\Source\GameSpy\GameSpy\peer\GameSpyPeer.dsp
GameSpyPresence | Libraries\Source\GameSpy\GameSpy\gp\GameSpyPresence.dsp
GameSpyStats | Libraries\Source\GameSpy\GameSpy\gstats\GameSpyStats.dsp

## Notes
1. Update Compression library 
- Remove LZHCompress dependency; 
- Use Zlib v1.3.1 library from https://github.com/madler/zlib
- Use Preprocessor _CRT_SECURE_NO_WARNINGS to support warning
2. Remove SafeDisk\CdaPfn.h control
3. Attempt retrofit original DirectX library to newer version of DirectX library