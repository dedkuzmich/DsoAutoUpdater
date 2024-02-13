# DsoAutoUpdater 
DsoAutoUpdater is an auto updater for DS-Overlay cheat for War Thunder. It works this way:
* Obtains a list of offsets and WT versions from [repository](https://github.com/Hxnter999/ThunderDumps).
* Determines WT version that the cheat was developed for.
* Obtains the latest version of WT and its offsets.
* Updates the offsets values presented in binary to their up-to-date versions.


## Compilation with dynamic linking
1. Install dynamic libraries via vcpkg:
```shell
$ vcpkg install nlohmann-json:x64-windows
$ vcpkg install curl:x64-windows
```
The output will probably be very large

2. Build you project as always (don't forget to change your Solution Platform to x64).


## Compilation with static linking
1. Install static libraries via vcpkg:
```shell
$ vcpkg install nlohmann-json:x64-windows-static
$ vcpkg install curl:x64-windows-static
```

2. Change these settings (Project -> Properties):
 
2.1. Configuration Properties -> C/C++ -> Code Generation
| Configuration    | Release |
| ---------------- | ------- |
| Runtime Library  | /MT     |

2.2. Configuration Properties -> C/C++ -> Code Generation
| Configuration    | Debug   |
| ---------------- | ------- |
| Runtime Library  | /MTd    |

2.3. Configuration Properties -> Linker -> Input
| Configuration    	       | All Configurations   		            |
| ------------------------ | ---------------------------------- |
| Additional dependencies  | Ws2_32.Lib;Crypt32.Lib;Wldap32.Lib |			

2.4. Configuration Properties -> vcpkg
| Configuration | All Configurations |
| ------------- | -------------------|
| Triplet       | x64-windows-static |	

3. Apply changes.

4. Build you project as always.


## Possible remedy
1. Guides:
* [YouTube guide](https://www.youtube.com/watch?v=9TNPhanYbrA) about cvpkg installation and integration with MSVC.
* [Text guide](https://levelup.gitconnected.com/how-to-statically-link-c-libraries-with-vcpkg-visual-studio-2019-435c2d4ace03)
* [Official guide](https://devblogs.microsoft.com/cppblog/vcpkg-updates-static-linking-is-now-available/) about static linking.

2. Add VcpkgTriplet's to "Globals" section in project file `DsoAutoUpdater.vcxproj`:
```xml
<PropertyGroup Label="Globals">
 <!-- .... -->
 <VcpkgTriplet Condition="'$(Platform)'=='Win32'">x86-windows-static</VcpkgTriplet>
 <VcpkgTriplet Condition="'$(Platform)'=='x64'">x64-windows-static</VcpkgTriplet>
</PropertyGroup>
```

3. Configuration Properties -> vcpkg

| Configuration        | All Configurations |
| -------------------- | ------------------ |
| Use Static Libraries | Yes 		             |
