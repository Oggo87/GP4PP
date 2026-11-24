# Steering Wheels Settings

These sections contain entries to give each team, or each car, its own steering wheel and cockpit dash. GP4 normally uses a single steering wheel for everyone: one texture (`stwheel.tex`) for the external steering wheels of all cars, and one mesh (`cockpit_wheel.gp4`) for the steering wheel seen from the cockpit view. Likewise, a single mesh (`cockpit_dash.gp4`) is used for the dash of every car.

* [ExternalSteeringWheels](#ExternalSteeringWheels)
* [CockpitSteeringWheels](#CockpitSteeringWheels)
* [CockpitDashes](#CockpitDashes)
* [CockpitLCD](#CockpitLCD)
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

[CockpitLCD]
PerTeam = false
PerDriver = false
PerWheel = false
AutoName = true
FileName = 
AlwaysShow = false
FontSize = 16
FontWeight = 700
Scale = 1.0
Individual = false
Colour = 0, 0, 0
Ini = LCD.ini
```

All sections work the same way and are independent of each other. All are disabled by default, and GP4's own steering wheels and dash are used.

A section is enabled when:

* `AutoName` is enabled and `PerTeam` or `PerDriver` (or `PerWheel` for [CockpitLCD](#CockpitLCD)) is enabled, or
* `AutoName` is disabled and `FileName` is not empty

Every car is handled individually: the file name decides whether two cars share a steering wheel or dash. Every car that has its own file gets its own steering wheel or dash, while cars without one keep the default GP4 one. All files are checked the same way GP4 checks its own assets, so they can be placed in `cars.wad` or, when GP4 does not use the WAD, in the `cars` folder. The only exception are the LCD position files of [CockpitLCD](#CockpitLCD), which are always read from GP4's `fonts` folder on disk.

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

The LCD text is drawn on the mesh's own `LCD` object, with the same layout and size it has on the default steering wheel, so it follows wherever the `LCD` object is placed in the mesh. The object name is not case sensitive (`LCD`, `lcd`). The log lists, for each steering wheel mesh, whether it has an `LCD` object. Steering wheels without an `LCD` object only show the LCD when [`AlwaysShow`](#AlwaysShow) is enabled, and the text positions can be changed per team, car or steering wheel with [CockpitLCD](#CockpitLCD).

The meshes are loaded together with the default cockpit meshes, once per file, and are released when returning to the menu.

The meshes use the same shader as the default cockpit steering wheel, so they follow the [Steering Wheel Advanced Car Shader](General-Settings.md#Steering-Wheel-Advanced-Car-Shader) setting.

## CockpitDashes

Per-team or per-car dash meshes in the cockpit view.

Each mesh is a full replacement for `cars\cockpits\cockpit_dash.gp4`, and works the same way as the cockpit steering wheel meshes: it is selected automatically for the car being viewed, it is loaded once per file and released when returning to the menu, and it uses the same shader as the default cockpit dash.

Dashes and cockpit steering wheels are independent of each other, and can be per team or per car separately. Displays, lights and buttons keep working for every object the dash and steering wheel of the viewed car contain, using the same object names as the default meshes.

## CockpitLCD

Per-team, per-car or per-steering wheel positions of the LCD texts on the cockpit steering wheel. GP4 reads the position of every LCD text from `fonts\<language>.pos`, where `<language>` is the language GP4 runs in (`English`, `French`, `German`, `Spanish`, `Portuguese` or `Italian`). This section gives each team, car or steering wheel its own copy of that file. The LCD texts themselves and their size are always GP4's.

The files are placed in GP4's `fonts` folder, in a subfolder per team, car or steering wheel, with the same name and format as GP4's own file. They have to be on disk: like GP4's own `.pos` files, they are never read from `cars.wad`. The easiest way to make one is to copy GP4's own file and change the positions: only the entries that are in the file are moved, any entry left out keeps GP4's position.

| Setting | File |
|---|---|
| `PerTeam` | `fonts\{teamname}\{language}.pos` |
| `PerDriver` | `fonts\{teamname}_car{car}\{language}.pos` |
| `PerWheel` | `fonts\` followed by the folder of the [CockpitSteeringWheels](#CockpitSteeringWheels) file name |

`PerWheel` gives the LCD positions the same names the cockpit steering wheels use, so every steering wheel can have its own positions, shared by all the cars that use it. It supersedes `PerTeam` and `PerDriver`, and needs [CockpitSteeringWheels](#CockpitSteeringWheels) to be enabled. The folder of the steering wheel file name is used, without the `cockpits\` part when the steering wheels use `AutoName`, and without the extension when the steering wheel file name has no folder. Only cars whose own steering wheel was loaded use these positions; cars that ended up with the default steering wheel keep GP4's.

| [CockpitSteeringWheels](#CockpitSteeringWheels) | LCD positions with `PerWheel` |
|---|---|
| `AutoName`, `PerTeam` | `fonts\{teamname}\{language}.pos` |
| `AutoName`, `PerDriver` | `fonts\{teamname}_car{car}\{language}.pos` |
| `FileName = stwheels\\{teamname}_{car}\\cockpit_wheel.gp4` | `fonts\stwheels\{teamname}_{car}\{language}.pos` |

With `AutoName` disabled, `FileName` is relative to the `fonts` folder, and can use the `{language}` variable as well as the other [variables](#FileName). A file name without `{language}` is used for every language.

Example - Ferrari's LCD positions with `PerTeam` enabled, in English and Italian

```
fonts\ferrari\English.pos
fonts\ferrari\Italian.pos
```

The files are read once per session, the first time the LCD is drawn. The log lists, for every car, the file that was loaded or not found, and for every file how many positions it moved.

### AlwaysShow

When enabled, the LCD is also shown on cockpit steering wheels that have no `LCD` object, in the same place as on the default steering wheel. When disabled (default), the LCD is only shown on steering wheels that have an `LCD` object.

### FontSize and FontWeight

The size and weight of the TrueType font GP4 uses for the LCD text, for all cars. They only have an effect when GP4's TrueType fonts option is enabled: by default, GP4 draws the LCD text with its own bitmap font, `digital.tga`, which has a fixed size and weight. `FontSize` is the size of the font, from 1 to 23 (GP4's default is 16; larger sizes crash GP4); `FontWeight` is its thickness, from 1 to 1000 (400 is normal, 700 is bold, GP4's default). Values outside these ranges are ignored and GP4's are used instead. GP4's defaults leave GP4's code untouched. To change the size of the text on the steering wheel, with either font, use [`Scale`](#Colour-Scale-Individual-and-Ini).

### Colour, Scale, Individual and Ini

The colour and size of the LCD text work like the [visor colours](Visor-Settings.md), and do not need any of the settings above to be enabled. `Colour` is the LCD text colour, in `0-255 BGR` format, for all cars (unlike the visors, there is no alpha: GP4 draws the LCD text opaque). The default, `0, 0, 0`, is GP4's own black. `Scale` multiplies the size of the LCD text, for all cars: `1.0` (default) is GP4's own size, `0.8` is 20% smaller, `2.0` is twice as large. Values of 0 or less are ignored and `1.0` is used instead. The defaults leave GP4's code untouched.

When `Individual` is enabled, each car can have its own colour and scale, in the INI file named by `Ini` (default `LCD.ini`), located in the same folder as the main `GP4PP.ini` file. It contains one section per car, named sequentially `CockpitLCD1` to `CockpitLCD22` (as `{driverindex}`), each with its own `Colour` and `Scale` entries. Cars without an entry use `Colour` and `Scale` from the `CockpitLCD` section. The colour and scale are the ones of the car whose cockpit is viewed.

Example - Yellow, 20% smaller LCD text for the first Ferrari, GP4's own for all the other cars

```ini
[CockpitLCD]
Individual = true
Colour = 0, 0, 0
Scale = 1.0
Ini = LCD.ini
```

```ini
[CockpitLCD1]
Colour = 15, 236, 250
Scale = 0.8
```

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
| Cockpit LCD positions | `fonts\{teamname}\{language}.pos` | `fonts\{teamname}_car{car}\{language}.pos` |

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

[CockpitLCD](#CockpitLCD) file names, relative to the `fonts` folder instead, can also use `{language}`, the language GP4 runs in (`English`, `French`, `German`, `Spanish`, `Portuguese` or `Italian`).

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
