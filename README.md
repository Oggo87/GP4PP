# GP4++

A DLL to enhance and extend Grand Prix 4 functionality.

## Description

This DLL, formerly known as *Individual Meshes And Textures*, expands original Grand Prix 4 to allow full control of how meshes (front/rear wheels, helmets, cockpits, cars, collision meshes) and textures (cockpits, helmets) are loaded, supporting per-driver, per-team, and per-track custom loading. It includes features to manage LODs (Level of Detail), auto-naming conventions, and fallbacks to default GP4 assets when custom assets are unavailable.

Additionally, it includes and expands on features originally found in [*High Resolution Pitcrews for Grand Prix 4* (`HiResPitcrew.dll`)](https://github.com/Oggo87/HiResPitcrew), both overcoming the limitation of 400 vertices in the original game, allowing high resolution pitcrew meshes to be used, and introducing support for per-team pitcrew meshes and per-car engineer textures.

It also includes options to fully control cockpit and helmet visor colours and transparency, customise the rear light(s) behaviour and enable the use of GP4's built-in visor shader in the cockpit mesh.

It also supports per-team and per-car steering wheels and cockpit dashes. Each car's external steering wheel can also be drawn in 3D from its cockpit steering wheel mesh, with its own rev lights, flags, gear and dash displays (position, lap, lap time, fuel, speed and more), while keeping GP4's original gloves and arms.

Other features include:
* Fixing the reversed tyre tread bug when using 3D rims
* Fixing the projected shadows of cars significantly longer or larger than the original ones
* Showing every car's fuel laps in a race, not just the player's
* Forcing the z-buffer format used by Direct3D
* Overriding the brake light behaviour from GP4 Tweaker
* Disabling the built-in check for an optical drive

## Getting Started

See [Getting Started](docs/Getting-Started.md) for full details

## Usage

See [Usage](docs/Usage.md) for full details

## Authors

Diego "Öggo" Noriega

## Additional Credits

[René 'SDI' Smit](https://github.com/sdidit) and [Paulo 'Prblanco' Blanco](https://github.com/Prblanco) for their extensive work on car dynamic data

[Ronald 'ZaZ' Plukker](https://github.com/RonaldZaZ) for the steering wheels concept and initial reverse engineering from the ZaZ GP4 Tools proof of concept

## Version History

See [Change Log](docs/CHANGELOG.md)
