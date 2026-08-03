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
Run the format-template tests with `xmake build FormatTemplateTests` followed by
`xmake run FormatTemplateTests`.

## Packaging

`xmake package` assembles the installable mod layout into `dist/` — the same tree the CI
artifact ships:

```
dist/
  F4SE/Plugins/Fallout4RichPresence.{dll,pdb,toml}
  presets/{Default,Full,Minimal,SpoilerFree}.toml
  fomod/{ModuleConfig.xml,info.xml}
```

Point a mod manager at `dist/` as a mod folder, or zip it for release. Repackaging overwrites
only the files above, so a `Fallout4RichPresenceCustom.toml` you keep there survives.

## Installing

`xmake install` copies the plugin into `F4SE/Plugins` under the path given by one of these
environment variables, whichever is set first:

| Variable | Meaning |
| --- | --- |
| `XSE_FO4_MODS_PATH` | A mod manager's mods directory. The plugin installs into its own mod folder. |
| `XSE_FO4_GAME_PATH` | The Fallout 4 install directory. The plugin installs into `Data`. |

Release archives include a FOMOD installer. Install the archive with a mod manager and choose
exactly one configuration preset. For a manual installation, copy the DLL and one file from
`presets` to `Data/F4SE/Plugins`, renaming the preset to `Fallout4RichPresence.toml`.

## Configuration

| Section | Key | Default | Purpose |
| --- | --- | --- | --- |
| General | `iSamplingIntervalMs` | `500` | Milliseconds between game-state samples. |
| General | `bDebugLogging` | `false` | Enables diagnostic logging. |
| Privacy | `bShowPlayerName` | `false` | Makes `{name}` available to templates. |
| Privacy | `bShowQuest` | `true` | Makes `{quest}` and `{objective}` available. |
| Privacy | `bShowLocation` | `true` | Makes `{worldspace}` available and permits location data. |
| Privacy | `bShowExactLocation` | `true` | Makes `{location}` available when location data is permitted. |
| Discord | `sApplicationID` | `"1533687297684537374"` | Discord application ID. |
| Assets | `sAssetDefault` | `"fallout4"` | Large image during normal gameplay. |
| Assets | `sAssetMainMenu` | `"mainmenu"` | Large image at the main menu. |
| Assets | `sAssetLoading` | `"fallout4"` | Large image while loading. |
| Assets | `sAssetCharacterCreation` | `"fallout4"` | Large image during character creation. |
| Assets | `sAssetPlayer` | `"vaultboy"` | Small image beside player information. |
| Assets | `sAssetCombat` | `"vaultboy"` | Small image while in combat. |

An asset key may be empty to show no image for that slot. A small image identical to the large
image is suppressed, so a single uploaded asset renders one icon rather than a duplicated badge;
upload distinct art and the badge appears with no configuration change.
| Format | `sDetails` | `"{quest}"` | In-game details line. |
| Format | `sState` | `"{location} - {worldspace}"` | In-game state line. |
| Format | `sLargeText` | `"{objective}"` | In-game large-image tooltip. |
| Format | `sSmallText` | `"{name} - Level {level}"` | Normal in-game small-image tooltip. |
| Format | `sCombatSmallText` | `"{state}"` | Combat small-image tooltip. |

Override `sApplicationID` only to point the mod at your own registered Discord application, for
example to ship different artwork with a modlist.
`Fallout4RichPresence.toml` is replaced on reinstall, so put personal overrides in
`Fallout4RichPresenceCustom.toml` next to it. Keys omitted there inherit the selected preset.

### Presets

| Preset | Gameplay text |
| --- | --- |
| Default | Quest, objective, location, worldspace, and level; player name hidden. |
| Spoiler-free | Worldspace and level; quest, objective, exact location, and player name hidden. |
| Full | Quest, objective, location, worldspace, player name, and level. |
| Minimal | Level only. |

Each preset is a complete base configuration, and the installer never includes the custom file.
If a mod manager replaces whole mod directories on reinstall, keep the custom file in a separate
higher-priority mod.

### Format templates

The in-game format keys accept `{name}`, `{level}`, `{quest}`, `{objective}`, `{location}`,
`{worldspace}`, and `{state}`. `{state}` resolves to `In Game` or `In Combat`. Main-menu,
loading, and character-creation labels remain fixed.

Hidden or unavailable values resolve to empty. An empty token joins the separator runs on either
side into one boundary, then whitespace is collapsed without splitting UTF-8 code points. If
every token is empty, the field is empty; templates without tokens remain constant text after
whitespace normalization. For example,
`{quest} - {objective} - {location}` becomes `Reunions - Diamond City` when the objective is
missing. Sources over 512 bytes, unknown tokens, and unbalanced braces fall back to that key's
compiled-in default.

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

Upload one Rich Presence image under the key `fallout4`; every asset slot uses it by default.
Custom artwork can use a different configured key for each slot. Keys must contain 1-32 lowercase
ASCII letters, digits, or underscores. Invalid configured keys fall back to `fallout4`.

## License

GPL-3.0. See [LICENSE](LICENSE).
