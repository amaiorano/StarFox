# StarFox

A StarFox-like game written in C++.

I put this project together mostly because I wanted to test out some ideas I had for building a Unity3D-inspired object/component model and scene graph.

![Screenshot](https://raw.githubusercontent.com/wiki/amaiorano/StarFox/images/StarFox-1.png)

# Building the Code

This project is developed on Windows, but should be easy enough to port to other platforms.

**Install:**

- Visual Studio 2013
  - Get Community Edition for free: https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx

- Autodesk FDB SDK 2015.1 (vs2013)
  - Download from: http://images.autodesk.com/adsk/files/fbx20151_fbxsdk_vs2013_win.exe
  - When you build the solution, fbxsdk.props looks for the SDK in the default install folder "C:\Program Files\Autodesk\FBX\FBX SDK\2015.1", but you can override this by setting an environment variable named FBX_SDK_ROOT to your installed path. You can also try using this if you installed a newer version, although the code may not compile against a newer version.

**To Build:**

Open ```source\StarFox.sln``` in Visual Studio 2013 and press F5 to build and launch the game.

Note that the binaries for the game end up in ```source\_output\[Debug|Release]```, but the game data is in ```source\starfoxgame\data```. Visual Studio launches the exe in the output directory, but uses the project directory (where the vcxproj is) as it's current working directory.

If you wish to package the binaries and data together, use the ```source\package.bat``` script in the root directory. This will copy everything to ```source\_package\StarFox```.
