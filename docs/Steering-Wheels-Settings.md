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
UseCockpitMesh = false
CockpitMeshScale = 0.7

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

### UseCockpitMesh

When enabled, the external steering wheels are drawn in 3D with the cockpit steering wheel mesh, instead of GP4's flat textured steering wheel. Only the objects the cockpit view shows when nothing is lit are always shown: `steering_wheel`, `Warning_off`, `LED01_off` to `LED05_off` and the `Water_T_on`, `Fuel_F_on` and `Lap_L_on` labels, together with each car's own:

* rev lights (`Left_REV1_on` to `Left_REV5_on` and `Right_REV1_on` to `Right_REV5_on`)
* flag lights (`Left_flag1_on` to `Left_flag3_on` and `Right_flag1_on` to `Right_flag3_on`), for yellow and blue flags, failures and warnings
* gear (`Gear_R`, `Gear_N`, `Gear_1` to `Gear_7`)
* water temperature (`Water_A_1`, `Water_B_0` to `Water_B_9`, `Water_C_0` to `Water_C_9`)
* fuel laps (`Fuel_remaining_point_on`, `Fuel_A_*`, `Fuel_B_*`, `Fuel_C_*`). As on GP4's own dash, in a race only the player's car shows its fuel; every other car shows `--.-` (`Fuel_F_minus`, `Fuel_A_minus`, `Fuel_B_minus`, `Fuel_C_minus`), unless [Show Opponents Fuel Laps](General-Settings.md#Show-Opponents-Fuel-Laps) is enabled
* position (`Position_P_on`, `Position_A_*`, `Position_B_*`): race position, or best lap order in the other sessions
* lap (`Lap_A_*`, `Lap_B_*`)
* lap time (`lap_time_colon_on`, `lap_time_point_on`, `Lap_Time_A_*` to `Lap_Time_F_*`): the running lap time in whole seconds, then for about 6 seconds after crossing a timing line the lap time, or outside a race the split time, to the millisecond. As on GP4's own dash, the time is held longer while a warning or failure message is shown on the car's LCD, and is `-:--.---` until the car starts a lap
* speed (`Speed_A_0` to `Speed_A_3`, `Speed_B_*`, `Speed_C_*`), in the units chosen in GP4
* driver status (`Driver_Status_off`, `Driver_Status_red` on the player's car)
* warning lights (`Warning_left_front_tyre_1` and the other tyre, wing and plank warnings)
* pit lights (`Pit_1` for the car's own pit light, `Pit_2` for its team-mate's in a race)

These work the same way they do in the cockpit view. As in the cockpit view, the off state objects of the aids, rev lights, flags, pit and opposition spread displays are never shown. Only objects that are part of the steering wheel mesh are shown, so for example a gear display that is part of the dash mesh is not. The log lists how many objects of each display a steering wheel mesh has. Other displays (suggested gear, aids, LEDs, opposition spread), mirrors and the LCD are not shown.

GP4's own flat gloves and arms are still drawn on the 3D external steering wheels, in the same place and with the same team arm colours as on the flat steering wheel; only the flat steering wheel and its rev lights are left out.

When GP4 Tweaker's `SteerPos` tweak is enabled, the 3D external steering wheels use the per-car position set in GP4 Tweaker, the same as the flat steering wheels.

When `UseCockpitMesh` is enabled, every [CockpitSteeringWheels](#CockpitSteeringWheels) setting applies to both the cockpit and the external steering wheels: each car's external steering wheel is the same mesh it has in the cockpit view, per team or per car when enabled (`PerTeam`, `PerDriver`, `AutoName`, `FileName`), the default `cockpit_wheel.gp4` otherwise. `PerTeam`, `PerDriver`, `AutoName` and `FileName` of this section only choose the flat external steering wheel textures; while `UseCockpitMesh` is enabled all cars use the 3D steering wheel, so those textures are only used for the gloves and arms.

### CockpitMeshScale

Scale of the 3D external steering wheels, applied to the whole mesh around its rotation point. The cockpit steering wheel mesh is larger than GP4's flat steering wheel, and `0.7` matches it closely. Values of 0 or less are ignored and `0.7` is used instead. Only used when `UseCockpitMesh` is enabled.

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
