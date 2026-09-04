# Fallout 4 Rich Presence

Discord Rich Presence for Fallout 4, as an F4SE plugin.

## Requirements

- [Fallout 4 Script Extender (F4SE)](https://f4se.silverlock.org/)
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327) — required at runtime
- [xmake](https://xmake.io/) 3.0 or newer
- Visual Studio 2022 with the Desktop development with C++ workload

[DearModdingUI](https://github.com/Dear-Modding-FO4/DearModdingUI) is an optional soft
dependency that provides the in-game settings page. Rich Presence works normally when it is not
installed.

Supported runtimes: 1.10.163, 1.10.984, 1.11.221, 1.11.240.

## Building

```
git clone --recurse-submodules https://github.com/northaxosky/fallout4-rich-presence
cd fallout4-rich-presence
xmake config --mode=release
xmake build
```

The plugin builds to `build/windows/x64/release/Fallout4RichPresence.dll`.
Run the unit tests with `xmake build FormatTemplateTests MarkerAssetTests StateBadgeTests`,
followed by `xmake run FormatTemplateTests`, `xmake run MarkerAssetTests`, and
`xmake run StateBadgeTests`.

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

`xmake install` copies the plugin into `F4SE/Plugins` under the first variable that is set:

| Variable | Install root |
| --- | --- |
| `FO4_DEV_MODS` | A mods root, plus `Discord Rich Presence - Dev`. |
| `XSE_FO4_MODS_PATH` | A mods root, plus a folder named after the target. |
| `XSE_FO4_GAME_PATH` | The Fallout 4 install directory, plus `Data`. |

Release archives include a FOMOD installer. Install the archive with a mod manager and choose
exactly one configuration preset. For a manual installation, copy the DLL and one file from
`presets` to `Data/F4SE/Plugins`, renaming the preset to `Fallout4RichPresence.toml`.

## Configuration

| Section | Key | Default | Purpose |
| --- | --- | --- | --- |
| General | `iSamplingIntervalMs` | `500` | Milliseconds between game-state samples. |
| General | `iIrradiatedPercent` | `25` | Radiation percentage of the health pool at which the irradiated badge appears. |
| General | `bDebugLogging` | `false` | Enables diagnostic logging. |
| Privacy | `bShowPlayerName` | `false` | Makes `{name}` available to templates. |
| Privacy | `bShowQuest` | `true` | Makes `{quest}` and `{objective}` available. |
| Privacy | `bShowLocation` | `true` | Makes `{worldspace}` available and permits location data. |
| Privacy | `bShowExactLocation` | `true` | Makes `{location}` available when location data is permitted. |
| Privacy | `bShowCombatTarget` | `true` | Makes `{target}` available while in combat. |
| Discord | `sApplicationID` | `"1533687297684537374"` | Discord application ID. |
| Assets | `bMarkerArtwork` | `true` | Uses nearby discovered map-marker artwork during gameplay. |
| Assets | `bStateBadge` | `true` | Enables power-armor and irradiated state badges. |
| Assets | `iMarkerMaxDistance` | `16384` | Maximum game-unit distance for marker artwork and interior location fallback. |
| Assets | `sAssetDefault` | `"fallout4"` | Large image during normal gameplay. |
| Assets | `sAssetMainMenu` | `"mainmenu"` | Large image at the main menu. |
| Assets | `sAssetLoading` | `"fallout4"` | Large image while loading. |
| Assets | `sAssetCharacterCreation` | `"fallout4"` | Large image during character creation. |
| Assets | `sAssetPlayer` | `"vaultboy"` | Small image beside player information. |
| Assets | `sAssetCombat` | `"vaultboy"` | Small image while in combat. |
| Assets | `sAssetPowerArmor` | `"state_powerarmor"` | Small image while wearing power armor. |
| Assets | `sAssetIrradiated` | `"state_irradiated"` | Small image at or above the irradiated threshold. |
| Format | `sDetails` | `"{quest}"` | In-game details line. |
| Format | `sState` | `"{location} - {worldspace}"` | In-game state line. |
| Format | `sLargeText` | `"{objective}"` | In-game large-image tooltip. |
| Format | `sSmallText` | `"{name} - Level {level}"` | Normal in-game small-image tooltip. |
| Format | `sCombatSmallText` | `"Fighting {target}"` | Combat small-image tooltip. |

An asset key may be empty to show no image for that slot. A small image identical to the large
image is suppressed, so a single uploaded asset renders one icon rather than a duplicated badge;
upload distinct art and the badge appears with no configuration change.

State badges use the first active state in this order: combat, power armor, then irradiated.
Each state must be observed for two consecutive samples before it changes. Disabling
`bStateBadge` restores the original combat-or-player badge behavior.
Combat targets are also debounced by actor identity: a target must be observed twice
consecutively before its name changes, and the previous name remains while a new target settles.

During gameplay, marker artwork uses the nearest discovered Pip-Boy map marker within
`iMarkerMaxDistance`. It is disabled when either `bMarkerArtwork` or `bShowLocation` is false,
because the selected art communicates location. Marker types without curated art fall back to
`sAssetDefault`. Main-menu, loading, and character-creation art is
unchanged. When an interior has no authored location name, the location line can fall back to the
nearest discovered exterior marker name while preserving the existing exact-location privacy
setting.

Override `sApplicationID` only to point the mod at your own registered Discord application, for
example to ship different artwork with a modlist.
`Fallout4RichPresence.toml` is replaced on reinstall, so put personal overrides in
`Fallout4RichPresenceCustom.toml` next to it. Keys omitted there inherit the selected preset.
When no overrides remain, the custom file is kept with only a short header comment.

### In-game settings

With DearModdingUI installed, open its menu and select **Rich Presence > Settings** to edit the
same options in game and inspect the live Discord transport status. Sampling, privacy, format,
asset, and logging changes apply immediately; changing `sApplicationID` requires restarting
Fallout 4 because the Discord worker captures it at startup.

Use **Apply** to persist edits. The page writes only values that differ from the installed preset
to `Fallout4RichPresenceCustom.toml`, so they survive reinstalling the mod. **Reset all** restores
the installed preset and saves the result.

### Presets

| Preset | Gameplay text |
| --- | --- |
| Default | Quest, objective, location, worldspace, and level; player name hidden. |
| Spoiler-free | Worldspace and level; quest, objective, exact location, player name, combat target, and marker artwork hidden. |
| Full | Quest, objective, location, worldspace, player name, and level. |
| Minimal | Level only. |

Each preset is a complete base configuration, and the installer never includes the custom file.
If a mod manager replaces whole mod directories on reinstall, keep the custom file in a separate
higher-priority mod.

### Format templates

The in-game format keys accept `{name}`, `{level}`, `{quest}`, `{objective}`, `{location}`,
`{worldspace}`, `{state}`, and `{target}`. `{state}` resolves to `In Game`, `In Combat`,
`In Power Armor`, or `Irradiated` according to the active badge. `{target}` resolves to the
debounced combat target name and is empty outside combat, when no safe name is available, or
when `bShowCombatTarget` is false. Main-menu, loading, and character-creation labels remain fixed.

Hidden or unavailable values resolve to empty. An empty token joins the separator runs on either
side into one boundary, then whitespace is collapsed without splitting UTF-8 code points. If
every token is empty, the field is empty; templates without tokens remain constant text after
whitespace normalization. For example,
`{quest} - {objective} - {location}` becomes `Reunions - Diamond City` when the objective is
missing. Sources over 512 bytes, unknown tokens, and unbalanced braces fall back to that key's
compiled-in default.

The combat tooltip is the one field with a fallback: when `sCombatSmallText` renders empty,
because no target name is available, the badge is labelled `In Combat` rather than left untitled.

The Discord worker is intentionally leaked until process exit because F4SE provides no safe plugin shutdown callback.

## Runtime verification

Set `bDebugLogging = true` in `Fallout4RichPresenceCustom.toml`, then inspect
`Documents/My Games/Fallout4/F4SE/Fallout4RichPresence.log`.

1. Cold-launch to the title screen. Every sample before and at the main menu must show `sessionActive=false`, and `presence=in_game` must never appear.
2. Load a save, then quit back to the title screen. A successful load must change `sessionActive` to `true`; the first observed main-menu sample must change it back to `false`, with no previous quest or location published afterward.
3. Enter and leave combat. `combatStable` must change only after two consecutive equal `combatRaw`
   samples; a new `targetStableID` must likewise require two consecutive `targetRawID` samples.
4. Use an interior door that closes within one sample. It must log `holding=true` without publishing the loading activity; a loading menu must persist for two samples before becoming visible.
5. For chargen settling only, also set `bShowPlayerName = true` and `iSamplingIntervalMs = 50`. After the Looks menu closes, `nameTrusted` must remain `false` for at least 200 ms before becoming `true`.

Restore the 500 ms sampling interval and disable debug logging after testing.

## Discord asset checklist

Upload Rich Presence images under the shipped keys `fallout4`, `mainmenu`, `vaultboy`,
`state_powerarmor`, and `state_irradiated`. Custom artwork can use a different configured key for
each slot. Keys must contain 1-32 lowercase ASCII letters, digits, or underscores. Invalid
configured keys fall back to that slot's compiled-in key.

A configured or mapped key that has not been uploaded renders blank because Discord does not
expose the application's asset inventory to the plugin.

For all curated map-marker artwork, upload images under these keys:

`marker_bos`, `marker_bunkerhill`, `marker_castle`, `marker_cave`, `marker_church`,
`marker_city`, `marker_diamondcity`, `marker_drivein`, `marker_farm`,
`marker_fillingstation`, `marker_goodneighbor`, `marker_graveyard`, `marker_hospital`,
`marker_industrial`, `marker_institute`, `marker_junkyard`, `marker_metro`,
`marker_militarybase`, `marker_minutemen`, `marker_policestation`, `marker_prydwen`,
`marker_radioactive`, `marker_radiotower`, `marker_railroad`, `marker_sanctuary`,
`marker_school`, `marker_settlement`, `marker_vault`, and `marker_water`.

Unmapped markers use the configured default gameplay image. Every mapped key above must be
uploaded: Discord does not expose the application's asset inventory to the plugin, so a missing
remote asset cannot be detected for client-side fallback and Discord renders no marker image.

## License

GPL-3.0. See [LICENSE](LICENSE).
