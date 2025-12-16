# GP4++

A DLL to enhance and extend Grand Prix 4 functionality.

## Description

This DLL, formerly known as *Individual Meshes And Textures*, expands original Grand Prix 4 to allow full control of how meshes (front/rear wheels, helmets, cockpits, cars, collision meshes) and textures (cockpits, helmets) are loaded, supporting per-driver, per-team, and per-track custom loading. It includes features to manage LODs (Level of Detail), auto-naming conventions, and fallbacks to default GP4 assets when custom assets are unavailable.

Additionally, it includes and expands on features originally found in [*High Resolution Pitcrews for Grand Prix 4* (`HiResPitcrew.dll`)](https://github.com/Oggo87/HiResPitcrew), both overcoming the limitation of 400 vertices in the original game, allowing high resolution pitcrew meshes to be used, and introducing support for per-team pitcrew meshes.

It also includes options to fully control cockpit and helmet visor colours and transparency, and enable the use of GP4's built-in visor shader in the cockpit mesh.

Other features include:
* Fixing the reversed tyre tread bug when using 3D rims
* Disabling the built-in check for an optical drive

See [Usage](#Usage) for more info.

## Getting Started

### Dependencies

* [GP4MemLib](https://github.com/Oggo87/GP4MemLib)
* [IniLib](https://github.com/Oggo87/IniLib)

### Prerequisites

* Grand Prix 4
* Carset Manager (CSM) by ZaZ (recommended) or)
* GP4 Memory Access by Carl_gpgames (recommended) or
* CheatEngine or
* Any other DLL injection tool

### Installing

#### Carset Manager (CSM)

In the `Files` section of the CSM file, add the subpath to the DLL. Make sure that `GP4PP.ini` is located in the same folder as the DLL.

Example - `GP4PP.dll` and `MyOtherDLL.dll` located in the `Files` folder inside the main mod data folder

```ini
[Files]
InjectDll = ""%DataPath%Files\GP4PP.dll","%DataPath%Files\MyOtherDLL.dll""
```
#### GP4 Memory Access

Under the *DLLs* tab, add an entry pointing to `GP4PP.dll`. Make sure that `GP4PP.ini` is located in the same folder as the DLL.

### Usage

All settings are contained in `GP4PP.ini` and everything is designed to revert back to default GP4 behaviour, including if `GP4PP.ini` itself missing. The `GP4PP.ini` file that is included with the release is also pre-configured to mimic GP4's default behaviour.

`GP4PP.ini` is divided in 4 logical sections: 

* [General Settings](#General-Settings)
* [Visor Settings](#Visor-Settings)
* [Pitcrew Settings](#Pitcrew-Settings)
* [Assets Settings](#Assets-Settings)

#### General Settings
This section contains four entries:

* [Disable CD Check](#Disable-CD-Check)
* [Fix 3D Wheels](#Fix-3D-Wheels)
* [Track Folders](#Track-Folders)
* [Cockpit Visor](#Cockpit-Visor)
* [Steering Wheel Advanced Car Sahder](#Steering-Wheel-Advanced-Car-Shader)

```ini
[Settings]
DisableCDCheck = false
Fix3DWheels = false
TrackFolders = false
CockpitVisor = false
StWheelAdvancedCarShader = false
```

##### Disable CD Check

This utility setting disables the built-in check for an optical drive in GP4, allowing the game to run without requiring a CD/DVD drive or a mounted image.

##### Fix 3D Wheels

This utility setting fixes the reversed tyre tread bug that occurs when using 3D rims in GP4. When enabled, the tyre treads will display correctly when `Rotate wheels via textures v2` is disabled in `f1graphics.cfg`.

##### Track Folders

This utility setting, in combination with `AutoName` found in each individual asset section (see [Assets Settings](#Assets-Settings)), tells `GP4PP.dll` to look into track subfolders for those assets that are have `PerTrack` set to `true`.

Tracks are indexed 1 through 17.

Example - Per-Track Cars with Track Folders enabled

```ini
[Settings]
TrackFolders = true

[Cars]
AutoName = true
PerTrack = true
```

Ferrari's first car (LOD 0) at Monza will be loaded as follows

```
cars\track15\car_ferrari_car1_lod_0.gp4
```

##### Cockpit Visor

This setting allows to enable using GP4's built-in visor shader in the cockpit mesh. In combination with the entries in the ```CockpitVisor``` section, it's possible to specify the ```ObjectName``` to apply the visor shader to, the ```Colour```,  and ```Transparency```. See [Visor Settings](#Visor-Settings) for more details.

Example - Cockpit Visor enabled, using object named `Visor`, with a grey colour and 50% transparency, for all cars

```ini
[Settings]
CockpitVisor = true

[CockpitVisor]
ObjectName = Visor
Individual = false
Colour = 128, 128, 128, 0
Transparency = 0.5
```

##### Steering Wheel Advanced Car Shader

This setting allows to enable using GP4's built-in advanced car shader (the same one used for cars and helmets) for the in-cockpit steering wheel mesh, improving its appearance with better reflections and lighting effects.

Advanced Car Shader does not support alpha-blending, so if the steering wheel mesh uses any alpha-blending textures (for example, for cutouts or holes in the mesh), those parts will appear fully opaque when this setting is enabled. It is intended to be used with full 3D steering wheel meshes that do not rely on alpha-blending.

#### Visor Settings

These sections contain entries to set the colour and transparency of both helmet and cockpit visors. ```CockpitVisor``` settings are applied to the in-cockpit visor when the corresponding setting in the ```Settings``` section is enabled, ```ZCockpitVisor``` and ```HelmetVisor``` settings are always applied to ```z_cockpit_visor``` objects in the car mesh and helmet visors respectively.

For each section, it's possible to specify whether the settings are applied individually (when ```Individual``` is set to ```true```/```1```), or globally for all cars (when ```Individual``` is set to ```false```/```0```).

When not using individual settings, it's possible to specify the ```Colour``` (in ```0-255 BGRA``` format),  and ```Transparency``` (a ```float``` value) that will be applied to all cars. The Alpha component of the ```Colour``` and the ```Transparency``` value work combinedly to set the visor's transparency. These settings will also be used as default values when using individual settings and a car doesn't have specific entries in the specified INI file.

When using individual settings, it's possible to specify different colours and transparency values for each team by creating a separate INI file, specified in the ```Ini``` entry. The INI file needs to contain one section per car, named sequentially 1-22, with the same ```Colour``` and ```Transparency``` entries as above.

Example - Global settings for all visors

```ini
[CockpitVisor]
ObjectName = Visor
Individual = false
Colour = 128, 128, 128, 0
Transparency = 0.5
Ini = VisorColours.ini

[ZCockpitVisor]
Individual = false
Colour = 128, 128, 128, 0
Transparency = 0.3
Ini = VisorColours.ini

[HelmetVisor]
Individual = true
Colour = 0, 0, 0, 0
Transparency = 0.5
Ini = VisorColours.ini
```

Example - Individual settings for visors

```ini
[ZCockpitVisor1]
Colour = 128, 128, 128, 0
Transparency = 0.5

[CockpitVisor4]
Colour = 128, 128, 128, 0
Transparency = 0.3

[HelmetVisor14]
Colour = 0, 30, 50, 0
Transparency = 0.65

```

#### Pitcrew Settings

This section contains entries to enable high resolution pitcrew meshes and per-team pitcrew meshes.

```ini
[Pitcrews]
HiResMeshes = false
IndividualMeshes = false
CustomD3DBufferSize = 0x400000
```
##### Hi-Resolution Meshes
This setting allows to enable loading high resolution pitcrew meshes, overcoming the original GP4 limitation of 400 vertices per pitcrew mesh. The entry can be set to `true` / `false` or `1` / `0`.

##### Individual Meshes
This setting allow to enable loading per-team pitcrew meshes. The entry can be set to `true` / `false` or `1` / `0`.

When enabled, all `animsetup.set` and `animsetupmonaco.set` files, located in both the `qualifying` and `race` folders of `anim.wad`, need to be modified to include 12 entries for the meshes, one per team, instead of the original single entry for all teams, plus the Marshall mesh. Failure to do so will result in crashes or other issues.

Example - Original vs Modified `animsetup.set` and `animsetupmonaco.set` for the `qualifying` folder

<table>
<tr>
<th>Original</th>
<th>Modified</th>
</tr>
<tr>
<td>

```
2
Engineer.mdb
Marshall.mdb
2
EngineerLo.mdb
MarshallLo.mdb
```
</td>
<td>

```
12
Engineer_Ferrari.mdb
Engineer_Mclaren.mdb
Engineer_Williams.mdb
Engineer_Benetton.mdb
Engineer_Bar.mdb
Engineer_Jordan.mdb
Engineer_Arrows.mdb
Engineer_Sauber.mdb
Engineer_Jaguar.mdb
Engineer_Minardi.mdb
Engineer_Prost.mdb
Marshall.mdb
12
Engineer_FerrariLo.mdb
Engineer_MclarenLo.mdb
Engineer_WilliamsLo.mdb
Engineer_BenettonLo.mdb
Engineer_BarLo.mdb
Engineer_JordanLo.mdb
Engineer_ArrowsLo.mdb
Engineer_SauberLo.mdb
Engineer_JaguarLo.mdb
Engineer_MinardiLo.mdb
Engineer_ProstLo.mdb
MarshallLo.mdb
```

</td>
</tr>
</table>

##### Custom D3D Buffer Size
This setting allows to specify a custom size (in either decimal or hexadecimal format) for the Direct3D buffer used to load pitcrew meshes. By default, GP4 uses a 0x4000 (16384 bytes) buffer, which works fine for the original low-poly pitcrew meshes, but not enough for high resolution pitcrew meshes. Some instability issues may arise if the buffer size is too large, hence the default setting in `GP4PP.ini` is set to 0x400000 (4 megabytes), which should be enough for most high resolution pitcrew meshes. If `HiResMeshes` is set to `false`, this setting is ignored and GP4's default buffer size is used.

#### Assets Settings

These sections contain entries to control how meshes and textures are loaded in GP4. There are three types of sections:

* [Individual Meshes](#Individual-Meshes)
* [LOD Table](#LOD-Table)
* [Assets Sections](#Assets-Sections)

##### Individual Meshes

This section contains entries to enable individual mesh loading for mesh types that are generic by default in GP4: Front Wheels, Rear Wheels, Helmets and Cockpits. Entries can be set to `true` / `false` or `1` / `0`.

Cars are not included in this section, as they already use individual loading by default and `GP4PP.dll`, by design choice, does not allow to set them as generic.

```ini
[IndividualMeshes]
FrontWheels = false
RearWheels = false
Helmets = false
Cockpits = false
```

##### LOD Table
This section allows to specify which LODs (Levels of Detail) will be used by each mesh type. GP4 allows a maximum of 5 LODs per mesh type. `GP4PP.dll` will automatically exclude all 0 values at the _end_ of each entry, and will also 0-fill any entries that contain less than 5 values, guaranteeing full compatibility with GP4 internal loading. 

```ini
[LODTable]
FrontWheels	=	0,	1,	2,	3,	0
RearWheels	=	0,	1,	2,	3,	0
Helmets		=	0,	1,	2,	3,	0
Cockpits	=	0,	0,	0,	0,	0
Cars		=	0,	1,	2,	3,	4
```

##### Assets Sections

These sections are, more specifically:

* Front Wheels
* Rear Wheels
* Helmets
* Cockpits
* Cars
* Collision Mesh
* Helmet Textures
* Cockpit Textures

They all share a common structure, with some minor differences for the two texture sections:

```ini
AutoName = true
LOD0Only = false
PerTeam = false
PerDriver = false
PerTrack = false
FileName = 
Tracks = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17
```

##### AutoName
Available for all assets, `AutoName` works in conjunction with `IndividualMeshes`, `PerTeam`, `PerDriver`, `PerTrack` and `TrackFolders`, and allows `GP4PP.dll` to automatically determine what file names to use, depending on the chosen options.

`AutoName` follows the convention of original GP4 files, trying to keep it as close as possible to the original file names, including the little differences like `LOD_0` for Cars and Wheels and `00` for Helmets and Cockpits.

See [`PerTrack`](#PerTeam-PerDriver-PerTrack) for some more details, it is recommended to check the GPxPatch debug log for the asset name(s) being loaded.

##### LOD0Only
Available for mesh entries only, overrides the corresponding entry in the [LOD Table](#LOD-Table), forcing to use only LOD 0.

##### PerTeam, PerDriver, PerTrack
Available for all assets, these settings only have an effect when [`AutoName`](#AutoName) is enabled for the corresponding asset.

* `PerTrack`, when not using `TrackFolders`, will append `_track_{track}` at the end of the filename.
* `PerDriver` will combine `{teamname}` with car `1` and `2` for meshes, while using `{driver}` for textures.

See [`FileName`](#FileName) for more details about `{variables}`.

##### FileName
Available for all assets, this setting allows to specify a custom file name pattern for each asset. For **Helmet Textures**, there are two independent settings, one per texture: `FileName1` and `FileName2`.

7 `{variables}` are available to use in the file names, enclosed in curly brackets:

* `{track}` - Available for all assets. Indexed 1 through 17
* `{team}` - Available for collision mesh and all textures, available for meshes only when [`IndividualMeshes`](#Individual-Meshes) is enabled for the corresponding mesh. Indexed 1 through 11.
* `{teamname}` - Available for collision mesh and all textures, available for meshes only when [`IndividualMeshes`](#Individual-Meshes) is enabled for the corresponding mesh.
* `{car}` - Available for all textures, available for meshes only when [`IndividualMeshes`](#Individual-Meshes) is enabled for the corresponding mesh. Indexed 1 and 2 for first and second car of the team.
* `{driver}` - Available for collision mesh and all textures, available for meshes only when [`IndividualMeshes`](#Individual-Meshes) is enabled for the corresponding mesh. Indexed 1 through 12 and 14 through 23 (as for original GP4 helmet textures, following 2001 car numbering)
* `{driverindex}` - Available for collision mesh and all textures, available for meshes only when [`IndividualMeshes`](#Individual-Meshes) is enabled for the corresponding mesh. Indexed 1 through 22.
* `{lod}` - Available for for meshes only, follows the corresponding entry in the [LOD Table](#LOD-Table)

Please note that collision mesh and texture entries need to have the extension explicitly written as either `.tex` or `.tga` for textures and `.ct` for collision mesh

Example - Per-Team Helmet mesh and Per-Track Helmet Textures

```ini
[Helmets]
FileName=car_helmet_{teamname}_0{lod}

[HelmetTextures]
FileName1=driver{driver}_track_{track}_1.tga
FileName1=driver{driver}_track_{track}_2.tga
```

Ferrari's first driver helmet assets at Monza will be loaded as follows

```
cars\car_helmet_ferrari_00.gp4
cars\driver1_track_15_1.tex
cars\driver1_track_15_2.tex
```

Subolders can be included as part of the `FileName` entry. Please note that the directory separator character has to be escaped and written as `\\`.

Example - Per-Track generic Helmet mesh

```ini
[Helmets]
FileName=track{track}\\car_helmet_0{lod}
```

The Helmet mesh at Monza will be loaded as follows

```
cars\track15\car_helmet_00.gp4
```

##### Tracks
Available for all assets, this setting allows some additional control when using per-track assets, either with the [`PerTrack`](#PerTeam-PerDriver-PerTrack) setting or the manual `{track}` variable. Acknowledging that not all tracks might need their own unique asset, with this setting is possible to specify a fall-back track index for each track slot. If an asset isn't found for the track slot, the asset from the specified fall-back track slot will be attempted to be loaded, before reverting back to GP4's default. If less than 17 entries are present, track 1 will be used for the missing track slots.

Example - Per-Team Helmet mesh, with Track Folders enabled and all fall-back tracks set to 1

```ini
[Settings]
TrackFolders = true

[Cars]
AutoName = true
PerTrack = true
Tracks = 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
```

Ferrari's first car (LOD 0) at Monza will be loaded as follows

```
cars\track15\car_ferrari_car1_lod_0.gp4
```

If the asset doesn't exist, the fall-back track slot will be used

```
cars\track1\car_ferrari_car1_lod_0.gp4
```

If the fall-back asset also doesn't exist, GP4's default file will be used

```
cars\car_ferrari_car1_lod_0.gp4
```

## Authors

Diego "Öggo" Noriega

## Version History

* 2.1
    * Added settings for external cockpit and helmet visor colours and transparency
    * Added settings to enable individual colour and transparency per car for cockpit and helmet visors
    * Some bug fixes and improvements

* * 2.0
    * First release as GP4++
    * Major refactor of the codebase
    * Merged HiResPitcrew.dll functionality into GP4PP.dll
    * Added support for per-team pitcrew meshes
    * Added patch to disable built-in CD check
    * Added patch to enable advanced car shader for in-cockpit steering wheel mesh

* 1.4
    * Added fix for reversed tyre tread bug when using 3D rims

* 1.3
    * Enable using GP4 Visor Shader for cockpit mesh

* 1.2
    * Added collision mesh assets

* 1.1
    * Fall-back track slots

* 1.0
    * First Release

* 0.1
    * Pre-Release
