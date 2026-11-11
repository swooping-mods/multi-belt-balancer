# Multi Belt Balancer

Configure 2x2, 3x3 and 4x4 native belt junctions; aligned 3x3 layouts display the supplied housing.

## Install

Install [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) first.

Download [multi-belt-balancer-0.2.0.msmod](content/multi-belt-balancer-0.2.0.msmod), import it with **Install mod package**, then select the mod and its required frameworks.

## Controls

- F5 selects 2x2/3x3/4x4; I selects aimed inputs, O selects outputs; $40/lane.
- Aligned 3x3 junctions display the supplied three-lane housing; gameplay qualification pending.

## Compatibility

Version 0.2.0. Targets the pinned Steam Windows x64 IL2CPP build of Moon Sweepers (Steam app 5335510). On Linux, the game runs through Proton. Experimental: back up saves before using save-affecting mods.

## Build

Run `python3 build.py --check` to verify source hashes, then `python3 build.py` to write a build under `.local/build/`. Requires installed Clang, lld-link and llvm-dlltool.

## Credits

See [CREDITS.txt](CREDITS.txt) and [third-party notices](vendored/THIRD_PARTY_NOTICES.md).
