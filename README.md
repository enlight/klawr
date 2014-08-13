klawr (WIP)
=======

A set of Unreal Engine 4 plugins that enable the use of C# (and other CLI languages) in game projects targeting the Windows platform. Minimum required Unreal Engine version is **4.4.0 (preview)**.

Overview
======
ScriptGeneratorPlugin has been patched to generate C# wrappers for UObject subclasses. ScriptPlugin has been patched to execute user scripts written in C#. Execution of C# scripts is made possible by an embedded CLR host in ScriptPlugin. The patching is a temporary workaround for the lack of extensibility in the current iteration of ScriptGeneratorPlugin and ScriptPlugin.

License
=====
Klawr is licensed under the MIT license.

The pugixml library is used by Klawr to parse XML, pugixml was written by Arseny Kapoulkine and is licensed under the MIT license.
