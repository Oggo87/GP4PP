# General Settings

This section contains six entries:

* [Disable CD Check](#Disable-CD-Check)
* [Fix 3D Wheels](#Fix-3D-Wheels)
* [Track Folders](#Track-Folders)
* [Cockpit Visor](#Cockpit-Visor)
* [Steering Wheel Advanced Car Sahder](#Steering-Wheel-Advanced-Car-Shader)
* [Fix Projected Shadows](#Fix-Projected-Shadows)

```ini
[Settings]
DisableCDCheck = false
Fix3DWheels = false
Fix3DWheelsCockpitView = false
TrackFolders = false
CockpitVisor = false
StWheelAdvancedCarShader = false
FixProjectedShadows = false
```

## Disable CD Check

This utility setting disables the built-in check for an optical drive in GP4, allowing the game to run without requiring a CD/DVD drive or a mounted image.

## Fix 3D Wheels

These two utility settings fix the reversed tyre tread bug that occurs when using 3D rims in GP4. When enabled, the tyre treads will display correctly when `Rotate wheels via textures v2` is disabled in `f1graphics.cfg`. `Fix3DWheels` affects the external view, while `Fix3DWheelsCockpitView` affects the cockpit view. They can be enabled independently, allowing for different behaviour in the two views if desired.

Example - Fix 3D Wheels enabled for external view only

```ini
[Settings]
Fix3DWheels = true
Fix3DWheelsCockpitView = false
```

## Track Folders

This utility setting, in combination with `AutoName` found in each individual asset section (see [Assets Settings](Assets-Settings.md)), tells `GP4PP.dll` to look into track subfolders for those assets that are have `PerTrack` set to `true`.

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

## Cockpit Visor

This setting allows to enable using GP4's built-in visor shader in the cockpit mesh. In combination with the entries in the ```CockpitVisor``` section, it's possible to specify the ```ObjectName``` to apply the visor shader to, the ```Colour```,  and ```Transparency```. See [Visor Settings](Visor-Settings.md) for more details.

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

## Steering Wheel Advanced Car Shader

This setting allows to enable using GP4's built-in advanced car shader (the same one used for cars and helmets) for the in-cockpit steering wheel mesh, improving its appearance with better reflections and lighting effects.

Advanced Car Shader does not support alpha-blending, so if the steering wheel mesh uses any alpha-blending textures (for example, for cutouts or holes in the mesh), those parts will appear fully opaque when this setting is enabled. It is intended to be used with full 3D steering wheel meshes that do not rely on alpha-blending.

## Fix Projected Shadows

With projected shadows, GP4 draws each car's shadow into a shadow map that only covers 3.45 units around the car's origin. The shadow of a car that is significantly longer or larger than the original ones goes past the edge of the map and is cut off or looks stretched, especially at the tail and when the sun is low, as the shadow gets longer.

When this setting is enabled, the shadow map covers twice the area, 6.9 units around the car, and the shadow drawn on the track is twice as large to match, so the whole shadow of a long car fits. As the shadow map keeps its size in texels, the projected shadows are also somewhat softer.

Only the projected shadows change. The static shadows, also used by projected shadows for cars far away from the camera, keep their original size.
