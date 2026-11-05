# Steering Wheels Settings

These sections contain entries to give each team, or each car, its own steering wheel and cockpit dash. GP4 normally uses a single steering wheel for everyone: one texture (`stwheel.tex`) for the external steering wheels of all cars, and one mesh (`cockpit_wheel.gp4`) for the steering wheel seen from the cockpit view. Likewise, a single mesh (`cockpit_dash.gp4`) is used for the dash of every car.

* [ExternalSteeringWheels](#ExternalSteeringWheels)
* [CockpitSteeringWheels](#CockpitSteeringWheels)
* [CockpitDashes](#CockpitDashes)
* [PerTeam and PerDriver](#PerTeam-and-PerDriver)
* [AutoName](#AutoName)
* [FileName](#FileName)

```ini
[ExternalSteeringWheels]
PerTeam = false
PerDriver = false
AutoName = true
FileName = 

[CockpitSteeringWheels]
PerTeam = false
PerDriver = false
AutoName = true
FileName = 

[CockpitDashes]
PerTeam = false
PerDriver = false
AutoName = true
FileName = 
```

All sections work the same way and are independent of each other. All are disabled by default, and GP4's own steering wheels and dash are used.

A section is enabled when:

* `AutoName` is enabled and `PerTeam` or `PerDriver` is enabled, or
* `AutoName` is disabled and `FileName` is not empty

Every car is handled individually: the file name decides whether two cars share a steering wheel or dash. Every car that has its own file gets its own steering wheel or dash, while cars without one keep the default GP4 one. All files are checked the same way GP4 checks its own assets, so they can be placed in `cars.wad` or, when GP4 does not use the WAD, in the `cars` folder.

## ExternalSteeringWheels

Per-team or per-car textures for the external steering wheels, i.e. the steering wheels seen on the cars from any external camera, TV camera or replay.

The textures are loaded together with GP4's own `stwheel.tex`, from the same `cars` folder. Only the texture changes: all cars share GP4's external steering wheel geometry.

## CockpitSteeringWheels

Per-team or per-car steering wheel meshes in the cockpit view.

Each mesh is a full replacement for `cars\cockpits\cockpit_wheel.gp4`, so it can have its own shape, objects and textures. The mesh of the car being viewed is selected automatically, including when switching between cars. Displays, lights and buttons keep working for every object the mesh contains, using the same object names as the default mesh. Objects that are missing from a mesh are simply not shown.

The meshes are loaded together with the default cockpit meshes, once per file, and are released when returning to the menu.

The meshes use the same shader as the default cockpit steering wheel, so they follow the [Steering Wheel Advanced Car Shader](General-Settings.md#Steering-Wheel-Advanced-Car-Shader) setting.

## CockpitDashes

Per-team or per-car dash meshes in the cockpit view.

Each mesh is a full replacement for `cars\cockpits\cockpit_dash.gp4`, and works the same way as the cockpit steering wheel meshes: it is selected automatically for the car being viewed, it is loaded once per file and released when returning to the menu, and it uses the same shader as the default cockpit dash.

Dashes and cockpit steering wheels are independent of each other, and can be per team or per car separately. Displays, lights and buttons keep working for every object the dash and steering wheel of the viewed car contain, using the same object names as the default meshes.

## Textures used by a cockpit mesh

GP4 looks for the textures used by a `.gp4` mesh in the folder of the mesh itself, and loads each texture only once per full path. Two meshes in the same folder that use a texture with the same name therefore share that texture, and a mesh placed next to the default `cockpit_wheel.gp4` would show the default steering wheel textures wherever the names match.

For this reason, the default naming puts each mesh in its own subfolder, together with its textures. Meshes can then use the same texture names as the default mesh, and as each other, without conflicts.

Cockpit dashes use the same subfolders by default, so a team's or car's cockpit dash and steering wheel are kept together, and share any texture with the same name.

## PerTeam and PerDriver

These settings only have an effect on the file names when [`AutoName`](#AutoName) is enabled.

* `PerTeam` gives each team its own steering wheel or dash, shared by both of its cars.
* `PerDriver` gives each car its own steering wheel or dash. As for the other assets, `PerDriver` supersedes `PerTeam`.

## AutoName

When enabled, the file names below are used, depending on `PerTeam` and `PerDriver`. When disabled, the name in `FileName` is used.

| Asset | `PerTeam` | `PerDriver` |
|---|---|---|
| External steering wheel texture | `stwheel_{teamname}.tga` | `stwheel_{teamname}_car{car}.tga` |
| Cockpit steering wheel mesh | `cockpits\{teamname}\cockpit_wheel.gp4` | `cockpits\{teamname}_car{car}\cockpit_wheel.gp4` |
| Cockpit dash mesh | `cockpits\{teamname}\cockpit_dash.gp4` | `cockpits\{teamname}_car{car}\cockpit_dash.gp4` |

Example - Ferrari's steering wheels and dash with `PerTeam` enabled in all sections

```
cars\stwheel_ferrari.tex
cars\cockpits\ferrari\cockpit_wheel.gp4
cars\cockpits\ferrari\cockpit_dash.gp4
cars\cockpits\ferrari\<textures used by the meshes>.tex
```

Example - Ferrari's steering wheels and dashes with `PerDriver` enabled in all sections

```
cars\stwheel_ferrari_car1.tex
cars\stwheel_ferrari_car2.tex
cars\cockpits\ferrari_car1\cockpit_wheel.gp4
cars\cockpits\ferrari_car1\cockpit_dash.gp4
cars\cockpits\ferrari_car1\<textures used by the meshes>.tex
cars\cockpits\ferrari_car2\cockpit_wheel.gp4
cars\cockpits\ferrari_car2\cockpit_dash.gp4
cars\cockpits\ferrari_car2\<textures used by the meshes>.tex
```

## FileName

Used when [`AutoName`](#AutoName) is disabled, this setting allows to specify a custom file name pattern, relative to the `cars` folder. When left empty, GP4's own steering wheel or dash is used. For the texture, GP4 always loads a `.tex` file: any extension (`.tga`, `.tex`) is replaced by `.tex`, and `.tex` is added when there is none. Mesh names have to include `.gp4`.

4 `{variables}` are available to use in the file names, enclosed in curly brackets:

* `{teamname}` - the team name, as used for the car meshes
* `{team}` - the team index, indexed 1 through 11
* `{car}` - indexed 1 and 2 for first and second car of the team
* `{driverindex}` - the car index, indexed 1 through 22

Whether a steering wheel or dash is per team or per car only depends on the variables used: a name with `{car}` or `{driverindex}` gives each car its own file, a name with only `{teamname}` or `{team}` is shared by both cars of a team.

`{driver}` (the driver number) is not available: the cockpit steering wheels and dashes are loaded before GP4 creates the cars, when the driver numbers are not known yet.

Subfolders can be included as part of the file name. Please note that the directory separator character has to be escaped and written as `\\`.

Example - Per-car steering wheels in a `stwheels` folder, one subfolder per car

```ini
[ExternalSteeringWheels]
AutoName = false
FileName = stwheels\\{teamname}_{car}\\stwheel.tga

[CockpitSteeringWheels]
AutoName = false
FileName = stwheels\\{teamname}_{car}\\cockpit_wheel.gp4
```

Ferrari's steering wheels will be loaded as follows

```
cars\stwheels\ferrari_1\stwheel.tex
cars\stwheels\ferrari_1\cockpit_wheel.gp4
cars\stwheels\ferrari_2\stwheel.tex
cars\stwheels\ferrari_2\cockpit_wheel.gp4
```

It is recommended to check the GPxPatch debug log for the file names being loaded: every car is listed with the file that was loaded, or with the file that was not found.

## Credits

Concept and reverse engineering from the ZaZ GP4 Tools proof of concept (Ronald Plukker).
