# Changelog

## [0.1.0] - 2026-09-24

### Added

- Per-game-instance typed signal channels, bounded worker delivery, and subscription lifetime management.
- Runtime property conversion for supported Blueprint wildcard key and payload values.
- Hidden runtime Blueprint thunks for key creation and signal publication.
- Editor-only `Extract Signal Payload` node, which expands to the hidden payload-extraction thunk and infers its output schema from the connected Blueprint value.

### Fixed

- SignalHub K2 wildcard pin types now persist through Blueprint reconstruction and editor reload.

### Notes

- Runtime wildcard conversion currently supports names, strings, booleans, numeric primitives, and reflected structs. Unsupported properties return an explicit result and create no channel.
