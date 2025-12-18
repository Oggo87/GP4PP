# Getting Started

## Dependencies

* [GP4MemLib](https://github.com/Oggo87/GP4MemLib)
* [IniLib](https://github.com/Oggo87/IniLib)

## Prerequisites

* Grand Prix 4
* Carset Manager (CSM) by ZaZ (recommended) or)
* GP4 Memory Access by Carl_gpgames (recommended) or
* CheatEngine or
* Any other DLL injection tool

## Installing

### Carset Manager (CSM)

In the `Files` section of the CSM file, add the subpath to the DLL. Make sure that `GP4PP.ini` is located in the same folder as the DLL.

Example - `GP4PP.dll` and `MyOtherDLL.dll` located in the `Files` folder inside the main mod data folder

```ini
[Files]
InjectDll = ""%DataPath%Files\GP4PP.dll","%DataPath%Files\MyOtherDLL.dll""
```
### GP4 Memory Access

Under the *DLLs* tab, add an entry pointing to `GP4PP.dll`. Make sure that `GP4PP.ini` is located in the same folder as the DLL.