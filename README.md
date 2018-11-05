# DevKitchen 2018

This repository contains the examples shown at the DevKitchen 2018 occurred in Frankfurt during the SuperMeet 2018 on 1st and 2nd of November 2018.
More information about the presentation are available on [DevKitchen 2018 Aftermath](https://developers.maxon.net/?p=3319).

### Plugin Migration

The folder **plugin_migration** includes code to demonstrate the plugin migration from R19 (ObjectData_RuledMesh which can be found in the SDK) to R20 and both the initial and final state are included. The code shows, but not limited to, how enums have changed, proper use of error handling and proper formatting.

### Python

The folder **python_script** includes all the resources to demonstrate how to deliver debugging in VisualStudio Code and c4dpy.

The other folders (**devkitchen** / **frameworks** / **plugins**) demonstrate how to expose new MAXON API C++ functionalities to python. In order to make use of the code note that:
 * devkitchen folder should go on `{C4D Pref}/python27/`.
 * frameworks/devkitchen.framework should go on Frameworks folder unzipped from the sdk.zip provided with the installation.
 * plugins/devkitchen_project and plugins/devkitchen_python_example should go on plugins folder unzipped from the sdk.zip provided with the installation.
 * python_script/use_devkitchen_frameworks.py demonstrates how to use the framework previously compiled.

 ### R20 Features
 
 The folder **r20_features** includes all the resources (scenes and sources) to demonstrate how to use new API-related features of R20 about Volumes, Fields, and Multi-Instances.
