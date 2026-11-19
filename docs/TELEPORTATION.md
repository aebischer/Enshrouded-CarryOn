# Cross-claim transport and interruptions

Goal: find decor, place a claim altar nearby, pick up with the pincers, teleport
to another claim and commit placement.

## Invariants

The original stays intact until valid placement. The map suspends commands and
preview without losing selection. Cancelling or closing the world leaves the
original in place. No intermediate inventory item or generic component
reconstruction is used.

The server validates the entire destination bounds against its own claim and
native permissions/collisions. Destination claim identity travels with the request;
an overlapping altar must not silently replace the validated one. Source claim
and object identity remain checked during travel. Component pointers never
survive unloading.

Native HUD values: InGame=1, Map=5, NoUi=51 (table `0x1bac180`, 54 values).
Map suspends pickup; NoUi only continues an existing suspension. TeleportCount
stores a count at +0 and lastTeleportTime at +8. Changing world/server invalidates
all transactions.

## Observation and implementation

November 17 read-only sample: `Prop_Deco_Villageprops_Markettable_03`, entity 1897,
template `04178d57450c134d9b6dc5c97c4777e9`. After travelling about 1.27 km,
the client copy disappeared but the server entity retained its world, identity,
transform and collision handle 486. Local evidence: `work/teleport-source-*-before.json`,
`teleport-source-server-after.json`, `teleport-presence.jsonl`. No objects were
modified during those observations.

A mutex-protected client snapshot contains screen, UI/target timestamps, world,
generation, activity and eligibility. The server reads a coherent copy with no
UI pointer. Suspension invalidates old clicks, preview samples and placement
requests. Returning to gameplay requires a fresh preview. Other menus, focus
loss, tool/world changes and invalid sources still cancel. Closing the map
without teleporting also resumes preview.

Every request carries destination altar GUID and entity. Validation/commit
require that same active altar to contain every object corner and satisfy native
placement checks. The source altar GUID/entity chosen at pickup remains fixed;
zone enumeration order or overlapping claims cannot replace it. Removing or
shrinking the source claim so it no longer contains the origin invalidates pickup.

After commit, synchronization waits for the client object to appear. Absence is
not mistaken for mismatched identity. Existing but different identity is rejected;
world destruction clears pending work. The server removes entries for vanished
or changed objects. Valid pending destinations do not expire. After five seconds,
rendering retries at most once per second and logs the wait even when the client
descriptor is absent. Later 0.1.47 changes follow authoritative server pose
adjustments rather than discarding valid pending visual repair.

## 0.1.45: map suspension fix

Opening the map cancelled pickup in 0.1.44. The graphics global at `0x283f4a8`
was usually null outside rendering callbacks; reads briefly observed Map=5
between nulls. The server interpreted missing graphics context as Invalid=0
and cancelled after the client had suspended active mode.

The server no longer dereferences this graphics context. transportFrame publishes
the actual cursor HudScreenType. If Map appears between server observation and
eligibility loss, suspension is rechecked before cancellation. The first delayed
map frame also preserves already armed selection. Tests cover absent graphics
context, Map/eligibility interleaving, delayed Map, NoUi and resumption.
Alain subsequently confirmed travel and save/reload persistence in-game.

## 0.1.46: interruption safeguards

- Native ECS destructor `0x95d9c0` destroys dynamic storage `world+0x7bc030`;
  observed callers include `0x94e0f6` / `0x94e571`. The detour invalidates mod
  state before forwarding outside its mutexes, increments a generation independent
  of memory address, and does not free render handles on that thread.
- UI pass `0xd5bb50`, called at `0xbd1127` with live context, publishes the screen
  even without targeting. Verified chain: context+0xd0 → services+0 → game+0x38
  → client+0x1cd8. The original call is always forwarded.
- Fresh UI heartbeat permits indefinitely long map/loading suspension. If the
  server runs but UI publication stops for five seconds, cancel. A server stall
  longer than five seconds allows two seconds for the first returning UI frame.
  After InGame resumes, targeting has two seconds to recover; missing targeting
  cannot maintain suspension forever.
- Server player `Health` uses the pinned 40-byte attribute layout: root
  `0x8eb84995`, storage offset 8, initialized flag and eight signed slots;
  current health is at +8 and maximum at +36. Invalid/missing server data
  or nonpositive current/maximum refuses pickup and commit, including while
  the map is open. `NetworkHealth` is not required: it was absent on the live
  player. Client preview checks the equipped tool; health authorization stays
  on the server, since the client player may expose neither health component.

Tests cover 144 family/phase/interruption combinations, health, menus/focus/tool,
source/claim invalidation, old-session snapshots, repeated trips, long live loading,
stalled server, failed resumption, closing before commit, address reuse, overlapping
claims, delayed replication, source disappearance and throttled render retries.
Hooks remain restricted to the pinned executable hash.

## Validation limits

Offline tests cover all four object families, stale placement, destination-altar
changes, delayed client replication and existing regressions. They do not run
the actual engine's save/teleport cycle. Alain confirmed cross-claim pickup,
map/teleport, placement and reload persistence in 0.1.45. Later interruption
changes require their own game checks, including closing the map without travel
and cancelling after arrival. Unsupported scene instances remain rejected.
