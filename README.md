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

## Runtime verification

Set `bDebugLogging = true` in `Fallout4RichPresenceCustom.toml`, then inspect
`Documents/My Games/Fallout4/F4SE/Fallout4RichPresence.log`.

1. Cold-launch to the title screen. Every sample before and at the main menu must show `sessionActive=false`, and `presence=in_game` must never appear.
2. Load a save, then quit back to the title screen. A successful load must change `sessionActive` to `true`; the first observed main-menu sample must change it back to `false`, with no previous quest or location published afterward.
3. Enter and leave combat. `combatStable` must change only after two consecutive equal `combatRaw` samples.
4. Use an interior door that closes within one sample. It must log `holding=true` without publishing the loading activity; a loading menu must persist for two samples before becoming visible.
5. For chargen settling only, also set `bShowPlayerName = true` and `iSamplingIntervalMs = 50`. After the Looks menu closes, `nameTrusted` must remain `false` for at least 200 ms before becoming `true`.

Restore the 500 ms sampling interval and disable debug logging after testing.

## Discord asset checklist

Upload Rich Presence art to the Discord application under these exact lowercase keys:

- [ ] `fallout4` — Fallout 4 key art or logo for normal gameplay.
- [ ] `main_menu` — Fallout 4 title-screen art for the main menu.
- [ ] `loading` — Vault-Tec loading-screen art for load transitions.
- [ ] `character_creation` — Vault 111 character-creation art for the Looks menu.
- [ ] `player` — A neutral Vault Boy portrait for player information.
- [ ] `combat` — A clear combat or crosshair icon for the in-combat modifier.

## License

GPL-3.0. See [LICENSE](LICENSE).
