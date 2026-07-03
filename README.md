# RTSInputSystem

Mass branch: this repository is the integrated RTS input/control system for Mass Battle Frame. It combines the former `OpenRTSCamera` camera/selection plugin and the former `RTSCommandSystem` command-grid runtime into one repository.

Repository name: `RTSInputSystem`

Current Unreal plugin module name: `OpenRTSCamera`

The module name is intentionally kept as `OpenRTSCamera` for now so existing assets, Blueprint references, and `/Script/OpenRTSCamera` paths keep loading. Renaming the C++ module should be a separate migration with explicit CoreRedirects and asset validation.

## Mass Battle Frame Integration

Core plugin role:

- `RTSInputSystem` owns camera movement, camera bounds, selection, command dispatch, and the 3x5 command panel runtime.
- `MassBattleFrame` remains the source of Mass agent data and movement/behavior execution.
- `FogOfWar` owns shared map bounds export through `Config/FogOfWarMapBounds.ini`.

The camera does not depend on `FogOfWar` as a C++ module. Instead it reads the same map-bounds INI protocol:

```ini
[MapBounds.MapName]
OriginX=0
OriginY=0
SizeX=409600
SizeY=409600
MapOverflowUU=0

[MapBounds.Default]
OriginX=0
OriginY=0
SizeX=409600
SizeY=409600
```

At runtime `URTSCamera` resolves bounds in this order:

1. `Config/FogOfWarMapBounds.ini`, section `MapBounds.<current map name>`.
2. `Config/FogOfWarMapBounds.ini`, section `MapBounds.Default`.
3. `ARTSCameraBoundsVolume`.
4. Any actor tagged `OpenRTSCamera#CameraBounds`.

This keeps the camera indirectly synchronized with the minimap/FogOfWar bounds without linking to minimap actor classes. The INI is currently a project config file with per-map sections; it is not embedded inside each `.umap`.

## Command System Integration

The previous `RTSCommandSystem` runtime classes are now inside this repository:

- `URTSCommandSubsystem`
- `URTSCommandButton`
- `URTSBuiltinCommandButton`
- `URTSCommandGridAsset`
- `URTSUnitCommandGrid`
- `URTSCityCommandGrid`
- `IRTSCommandInterface`

`Config/DefaultEngine.ini` contains CoreRedirects from `/Script/RTSCommandSystem` to `/Script/OpenRTSCamera` so existing command assets can migrate.

Default Mass unit commands are registered as native gameplay tags:

- `RTS.Command.Move`
- `RTS.Command.Attack`
- `RTS.Command.Stop`
- `RTS.Command.Hold`
- `RTS.Command.Patrol`

Actor-backed selections can still provide custom command grids through `IRTSCommandInterface`. Pure Mass selections fall back to the built-in unit command grid and dispatch through `URTSSelectionSubsystem`.

## Required Plugins

For the Mass branch, enable:

- `OpenRTSCamera`
- `MassBattle`
- `MassGameplay`
- `EnhancedInput`

Do not enable the old `MassBattleMinimap`, `LandmarkSystem`, or standalone `RTSCommandSystem` as dependencies for this integrated camera path.

- [Installing from GitHub](https://github.com/HeyZoos/OpenRTSCamera/wiki/Installing-from-GitHub)
- [Getting Started](https://github.com/HeyZoos/OpenRTSCamera/wiki/Getting-Started)

## Features

- Smoothed Movement
- Ground Height Adaptation
- Edge Scrolling
- [Follow Target](https://github.com/HeyZoos/OpenRTSCamera/wiki/Follow-Camera)
- [Mouse + Keyboard Controls](https://github.com/HeyZoos/OpenRTSCamera/wiki/Movement-Controls)
- [Gamepad Controls](https://github.com/HeyZoos/OpenRTSCamera/wiki/Movement-Controls)
- [Unit Selection](https://github.com/HeyZoos/OpenRTSCamera/wiki/Unit-Selection)

### [Camera Bounds](https://github.com/HeyZoos/OpenRTSCamera/wiki/Camera-Bounds)

https://user-images.githubusercontent.com/9408481/223589311-9d6b1cfd-76b4-4650-a6a3-0386a483bb96.mp4

### [Jump To](https://github.com/HeyZoos/OpenRTSCamera/wiki/Jump-To)

https://user-images.githubusercontent.com/9408481/223585144-d7e9c1c2-2e36-4628-9bbd-da91229e39e1.mp4

# Changelog

### 0.21.0

- Add [Unit Selection](https://github.com/HeyZoos/OpenRTSCamera/wiki/Unit-Selection)

### 0.20.0

- Build for Unreal Engine v5.3.X

### 0.19.0

- Add "Jump To" method
- Start overhauling documentation

### 0.18.0

- Fix crash when using the camera in a networked context
- **No longer take over view target automatically, the view target must be explicitly set, this is documented in the "Getting Started" section of the wiki**

### 0.17.0

- Fix [#27](https://github.com/HeyZoos/OpenRTSCamera/issues/27) by tying camera movement to delta time (thanks [@theMyll](https://github.com/theMyll))
- **This will result in slower movement across the board, if you notice your camera moving more slowly, up the speed values by about 100x. For example, the new camera blueprint speed defaults are 5000**
