# RTS Input System

Mass branch: this repository is the integrated RTS Input System for Mass Battle Frame. It combines the former `RTSInputSystem` camera/selection plugin and the former `RTSCommandSystem` command-grid runtime into one repository.

Repository/folder name: `RTSInputSystem`

Product name: `RTS Input System`

Current Unreal plugin module name: `RTSInputSystem`

The module name is intentionally kept as `RTSInputSystem` for now so existing assets, Blueprint references, and `/Script/RTSInputSystem` paths keep loading. Renaming the C++ module should be a separate migration with explicit CoreRedirects and asset validation.

## Mass Battle Frame Integration

Core plugin role:

- `RTS Input System` owns camera movement, camera bounds, selection, command dispatch, and the 3x5 command panel runtime.
- `MassBattleFrame` remains the source of Mass agent data and movement/behavior execution.
- `Config/MapRegion/<MapName>/MapRegion.ini` is the shared minimap coordinate source.

The camera does not depend on `FogOfWar` as a C++ module. Instead it reads the same map region INI protocol:

```ini
[MapRegion]
OriginX=0
OriginY=0
SizeX=409600
SizeY=409600
MapOverflowUU=0
```

At runtime `URTSCamera` resolves bounds from:

- `Config/MapRegion/<current map name>/MapRegion.ini`, section `MapRegion`.

This keeps the camera synchronized with the minimap unit projection without carrying a separate RTS camera boundary actor.

## Command System Integration

The previous `RTSCommandSystem` runtime classes are now inside this repository:

- `URTSCommandSubsystem`
- `URTSCommandButton`
- `URTSBuiltinCommandButton`
- `URTSCommandGridAsset`
- `URTSUnitCommandGrid`
- `URTSCityCommandGrid`
- `IRTSCommandInterface`

`Config/DefaultEngine.ini` contains CoreRedirects from `/Script/RTSCommandSystem` to `/Script/RTSInputSystem` so existing command assets can migrate.

Default Mass unit commands are registered as native gameplay tags:

- `RTS.Command.Move`
- `RTS.Command.Attack`
- `RTS.Command.Stop`
- `RTS.Command.Hold`
- `RTS.Command.Patrol`

Actor-backed selections can still provide custom command grids through `IRTSCommandInterface`. Pure Mass selections fall back to the built-in unit command grid and dispatch through `URTSSelectionSubsystem`.

## Common command progress UI

Long-running commands share one StarCraft-style presentation model. Unit
training, building construction, technology research, national focuses, and
future timed commands should publish `FRTSCommandProgressItem` snapshots. The
unit panel renders every snapshot through the same icon, progress bar,
remaining-time, and queue-state widgets.

Command-grid buttons only place orders. They do not use button cooldowns to
imitate research or production progress. Actor-backed systems can implement
`IRTSCommandProgressProvider`; Mass-backed systems can populate
`FRTSUnitData::CommandProgressItems` while enriching selection data. Gameplay
systems remain responsible for execution and completion callbacks.

Progress icons remain full-color while active or queued. A cancellable item
provides an `IRTSCommandProgressController` action target; clicking its icon
requests cancellation by stable item id, leaving refunds, authority checks,
and queue promotion to the gameplay system.

## Required Plugins

For the Mass branch, enable:

- `RTS Input System` (`RTSInputSystem` technical plugin id)
- `MassBattle`
- `MassGameplay`
- `EnhancedInput`

Do not enable the old `MassBattleMinimap`, `LandmarkSystem`, or standalone `RTSCommandSystem` as dependencies for this integrated camera path.

