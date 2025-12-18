# Visor Settings

These sections contain entries to set the colour and transparency of both helmet and cockpit visors. `CockpitVisor` settings are applied to the in-cockpit visor when the corresponding setting in the `Settings` section is enabled, `ZCockpitVisor` and `HelmetVisor` settings are always applied to `z_cockpit_visor` objects in the car mesh and helmet visors respectively. These sections share basically the same structure and options.

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

## ObjectName

Available only for the `CockpitVisor` section, this entry allows to specify the name of the object in the cockpit mesh that will have the visor shader applied to it.

## Individual

Available for all sections, `Individual` allows to specify whether the settings are applied individually (when set to `true`/`1`), or globally for all cars (when set to `false`/`0`).

## Colour and Transparency

Available for all sections, it's possible to specify the default `Colour` (in `0-255 BGRA` format),  and `Transparency` (a `float` value). The Alpha component of the `Colour` and the `Transparency` value work combinedly to set the visor's transparency. These settings will be used for all cars when [`Individual`](#Individual) is set to `false` and will be used as default values when using [`Individual`](#Individual) is set to `true` and a car doesn't have specific entries in the specified [INI file](#Ini).

## Ini

Available for all sections, this entry allows to specify the name of an INI file that contains individual settings per car when `Individual` is set to `true`. The INI file needs to be located in the same folder as the main `GP4PP.ini` file and it needs to contain one section per car, named sequentially 1-22, with the same [`Colour` and `Transparency`](#Colour-and-Transparency) entries as above.

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