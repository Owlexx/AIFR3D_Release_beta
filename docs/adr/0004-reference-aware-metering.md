# ADR 0004: Reference-Aware Metering

## Status
Accepted

## Decision
Reference profiles are computed/cached independently, then compared by feature deltas in runtime.

## Consequences
- Fast reload and repeatable comparison behavior.
- Additional cache invalidation complexity.