- [Installing from GitHub](https://github.com/HeyZoos/RTSInputSystem/wiki/Installing-from-GitHub)
- [Getting Started](https://github.com/HeyZoos/RTSInputSystem/wiki/Getting-Started)

## Features

- Smoothed Movement
- Ground Height Adaptation
- Edge Scrolling
- [Follow Target](https://github.com/HeyZoos/RTSInputSystem/wiki/Follow-Camera)
- [Mouse + Keyboard Controls](https://github.com/HeyZoos/RTSInputSystem/wiki/Movement-Controls)
- [Gamepad Controls](https://github.com/HeyZoos/RTSInputSystem/wiki/Movement-Controls)
- [Unit Selection](https://github.com/HeyZoos/RTSInputSystem/wiki/Unit-Selection)

## Mass-first control groups and quick selection

The selection subsystem owns ten session-persistent player control groups. Mass
entities are stored by `FEntityHandle` (index + serial), invalid handles are
pruned before recall/UI refresh, and Actor-backed units remain a compatibility
path only.

Default controls follow the familiar RTS convention:

- `Ctrl + 0-9`: replace a control group with the current selection.
- `Shift + 0-9`: add the current selection, or remove it when every selected
  unit is already in the group.
- `Alt + 0-9`: remove the selected units from other groups and replace the
  target group.
- `0-9`: recall a group without clearing the current selection when that group
  is empty.
- Double-tap `0-9`: recall and center the RTS camera on a sampled real unit in
  the group's dominant dense cluster, avoiding empty midpoints between split forces.

`/Game/UI/HeadUpDisplay/UnitDetails/UnitFormationList` is the bottom
`UnitDetailPanel` control-group strip. It displays assigned slots in `1-9, 0`
order as readable `128x64` cards, compacted into the same eight-column rhythm as
the unit grid. Empty slots collapse; groups 9 and 0 wrap to a second row when all
ten are assigned. Cards show representative type, total count, active state, and
a per-type composition tooltip. It is intentionally independent from `FRTSSelectionView.ActiveGroupKey`,
which remains the Tab-cycled unit-type subgroup of the current selection.

The existing `/Game/UI/HeadUpDisplay/TopSelect` category buttons are bound at
runtime by `URTSSelector` to hierarchical `RTS.Selection.*` queries. Mass type
protocols can author `SelectionTags`; when omitted, the runtime derives sensible
categories from `TypeKey` and `Role`. Idle selection reads MassBattle's `Idle`
entity flag. All quick-selection controls are Mass-only by default; Actor lookup
remains an opt-in compatibility path through `bIncludeActorUnits`.

UMG exposes three directly placeable native button classes:

- `RTS Selection Query Button`: configure any category and idle-only query.
- `RTS Select All Army Button`: ready-made all-army shortcut.
- `RTS Select All Idle Army Button`: ready-made all-idle-army shortcut.

`RTS Control Group Button` is also directly placeable for custom group layouts.
It mirrors the keyboard modifier behavior and exposes the full composition
snapshot to Blueprint through `OnControlGroupStateChanged`.

## Large-selection feedback architecture

Visual feedback is event-bounded and independent of army size:

- Task feedback never enumerates selected Actors or Mass entities and never
  reads a Mass fragment. One player command produces one straight route line;
  Shift-queued destinations form a capped chain of at most 32 segments. Move
  lines are green and attack/F2A lines are red.
- The route origin is cached by one screen-to-world trace when the selection
  gesture ends. A programmatic selection without an event-level origin does not
  draw a route instead of scanning units to reconstruct one.
- Right-click ground feedback is a short expanding line pulse, capped at eight
  simultaneous 32-segment rings.
- Exact per-unit attack/buff range outlines are disabled until a renderer-native
  Mass/GPU batch source is available. The input layer must not build them with a
  CPU unit scan.

Runtime profiling controls:

```text
stat RTSInputFeedback
RTS.SelectionFeedback.TaskLines 0
RTS.CommandFeedback.GroundPulse 0
```

Each enabled visual is capped by command count, not selected-unit count.

## City construction placement

The built-in city command card uses the following sparse 3x5 layout:

| Slot | Command | Targeting |
| --- | --- | --- |
| Row 1, Col 1 | Officer | Instant recruitment |
| Row 1, Col 2 | Militia | Instant recruitment |
| Row 1, Col 3 | Establish Capital | Place the capital base on the build grid |
| Row 1, Col 5 | Transfer City | Click an allied unit or city |
| Row 2, Col 1 | Factory | Place a factory on the build grid |
| Row 2, Col 2 | University | Place a university on the build grid |

Location-based building commands show a runtime world-space cell grid. Green
means the footprint passes the local map, slope, and collision checks; red means
the placement is invalid. Left click commits, right click cancels, and holding
either Shift key while left-clicking keeps placement active for repeated builds.
The server repeats the snap and validation before spawning a Mass building.

## Officer construction command cards

The Japanese officer subtypes `3226` and `3227` use the reusable `Builder`
loadout. Their main command card keeps the standard unit orders and places two
StarCraft-style construction category buttons on the bottom row:

| Slot | Command | Behavior |
| --- | --- | --- |
| Row 3, Col 1 | Build Defensive Structures | Opens `BuilderDefense` |
| Row 3, Col 2 | Build Production Structures | Opens `BuilderProduction` |

`BuilderProduction` now exposes **Build Barracks** in row 1, column 1. It opens
the shared circular placement grid with a 1x1 building footprint. After a legal
cell is committed, the closest selected officer with the `Builder` loadout walks
beside the site, constructs for 14 seconds (14 game days), and spawns the existing
Japanese barracks Mass AgentConfig. The officer remains available afterwards.
Holding Shift keeps placement active and appends each additional barracks site to
that officer's construction queue. Non-Shift placement replaces that officer's
unfinished construction queue. Ground move and attack-move locations also support
Shift waypoint queues.

`BuilderDefense` exposes **Build Field Bunker** in row 1, column 1 with a 1x1
placement footprint. The officer remains in place while the nearest available
friendly infantry inside the configured target-centered search radius walks to the
site and performs the construction. Infantry already reserved by another officer's
construction order is skipped. The default assignment is one infantry unit within
128 cells; both values are configurable. Shift placement appends defensive sites to
the issuing officer's construction queue.

Config-driven submenus use `SubMenuLoadoutId`; child cards can generate their
Back button with `BackToLoadoutId` and the `BackButton*` presentation fields.
Menu commands use `RTS.Command.Menu.*`, so opening a card never starts target
placement or reaches a construction command handler.

Placement is configured in `Config/DefaultRTSInputSystem.ini`, section
`[/Script/RTSInputSystem.RTSInputPanelSettings]`. The current map protocol is:

```ini
BuildGridMapResolution=(X=4096,Y=4096)
BuildGridWorldOrigin=(X=0.0,Y=0.0)
HashGridCellSize=16.0
bConstrainBuildingPlacementToMap=true
BuildPlacementMaxSlopeDegrees=30.0
bValidateBuildPlacementCollision=true
```

At these defaults, the logical grid covers 65,536 Unreal units per axis, from
-32,768 to +32,768 around the configured origin. Each command can override its
footprint in logical cells through `PlacementFootprintCells`. The runtime line
grid requires no material; `HashGridSelectionDecalMaterial` remains an optional
project-specific projected overlay.

### [Camera Bounds](https://github.com/HeyZoos/RTSInputSystem/wiki/Camera-Bounds)

https://user-images.githubusercontent.com/9408481/223589311-9d6b1cfd-76b4-4650-a6a3-0386a483bb96.mp4

### [Jump To](https://github.com/HeyZoos/RTSInputSystem/wiki/Jump-To)

https://user-images.githubusercontent.com/9408481/223585144-d7e9c1c2-2e36-4628-9bbd-da91229e39e1.mp4

# Changelog

### 0.21.0

- Add [Unit Selection](https://github.com/HeyZoos/RTSInputSystem/wiki/Unit-Selection)

### 0.20.0

- Build for Unreal Engine v5.3.X

### 0.19.0

- Add "Jump To" method
- Start overhauling documentation

### 0.18.0

- Fix crash when using the camera in a networked context
- **No longer take over view target automatically, the view target must be explicitly set, this is documented in the "Getting Started" section of the wiki**

### 0.17.0

- Fix [#27](https://github.com/HeyZoos/RTSInputSystem/issues/27) by tying camera movement to delta time (thanks [@theMyll](https://github.com/theMyll))
- **This will result in slower movement across the board, if you notice your camera moving more slowly, up the speed values by about 100x. For example, the new camera blueprint speed defaults are 5000**
