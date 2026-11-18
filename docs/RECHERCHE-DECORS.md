# Decor persistence research history

Current movable decor is a persistent dynamic native entity, not an arbitrary static scene instance. Native template identity and full CurrentTransform are retained through the Prop serializer. Existing comfort and attachment components are preserved; native spatial systems evaluate comfort from the moved position.

The first concrete case was `Prop_Furniture_Comfort1_Bar_01_Wood`, a dynamic Prop with a native placement ItemInfo. Later floor-placement fallback supported eligible objects without one. High-bit scene IDs, DoNotSave objects and incompatible attachment/collision contracts remain refused.

See [counter integration](DEPLACEMENT-COMPTOIR.md), [floor placement](POSE-AU-SOL.md), [doors](PORTES.md) and [wells](PUITS.md) for actionable contracts. The full dated notebook retains the comparison of chest/decor save mechanisms, reflection data, native offsets and observations. Do not infer movement support solely from a model name or from an object's destructibility.

## Original research record

The complete original French notebook is preserved in [the historical archive](archive/fr/RECHERCHE-DECORS.md). It records earlier development stages and is not the current user/developer guide. Technical measurements and offsets have not been discarded.
