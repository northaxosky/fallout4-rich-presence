# Fallout 4 Rich Presence

Shows your quests, location, combat, and menu activity on Discord, with matching map-marker artwork.

## Install

Download the mod ZIP from [Releases](https://github.com/northaxosky/fallout4-rich-presence/releases)
and install with your mod manager. Choose **Default**, **Spoiler-free**, **Full**, or **Minimal**.
Keep the Discord desktop app running with activity sharing enabled.

Requires [F4SE](https://f4se.silverlock.org/) and the matching
[Address Library](https://www.nexusmods.com/fallout4/mods/47327).
Supported runtimes: **1.10.163, 1.10.984, 1.11.221, 1.11.240**.

[DearModdingUI](https://github.com/Dear-Modding-FO4/DearModdingUI) is optional for in-game
Home and Settings pages. Use a field-feedback-capable build (host `d034b47` or newer);
the original 0.1.2 release is too old. Discord presence works without it.

## Configure

Player names are hidden by default. Use **Spoiler-free** to hide quests, exact locations,
enemy names, menu activity, and location artwork; **Minimal** shares only your level.

Edit settings in DearModdingUI under **Rich Presence → General → Settings**, or put overrides in
`Data\F4SE\Plugins\Fallout4RichPresenceCustom.toml`. Omitted values inherit the
[installed preset](data/F4SE/Plugins/Fallout4RichPresence.toml).

Valid edits apply live; **Apply** saves them. Invalid drafts retain the last valid value.
Changing `sApplicationID` requires a restart. Keep custom overrides in a separate mod if your
mod manager replaces entire folders on reinstall.

Artwork is served by Discord. Players need no local images or developer application.

## Build and release

Clone with `--recurse-submodules`; use Visual Studio's Desktop C++ workload and xmake 3+.
For build-only work, unset `FO4_DEV_MODS`, `XSE_FO4_MODS_PATH`, and `XSE_FO4_GAME_PATH`
to prevent automatic deployment.

```powershell
xmake config --mode=releasedbg --toolchain=msvc
xmake build
```

Push a `vX.Y.Z` tag matching the version in `xmake.lua`. GitHub Actions builds the DLL/PDB,
runs all six test suites, and attaches the FOMOD ZIP to a GitHub Release.
Branch and pull-request builds produce Actions artifacts without publishing releases.

[GPL-3.0](LICENSE).
