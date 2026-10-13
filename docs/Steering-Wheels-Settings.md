# Steering Wheels Settings

This section contains entries to give each team its own steering wheel. GP4 normally uses a single steering wheel for everyone: one texture (`stwheel.tex`) for the external steering wheels of all cars, and one mesh (`cockpit_wheel.gp4`) for the steering wheel seen from the cockpit view.

* [External](#External)
* [Cockpit](#Cockpit)
* [AutoName](#AutoName)
* [ExternalFileName and CockpitFileName](#ExternalFileName-and-CockpitFileName)

```ini
[SteeringWheels]
External = false
Cockpit = false
AutoName = true
ExternalFileName = 
CockpitFileName = 
```

Both features are disabled by default. When enabled, every team that has its own file gets its own steering wheel, while teams without one keep the default GP4 steering wheel. All files are checked the same way GP4 checks its own assets, so they can be placed in `cars.wad` or, when GP4 does not use the WAD, in the `cars` folder.

## External

Enables per-team textures for the external steering wheels, i.e. the steering wheels seen on the cars from any external camera, TV camera or replay. The entry can be set to `true` / `false` or `1` / `0`.

The team textures are loaded together with GP4's own `stwheel.tex`, from the same `cars` folder. Only the texture changes: all teams share GP4's external steering wheel geometry.

## Cockpit

Enables per-team steering wheel meshes in the cockpit view. The entry can be set to `true` / `false` or `1` / `0`.

Each team's mesh is a full replacement for `cars\cockpits\cockpit_wheel.gp4`, so it can have its own shape, objects and textures. The mesh of the team of the car being viewed is selected automatically, including when switching between cars. Displays, lights and buttons keep working for every object the team's mesh contains, using the same object names as the default mesh. Objects that are missing from a team's mesh are simply not shown.

The team meshes are loaded together with the default cockpit meshes and are released when returning to the menu.

The team meshes use the same shader as the default cockpit steering wheel, so they follow the [Steering Wheel Advanced Car Shader](General-Settings.md#Steering-Wheel-Advanced-Car-Shader) setting.

### Textures used by a cockpit mesh

GP4 looks for the textures used by a `.gp4` mesh in the folder of the mesh itself, and loads each texture only once per full path. Two meshes in the same folder that use a texture with the same name therefore share that texture, and a team mesh placed next to the default `cockpit_wheel.gp4` would show the default steering wheel textures wherever the names match.

For this reason, the default naming puts each team's mesh in its own subfolder, together with its textures. Team meshes can then use the same texture names as the default mesh, and as each other, without conflicts.

## AutoName

When enabled, the default file names below are used. When disabled, the names in `ExternalFileName` and `CockpitFileName` are used, falling back to the default names when left empty.

| Asset | Default file name |
|---|---|
| External steering wheel texture | `stwheel_{teamname}.tga` |
| Cockpit steering wheel mesh | `cockpits\{teamname}\cockpit_wheel.gp4` |

Example - Ferrari's steering wheels with `AutoName` enabled

```
cars\stwheel_ferrari.tex
cars\cockpits\ferrari\cockpit_wheel.gp4
cars\cockpits\ferrari\<textures used by the mesh>.tex
```

## ExternalFileName and CockpitFileName

These settings allow to specify a custom file name pattern for each of the two assets, relative to the `cars` folder. For the texture, GP4 always loads a `.tex` file: any extension (`.tga`, `.tex`) is replaced by `.tex`, and `.tex` is added when there is none. The mesh name has to include `.gp4`.

2 `{variables}` are available to use in the file names, enclosed in curly brackets:

* `{teamname}` - the team name, as used for the car meshes
* `{team}` - the team index, indexed 1 through 11

Subfolders can be included as part of the file name. Please note that the directory separator character has to be escaped and written as `\\`.

Example - Team steering wheels in a `stwheels` folder, one subfolder per team

```ini
[SteeringWheels]
External = true
Cockpit = true
AutoName = false
ExternalFileName = stwheels\\{teamname}\\stwheel.tga
CockpitFileName = stwheels\\{teamname}\\cockpit_wheel.gp4
```

Ferrari's steering wheels will be loaded as follows

```
cars\stwheels\ferrari\stwheel.tex
cars\stwheels\ferrari\cockpit_wheel.gp4
```

It is recommended to check the GPxPatch debug log for the file names being loaded: every team is listed with the file that was loaded, or with the file that was not found.

## Credits

Concept and reverse engineering from the ZaZ GP4 Tools proof of concept (Ronald Plukker).
