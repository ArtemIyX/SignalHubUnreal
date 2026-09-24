# Changelog

## [0.1.0] - 2026-09-24

### Added

- Per-game-instance typed signal channels, bounded worker delivery, and subscription lifetime management.
- Runtime property conversion for supported Blueprint wildcard key and payload values.
- Hidden runtime Blueprint thunks for key creation and signal publication.

### Notes

- Runtime wildcard conversion currently supports names, strings, booleans, numeric primitives, and reflected structs. Unsupported properties return an explicit result and create no channel.
