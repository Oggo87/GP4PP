# Version History

* 2.3.1
    * Added GP4 Tweaker compatibility for the 3D external steering wheels: when GP4 Tweaker's `SteerPos` tweak is enabled, they use its per-car steering wheel positions. See [Steering Wheels Settings](Steering-Wheels-Settings.md#UseCockpitMesh) for more details

* 2.3
    * Added option to fix the projected shadows of cars significantly longer or larger than the original ones, which were cut off at the edge of the shadow map. See [General Settings](General-Settings.md#Fix-Projected-Shadows) for more details
    * Added support for per-team and per-car steering wheels, both the external steering wheel texture and the in-cockpit steering wheel mesh, as well as cockpit dashes. See [Steering Wheels Settings](Steering-Wheels-Settings.md) for more details. Concept and initial reverse engineering from the ZaZ GP4 Tools proof of concept by [Ronald 'ZaZ' Plukker](https://github.com/RonaldZaZ)
    * Added option to draw the external steering wheels in 3D, using each car's cockpit steering wheel mesh. See [Steering Wheels Settings](Steering-Wheels-Settings.md#UseCockpitMesh) for more details. GP4's gloves and arms are kept on the 3D external steering wheels
    * Added option to show the fuel laps of every car in a race, in the cockpit and on the 3D external steering wheels. See [General Settings](General-Settings.md#Show-Opponents-Fuel-Laps) for more details

* 2.2.2
    * Added support for forcing the z-buffer format used by D3D directly from within GP4

* 2.2.1
    * Added support for overriding brake light behaviour from GP4 Tweaker. See [Tweaker Settings](Tweaker-Settings.md) for more details.
    * Decoupled the reveresed tread mapping fix between external and cockpit views. Credits to [node91](https://github.com/node91)

* 2.2
    * Added Rear Light Settings to control rear light behaviour in wet weather, when braking, when pit limiter is active and when charging ERS
    * Added back-end support for reading car's dynamic data in real time. Credits to [René 'SDI' Smit](https://github.com/sdidit) and [Paulo 'Prblanco' Blanco](https://github.com/Prblanco) for their extensive work
    * Added back-end support for reading GP4 Tweaker's status and its tweaks

* 2.1
    * Added support for per-driver pitcrew textures ***for engineers only***
    * Added settings for external cockpit and helmet visor colours and transparency
    * Added settings for individual colour and transparency per car for cockpit and helmet visors
    * Some bug fixes and improvements

* 2.0
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
