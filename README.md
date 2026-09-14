# Fallout 4 Rich Presence

Discord Rich Presence for Fallout 4, as an F4SE plugin.

## Requirements

- [Fallout 4 Script Extender (F4SE)](https://f4se.silverlock.org/)
- [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327) — required at runtime
- [xmake](https://xmake.io/) 3.0 or newer
- Visual Studio 2022 with the Desktop development with C++ workload

[DearModdingUI](https://github.com/Dear-Modding-FO4/DearModdingUI) is an optional soft dependency
that provides in-game status and settings pages. The host must include field-feedback support from
host commit `d034b47` / API commit `c8dc8fb`; the published 0.1.2 host predates that addition.
Rich Presence works normally when DearModdingUI is absent or older.

Supported runtimes: 1.10.163, 1.10.984, 1.11.221, 1.11.240.

## Building

```
git clone --recurse-submodules https://github.com/northaxosky/fallout4-rich-presence
cd fallout4-rich-presence
xmake config --mode=releasedbg
xmake build
```

The plugin builds to `build/windows/x64/releasedbg/Fallout4RichPresence.dll`, with its matching PDB.
Run the unit tests with
`xmake build FormatTemplateTests MarkerAssetTests StateBadgeTests MenuActivityTests HostPageTests ConfigFeedbackTests`,
followed by `xmake run` for each target.

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
| Privacy | `bShowMenuActivity` | `true` | Makes `{activity}` available and replaces details with the active menu activity. |
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
| Labels | `sLabelMainMenu` | `"Main Menu"` | Main-menu details. |
| Labels | `sLabelLoading` | `"Loading"` | Loading details. |
| Labels | `sLabelCharacterCreation` | `"Character Creation"` | Character-creation details. |
| Labels | `sLabelGameTitle` | `"Fallout 4"` | Large-image tooltip for fixed states. |
| Labels | `sLabelInGame` | `"In Game"` | Normal gameplay label. |
| Labels | `sLabelInCombat` | `"In Combat"` | Combat label and empty combat-tooltip fallback. |
| Labels | `sLabelInPowerArmor` | `"In Power Armor"` | Power-armor label. |
| Labels | `sLabelIrradiated` | `"Irradiated"` | Irradiated label. |
| Labels | `sLabelLevel` | `"Level {level}"` | In-game fallback when details and state are empty. |
| Labels | `sLabelBarter` | `"Trading"` | Barter activity without a vendor name. |
| Labels | `sLabelBarterNamed` | `"Trading with {name}"` | Barter activity with a vendor name. |
| Labels | `sLabelWorkbench` | `"Using a Workbench"` | Workbench activity without a furniture name. |
| Labels | `sLabelWorkbenchNamed` | `"Using the {name}"` | Workbench activity with a furniture name. |
| Labels | `sLabelWorkshop` | `"Building"` | Workshop building activity. |
| Labels | `sLabelTerminal` | `"Using a Terminal"` | Terminal activity. |
| Labels | `sLabelLockpicking` | `"Lockpicking"` | Lockpicking activity. |
| Labels | `sLabelSitWait` | `"Waiting"` | Sit/wait activity. |
| Labels | `sLabelDialogue` | `"Talking"` | Dialogue activity. |

An asset key may be empty to show no image for that slot. A small image identical to the large
image is suppressed, so a single uploaded asset renders one icon rather than a duplicated badge;
upload distinct art and the badge appears with no configuration change.

State badges use the first active state in this order: combat, power armor, then irradiated.
Each state must be observed for two consecutive samples before it changes. Disabling
`bStateBadge` restores the original combat-or-player badge behavior.
Combat targets are also debounced by actor identity: a target must be observed twice
consecutively before its name changes, and the previous name remains while a new target settles.

Menu activity reports barter, workbenches, workshop building, terminals, lockpicking, waiting, and
dialogue. Barter and workbench activity uses the vendor or occupied furniture name when a safe base
form name is available. Terminals are deliberately generic because the game exposes no safe terminal
name source. Pip-Boy and its sub-menus, containers, and standalone inventory examination are excluded
to avoid presence churn; workbenches are identified from the player's occupied furniture rather than
the examine menu. Activity and name changes must be observed for two consecutive samples; leaving an
activity clears it immediately. When `bShowMenuActivity` is true, active menu text replaces the
details line after normal template rendering. Other presence fields are unchanged.

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
The supported localisation route is to ship translated `[Labels]` values in a
`Fallout4RichPresenceCustom.toml`; empty labels intentionally render no text.

### In-game settings

With a compatible DearModdingUI host installed, the plugin contributes a **Rich Presence** entry
with an explicitly registered **General** category containing **Home** and **Settings** pages.
The heading remains visible even though it is the only category. **Home** shows plugin and runtime
facts, connection health, the latest generated presence, cached marker count, detected plugin
conflicts, and a host-rendered GitHub link and FAQ. The GitHub link opens the browser; the presence
preview can precede Discord updates while the transport is disconnected or rate-limited.
**Settings** edits the same options in game, with Status at the top. Sampling, privacy, format,
asset, and logging changes apply immediately; changing `sApplicationID` requires restarting
Fallout 4 because the Discord worker captures it at startup. Inline feedback identifies malformed
templates, invalid asset keys or application IDs, and out-of-range numeric values. Invalid drafts remain editable while the
last valid value stays active; **Apply** rejects them without rewriting the draft or custom file.
A valid application ID that differs from the startup value shows a restart notice even after it
is saved, and the notice clears only when the startup value is restored or the game restarts.

This integration uses DearModdingUI's stable `dmui::ui` contract with explicit categories,
host-owned icon resolution, external links, and host-rendered field feedback
(`DearModdingUI-API` revision `c8dc8fb`). The client negotiates the required stable UI prefix,
host services, and the appended field-feedback operations before registering pages. If the host
is absent or incompatible, Discord presence and TOML configuration continue without the UI.

Run the navigation and field-feedback compatibility tests with `xmake build HostPageTests` followed by
`xmake run HostPageTests`. They do not require a running game or a loaded UI host.

Use **Apply** to persist edits. The page writes only values that differ from the installed preset
to `Fallout4RichPresenceCustom.toml`, so they survive reinstalling the mod. **Reset all** restores
the installed preset and saves the result.

### Presets

| Preset | Gameplay text |
| --- | --- |
| Default | Quest, objective, location, worldspace, and level; player name hidden. |
| Spoiler-free | Worldspace and level; quest, objective, exact location, player name, combat target, menu activity, and marker artwork hidden. |
| Full | Quest, objective, location, worldspace, player name, and level. |
| Minimal | Level only; every other source hidden, including combat target, menu activity, and marker artwork. |

Each preset is a complete base configuration, and the installer never includes the custom file.
If a mod manager replaces whole mod directories on reinstall, keep the custom file in a separate
higher-priority mod.

### Format templates

The in-game format keys accept `{name}`, `{level}`, `{quest}`, `{objective}`, `{location}`,
`{worldspace}`, `{state}`, `{target}`, and `{activity}`. `{state}` resolves to `In Game`, `In Combat`,
`In Power Armor`, or `Irradiated` according to the active badge. `{target}` resolves to the
debounced combat target name and is empty outside combat, when no safe name is available, or
when `bShowCombatTarget` is false. `{activity}` resolves to the debounced menu activity text and is
empty outside a reported menu or when `bShowMenuActivity` is false. The named barter and workbench
labels are templates whose `{name}` token is resolved in a separate activity-label render context,
not from the player name. These state labels and the fixed-state text come from
`[Labels]`; `sLabelLevel` is also a template and accepts `{level}`.

Hidden or unavailable values resolve to empty. An empty token joins the separator runs on either
side into one boundary, then whitespace is collapsed without splitting UTF-8 code points. If
every token is empty, the field is empty; templates without tokens remain constant text after
whitespace normalization. For example,
`{quest} - {objective} - {location}` becomes `Reunions - Diamond City` when the objective is
missing. Sources over 512 bytes, unknown tokens, and unbalanced braces fall back to that key's installed
preset (or compiled-in default if the preset is also invalid) during startup loading. In-game
editing instead preserves the invalid draft, reports it inline, and keeps publishing the last
valid compiled template until corrected.

The combat tooltip is the one field with a fallback: when `sCombatSmallText` renders empty,
because no target name is available, the badge is labelled `In Combat` rather than left untitled.

The Discord worker is intentionally leaked until process exit because F4SE provides no safe plugin shutdown callback.

## Runtime verification

Set `bDebugLogging = true` in `Fallout4RichPresenceCustom.toml`, then inspect
`Documents/My Games/Fallout4/F4SE/Fallout4RichPresence.log`.

At every plugin load, the INFO log starts with a compact diagnostics banner containing the plugin
and runtime versions, `Fallout4.exe` module base, Address Library status, and the resolved
`Main::OnIdle` absolute address and RVA. This remains enabled without debug logging so bug reports
contain enough information to interpret hook addresses.

If either `Discord_Presence_F4SE_Remake.dll` or the original
`Discord_Presence_F4SE.dll` is already loaded, the plugin warns that running both presence mods
can duplicate or flicker Discord activity. It continues loading; disable one of the two mods to
remove the conflict. DearModdingUI also shows the detected conflict in the Status group.

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
each slot. Keys must contain up to 32 lowercase ASCII letters, digits, or underscores; empty means
no image. Invalid keys use the installed-preset or compiled-in fallback during startup. In-game
editing preserves an invalid draft and the last valid active key until it is corrected.

A configured or mapped key that has not been uploaded renders blank because Discord does not
expose the application's asset inventory to the plugin.

For all curated map-marker artwork, upload images under these 74 keys:

`marker_airfield`, `marker_bos`, `marker_bottlingplant`, `marker_brownstone`,
`marker_bunker`, `marker_bunkerhill`, `marker_camper`, `marker_castle`, `marker_cave`,
`marker_church`, `marker_city`, `marker_constitution`, `marker_countryclub`,
`marker_customhouse`, `marker_diamondcity`, `marker_disciples`, `marker_drivein`,
`marker_encampment`, `marker_faneuilhall`, `marker_farm`, `marker_fillingstation`,
`marker_forest`, `marker_galactic`, `marker_goodneighbor`, `marker_graveyard`,
`marker_highway`, `marker_hospital`, `marker_industrial`, `marker_industrialdome`,
`marker_industrialstacks`, `marker_institute`, `marker_irishpride`, `marker_junkyard`,
`marker_kiddiekingdom`, `marker_landmark`, `marker_libertalia`, `marker_lowrise`,
`marker_mechanist`, `marker_metro`, `marker_militarybase`, `marker_minutemen`,
`marker_monorail`, `marker_observatory`, `marker_office`, `marker_pier`,
`marker_policestation`, `marker_potentialvassal`, `marker_prydwen`, `marker_quarry`,
`marker_radioactive`, `marker_radiotower`, `marker_raidersettlement`, `marker_railroad`,
`marker_rides`, `marker_ruinstown`, `marker_ruinsurban`, `marker_safari`, `marker_salem`,
`marker_sanctuary`, `marker_satellite`, `marker_school`, `marker_sentinel`,
`marker_settlement`, `marker_sewer`, `marker_shipwreck`, `marker_skyscraper`,
`marker_submarine`, `marker_swanpond`, `marker_synthhead`, `marker_town`,
`marker_vassalsettlement`, `marker_vault`, `marker_water`, `marker_wildwest`.

Unmapped markers use the configured default gameplay image. Every mapped key above must be
uploaded: Discord does not expose the application's asset inventory to the plugin, so a missing
remote asset cannot be detected for client-side fallback and Discord renders no marker image.

## License

GPL-3.0. See [LICENSE](LICENSE).
