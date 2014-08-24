Klawr (WIP)
=======

A set of Unreal Engine 4 plugins that enable the use of C# and other CLI languages such as F# in game projects targeting the Windows platform. The aim of this project is to make it possible to do anything that can currently be done in Blueprints in C#. The minimum required Unreal Engine version is **4.4.0 (preview)**.

Overview
======
**KlawrCodeGeneratorPlugin** generates C# wrappers for UObject subclasses from the reflection information gathered by  **Unreal Header Tool** from UFUNCTION and UPROPERTY decorators in the engine/game source. This is the same reflection information that underpins much of the functionality provided by Blueprints.

**ScriptPlugin** has been patched to execute user scripts written in C#. Execution of C# scripts is made possible by an embedded CLR host in ScriptPlugin. For the time being I'm piggy-backing on ScriptPlugin, but eventually this code will be split out into a separate plugin to ease deployment.

Building
======

The build process is a bit convoluted at the moment, the goal is to simplify it once the project reaches a semi-functional state. Meanwhile here are some notes:

ScriptPlugin and ScriptEditorPlugin will only be built with Klawr support enabled if the directory **Engine\Source\ThirdParty\Klawr** exists at the time of the build (this is the expected location of the Klawr CLR host library projects).

The Unreal Header Tool project can be configured to build KlawrCodeGeneratorPlugin automatically, to do so specify **"KlawrCodeGeneratorPlugin"** as one of the **AdditionalPlugins** in **UnrealHeaderTool.Target.cs**

To get Unreal Header Tool to actually use KlawrCodeGeneratorPlugin you have to add/edit the **Plugins** section in **Engine\Programs\UnrealHeaderTool\Saved\Config\Windows\Engine.ini**:
```
[Plugins]
ProgramEnabledPlugins=KlawrCodeGeneratorPlugin
```

License
=====
Klawr is licensed under the MIT license.

The pugixml library is used by Klawr to parse XML, pugixml was written by Arseny Kapoulkine and is licensed under the MIT license.
