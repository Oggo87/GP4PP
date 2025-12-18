# Assets Settings

These sections contain entries to control how meshes and textures are loaded in GP4. There are three types of sections:

* [Individual Meshes](#Individual-Meshes)
* [LOD Table](#LOD-Table)
* [Assets Sections](#Assets-Sections)

## Individual Meshes

This section contains entries to enable individual mesh loading for mesh types that are generic by default in GP4: Front Wheels, Rear Wheels, Helmets and Cockpits. Entries can be set to `true` / `false` or `1` / `0`.

Cars are not included in this section, as they already use individual loading by default and `GP4PP.dll`, by design choice, does not allow to set them as generic.

```ini
[IndividualMeshes]
FrontWheels = false
RearWheels = false
Helmets = false
Cockpits = false
```

## LOD Table

This section allows to specify which LODs (Levels of Detail) will be used by each mesh type. GP4 allows a maximum of 5 LODs per mesh type. `GP4PP.dll` will automatically exclude all 0 values at the _end_ of each entry, and will also 0-fill any entries that contain less than 5 values, guaranteeing full compatibility with GP4 internal loading. 

```ini
[LODTable]
FrontWheels	=	0,	1,	2,	3,	0
RearWheels	=	0,	1,	2,	3,	0
Helmets		=	0,	1,	2,	3,	0
Cockpits	=	0,	0,	0,	0,	0
Cars		=	0,	1,	2,	3,	4
```

## Assets Sections

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

### AutoName

Available for all assets, `AutoName` works in conjunction with `IndividualMeshes`, `PerTeam`, `PerDriver`, `PerTrack` and `TrackFolders`, and allows `GP4PP.dll` to automatically determine what file names to use, depending on the chosen options.

`AutoName` follows the convention of original GP4 files, trying to keep it as close as possible to the original file names, including the little differences like `LOD_0` for Cars and Wheels and `00` for Helmets and Cockpits.

See [`PerTrack`](#PerTeam-PerDriver-PerTrack) for some more details, it is recommended to check the GPxPatch debug log for the asset name(s) being loaded.

### LOD0Only

Available for mesh entries only, overrides the corresponding entry in the [LOD Table](#LOD-Table), forcing to use only LOD 0.

### PerTeam, PerDriver, PerTrack

Available for all assets, these settings only have an effect when [`AutoName`](#AutoName) is enabled for the corresponding asset.

* `PerTrack`, when not using `TrackFolders`, will append `_track_{track}` at the end of the filename.
* `PerDriver` will combine `{teamname}` with car `1` and `2` for meshes, while using `{driver}` for textures.

See [`FileName`](#FileName) for more details about `{variables}`.

### FileName

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
