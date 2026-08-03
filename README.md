# Fallout 4 Rich Presence

Discord Rich Presence for Fallout 4, as an F4SE plugin.

## Requirements

- [Fallout 4 Script Extender (F4SE)](https://f4se.silverlock.org/)
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327) — required at runtime
- [xmake](https://xmake.io/) 3.0 or newer
- Visual Studio 2022 with the Desktop development with C++ workload

Supported runtimes: 1.10.163, 1.10.984, 1.11.221.

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

Otherwise, copy the DLL and `data/F4SE/Plugins/Fallout4RichPresence.toml` to
`Data/F4SE/Plugins` yourself.

## Configuration

The shipped defaults are:

```toml
[General]
iSamplingIntervalMs = 500
bDebugLogging = false

[Privacy]
bShowPlayerName = false
bShowQuest = true
bShowLocation = true

[Discord]
sApplicationID = "0"
```

`sApplicationID` must be replaced with the application ID of a registered Discord application before use.
`Fallout4RichPresence.toml` is replaced on reinstall, so put personal overrides in
`Fallout4RichPresenceCustom.toml` next to it. Keys omitted there inherit the shipped value.

The Discord worker is intentionally leaked until process exit because F4SE provides no safe plugin shutdown callback.

## License

GPL-3.0. See [LICENSE](LICENSE).
