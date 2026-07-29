# Fallout 4 Rich Presence

Discord Rich Presence for Fallout 4, as an F4SE plugin.

## Requirements

- [Fallout 4 Script Extender (F4SE)](https://f4se.silverlock.org/)
- [xmake](https://xmake.io/) 3.0 or newer
- Visual Studio 2022 with the Desktop development with C++ workload

## Building

```
git clone --recurse-submodules https://github.com/northaxosky/fallout4-rich-presence
cd fallout4-rich-presence
xmake config --mode=release
xmake build
```

The plugin builds to `build/windows/x64/release/Fallout4RichPresence.dll`.

## Installing

`xmake install` copies the plugin into `F4SE/Plugins` under the path given by one of these
environment variables, whichever is set first:

| Variable | Meaning |
| --- | --- |
| `XSE_FO4_MODS_PATH` | A mod manager's mods directory. The plugin installs into its own mod folder. |
| `XSE_FO4_GAME_PATH` | The Fallout 4 install directory. The plugin installs into `Data`. |

Otherwise, copy the DLL to `Data/F4SE/Plugins` yourself.

## License

GPL-3.0. See [LICENSE](LICENSE).
