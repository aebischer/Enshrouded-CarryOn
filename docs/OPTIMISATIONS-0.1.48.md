# Memory and processing — 0.1.48-dev

Native read batches retain descriptor identity, ordering and grouping. Only
private resources separate adjacent native runs, as in 0.1.47.

## Changes

1. Placement-cache allocations are protected through publication. Temporary
   resource read/allocation failure no longer permanently invalidates embedded
   data. Indexing resumes after partial publication without returning a zero ID.
2. Missing door/well resources retry at most once per second per model. Floor
   profiles retain their retry delay.
3. Movable-target queries stop when transport is inactive, unfocused or already
   carrying. Boundaries/inspection keep separate paths. An idle server tracks
   its session without resetting the transaction on every tick.
4. Indexing performs at most 64 native searches per callback. Search/read/profile
   work stays on the native thread outside the cache mutex. Preparation is
   serialized without blocking another callback on its lock; cached entries
   remain immediately available.
5. The server checks at most 32 pending synchronizations per pass, rotating for
   fairness. Entries older than five seconds are rechecked at most once per
   second. Fresh carried-object checks and final placement validation remain separate.
6. Profiles reserve their final size directly; relative references and published
   allocations stay intact. Missing-ID lists compact in place without allocating
   a new vector on each retry.
7. Private results are grouped by requester. Data copying occurs outside the
   lock while reservation/publication remain synchronized. Consumers cannot
   remove a queue with an active producer. Up to 64 freed nodes are reused;
   no per-request temporary sentinel allocation is needed. Empty idle queues are removed.

## Offline measurements

| Deterministic scenario | Before | After |
|---|---:|---:|
| Capacity of 5,769 corpus profiles | 29,688,691 bytes | 24,491,262 bytes |
| Missing door resource, 100 closely spaced requests | 100 reads | 1 read |
| 32 old sync entries, 100 closely spaced passes | 3,200 component reads | 32 reads |
| Allocation exceptions escaping the two investigated insertion points | 2 reproduced cases | 0 |

These are not actual gameplay RAM or FPS measurements. The game does not build
all profiles simultaneously; manual game performance checks remain separate.

`tests/memory_runtime.cpp` also covers eight indexing allocation-failure points,
full recovery, cache-mutex availability during native lookup, delayed retry,
synchronization fairness and 2,000 concurrent private results consumed in order
and exactly once. Reservation counters must return to zero.

Full validation retains 256 mixed-read combinations, a native 128-request batch,
interruptions, cross-claim movement, eleven wells, translations and 3D model tests.
