# Pitcrew Settings

This section contains entries to enable high resolution pitcrew meshes and per-team pitcrew meshes.

```ini
[Pitcrews]
HiResMeshes = false
IndividualTextures = false
IndividualMeshes = false
CustomD3DBufferSize = 0x400000
```
## Hi-Resolution Meshes

This setting allows to enable loading high resolution pitcrew meshes, overcoming the original GP4 limitation of 400 vertices per pitcrew mesh. The entry can be set to `true` / `false` or `1` / `0`.

## Individual Textures

This setting allows to enable per-driver textures (==**for engineers only**==).  The entry can be set to `true` / `false` or `1` / `0`.

When enabled, the `animsetup.set` and `animsetupmonaco.set` files located in the `qualifying` folder of `anim.wad`, need to be modified to include 23 entries for the textures, one per driver, instead of the original one per team, plus the Marshall texture. Failure to do so will result in crashes or other issues.

Example - Original vs Modified `animsetup.set` and `animsetupmonaco.set` for the `qualifying` folder

<table>
<tr>
<th>Original</th>
<th>Modified</th>
</tr>
<tr>
<td>

```
12
skins\engin_ferrari.tga
skins\engin_mclaren.tga
skins\engin_williams.tga
skins\engin_benetton.tga
skins\engin_bar.tga
skins\engin_jordan.tga
skins\engin_arrows.tga
skins\engin_sauber.tga
skins\engin_jaguar.tga
skins\engin_minardi.tga
skins\engin_prost.tga
skins\Marshall.tga
```
</td>
<td>

```
23
skins\engin_ferrari_1.tga
skins\engin_ferrari_2.tga
skins\engin_mclaren_1.tga
skins\engin_mclaren_2.tga
skins\engin_williams_1.tga
skins\engin_williams_2.tga
skins\engin_benetton_1.tga
skins\engin_benetton_2.tga
skins\engin_bar_1.tga
skins\engin_bar_2.tga
skins\engin_jordan_1.tga
skins\engin_jordan_2.tga
skins\engin_arrows_1.tga
skins\engin_arrows_2.tga
skins\engin_sauber_1.tga
skins\engin_sauber_2.tga
skins\engin_jaguar_1.tga
skins\engin_jaguar_2.tga
skins\engin_minardi_1.tga
skins\engin_minardi_2.tga
skins\engin_prost_1.tga
skins\engin_prost_2.tga
skins\Marshall.tga
```
</td>
</tr>
</table>

## Individual Meshes

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

## Custom D3D Buffer Size

This setting allows to specify a custom size (in either decimal or hexadecimal format) for the Direct3D buffer used to load pitcrew meshes. By default, GP4 uses a 0x4000 (16384 bytes) buffer, which works fine for the original low-poly pitcrew meshes, but not enough for high resolution pitcrew meshes. Some instability issues may arise if the buffer size is too large, hence the default setting in `GP4PP.ini` is set to 0x400000 (4 megabytes), which should be enough for most high resolution pitcrew meshes. If `HiResMeshes` is set to `false`, this setting is ignored and GP4's default buffer size is used.
