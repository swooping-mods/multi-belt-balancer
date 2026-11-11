# Multi Belt Balancer: behavior and implementation

Describes the exported implementation **0.2.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

F5 chooses 2×2, 3×3 or 4×4; I selects distinct existing input belts and O selects output belts within the configured junction radius (default 12 m). A real $40-per-lane wallet payment commits a junction. Capsules in the final 35 cm of inputs enter available outputs round-robin through native Enter; input polling rotates too. A rigid three-lane housing appears only when a 3×3 layout fits six authored ports within 0.30 m.

## State and lifecycle

Up to 32 junctions are session-local and clear on world/player replacement. Records are rooted during enumeration and transfers occur after key snapshots. One shared model serves collider-free per-junction roots; invalid/missing layouts hide/retry visuals. No native belt kind or saved route is added.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/multi-belt-balancer/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

Packaged defaults in [settings.ini](../source/mods/multi-belt-balancer/settings.ini); supported bounds and interpretation remain in the linked parser.

| Section | Setting | Default |
|---|---|---|
| `[settings]` | `VariantKey` | `116` |
| `[settings]` | `InputKey` | `73` |
| `[settings]` | `OutputKey` | `79` |
| `[settings]` | `CancelKey` | `8` |
| `[settings]` | `PricePerLane` | `40` |
| `[settings]` | `MaxJunctionMetres` | `12` |

## Implementation sources

- [mod.c](../source/mods/multi-belt-balancer/mod.c)
- [model_fit.h](../source/mods/multi-belt-balancer/model_fit.h)
- [model.h](../source/mods/multi-belt-balancer/model.h)
- [building.h](../source/mods/multi-belt-balancer/building.h)
- [config.h](../source/mods/multi-belt-balancer/config.h)

## Verification boundary

No standalone build item, saved junctions, removal UI or guest protocol. The model never stretches to arbitrary endpoints; unmatched 2×2/4×4 layouts retain native presentation. Throughput, support/clearance and co-op require game checks.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
