# Command Deck
Accelerate every day game development workflows with your Elgato Stream Deck!

# Quick Start

## 1. Install the Unreal Engine Plugin

### A. Install using the Epic Games Launcher

_Choose this option if you have installed the Unreal Engine using the Epic Games Launcher. The launcher simplifies installation, and will notify you of updates._

1. Open the Epic Games Launcher, select `Unreal Engine` on the left, then navigate to the `Library` tab
2. Under `Fab Library`, find the `Command Deck` plugin, click the `Install to Engine` button, select an engine slot, then click `Install`
> 💡 If the `Command Deck` plugin is not listed in your Fab Library, download the plugin [here](https://www.fab.com/listings/c9cd0b52-299b-4e76-8c53-bba41f2d1eba), then refresh the Fab Library

### B. Install from GitHub

_Choose this option if you have installed the Unreal Engine from the Epic Games [UnrealEngine GitHub repository](https://github.com/epicgames)._

1. Create a "CommandDeck" folder for the plugin:
    - If installing the plugin as an engine plugin, create the folder under {ENGINE_ROOT}\Engine\Plugins 
        - Example: _C:\Epic Games\UE5.6\Engine\Plugins\CommandDeck_
    - If installing the plugin as a project plugin, create the folder under {PROJECT_ROOT}\Plugins
        - Example: _Documents\Unreal Projects\MyUnrealProject\Plugins\CommandDeck_
2. Download or clone the `Command Deck` repository with the branch that matches your version of the Unreal Engine to the newly-created `CommandDeck` folder
	- For example, the <code>5.3</code> branch targets Unreal Engine 5.3
> 💡 The only difference between branches is the UPLUGIN <code>EngineVersion</code> field, the code itself is identical across branches

## 2. Enable the Unreal Engine Plugin

1. Launch the Unreal Editor, then from the top menu select Edit > Plugins
2. Find `Command Deck` in the list of plugins, then enable it (indicated by the checkbox)

![Unreal Editor Plugin](Docs/.Assets.GetStarted/UnrealEditorPlugin.png)

> 💡 If your project is a `C++` project, the `Command Deck` plugin will not appear in the list of plugins until the project has been recompiled!

## 3. Install Stream Deck

1. Download and install the `Stream Deck` software:
    - [Windows](https://www.elgato.com/us/en/s/downloads)
    - [iOS](https://apps.apple.com/ca/app/elgato-stream-deck-mobile/id1440014184)
    - [Android](https://play.google.com/store/apps/details?id=com.corsair.android.streamdeck)

## 4. Install the Stream Deck Plugin

1. Navigate to the `Command Deck - Unreal` plugin on the [Elgato Marketplace](https://marketplace.elgato.com/product/command-deck-unreal-17d6cac4-0985-449f-a565-8b1ebca6df31) 
2. Click the `Get` button to download and install

Once the plugin has been installed, `Command Deck - Unreal` will appear in the list of plugins.

![Unreal Editor Plugin](Docs/.Assets.GetStarted/StreamDeckPluginList.png)

## 5. Start Creating!

You are now ready to start adding actions to your Stream Deck device!

Refer to the documentation and guides below to optimize your experience with Command Deck.

# Documentation

- [Get Connected](Docs/GetConnected.md)
- [Versioning](Docs/Versioning.md)
- [Activate World](Docs/ActivateWorld.md)
- [Understanding Action Titles](Docs/UnderstandingActionTitles.md)
- [Enable and Configure Logging](Docs/EnableAndConfigureLogging.md)

# Action Guides

Detailed user guides for each of the actions available in Command Deck.

- [Adjust Editor Volume](Docs/Actions/AdjustEditorVolume.md)
- [Execute Command](Docs/Actions/ExecuteCommand.md)
- [Generate VS Project Files](Docs/Actions/GenerateVsProjectFiles.md)
- [Kill Editor](Docs/Actions/KillEditor.md)
- [Play World](Docs/Actions/PlayWorld.md)
- [Trigger Blueprint](Docs/Actions/TriggerBlueprint.md)

# Changelog

See [CHANGELOG.md](CHANGELOG.md).