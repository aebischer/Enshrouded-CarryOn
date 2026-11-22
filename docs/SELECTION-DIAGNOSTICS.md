# Intermittent selection investigation (0.1.53-dev)

The desert sideboard `Prop_Furniture_Desert_Sideboard_Small_01`
(`6d5aeede8685234faf88ddabc54bc919`) was reported without a highlight.
After reloading the same world with the unchanged 0.1.52 DLL, the user
confirmed both selection and grabbing work. Its observed client transform
has uniform scale (1, 1, 1), a valid rotation and a live model handle. Its
native resource passes the floor-placement generator and the full corpus test.

The original refusal was not recorded. A cache/indexing dependency is a
hypothesis, not an established cause. No selection rules, permissions,
placement, collision or persistence behavior are changed by this build.

With Debug enabled, target/selection transitions now record the selected and
candidate IDs, distance/reach, family, render/model availability, pose validity,
scale, placement item, catalog completion, rejection/retry caches and category
settings. A rejected floor descriptor also records its exception reason.
These observations do not prepare resources or write ECS components. Debug
off adds no diagnostic lookups. Native adapter tests compare selection with
Debug on and off; the corpus test explicitly requires the desert sideboard.

If the problem recurs, enable Debug and aim at the object before restarting
the game. Keep `ModLoader/mod-loader.log` and note whether toggling Debug or
reloading the world changed the result. No correction of the intermittent
failure is claimed until its cause is reproduced.
