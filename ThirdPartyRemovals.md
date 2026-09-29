Most art was done in-house or from contractors, but there were some off-the-shelf assets in use that were not compatible with an open source release.
No (proper) artists were involved in stripping third party assets. To the extent that the game looks worse, it is not their fault.

* Plenty of third party assets were removed that were not actively in use, and are not listed in details.
* Many 3rd party noise textures were replaced with existing or new noise textures
	* Textures in `/Game/ThirdPartyReplacements/ScreamingBrainStudiosCC0/` are from https://opengameart.org/content/700-noise-textures
* Foliage: Meshes for bushes, hedges, nettles, and most trees had previously been re-modeled by us based on the StylizedForest2 package (in order to be nanite-friendly). These have been replaced with cutout versions of older internal models. They may have more nanite overdraw, which may hurt performance. They are also quite large in editor, since the size reduction is just happening in the nanite import settings (but reasonable-ish in game).
* Guildhall: Medieval Feast meshes used for set dressing the kitchen have been replaced with other food and cutlery meshes.
* Cobwebs: Both gameplay levels had been set-dressed with cobweb meshes from a third-party Cobweb Pack (https://www.fab.com/listings/c33d10af-6d1e-44f9-8d6b-a594ea90325d). These mesh assets have each been replaced with a single triangle mesh in the `/ThirdPartyReplacements/CobwebReplacements`. Each mesh retains the same filename, but with a `_TOMBSTONE` suffix. This should allow these tombstones to be replaced with the original meshes to restore the original set dressing.
* NS_Fireplace: Remove explosion particles in Fountain emitter
* NS_Truncheon_Hit: Remove smoke
* OldManor: Remove water leak particles from secret passage
* NS_GooPipe_Enabled: Remove bubbles from Fountain emitter
* Tutorial Level: Replaced 3rd-party dresser table with a different mesh (one with map + key)
* OldManor: Remove rust drip decals from walls
* Gems: Set-Collection gems and Contract-7 Eye of Argon have been replace with older non-thirdparty models
* Gems: Replace gem shader with existing pearl-like gem shader (loot + vistara diamond)
* Skybox mountain textures replaced with generic grass texture
* ThievesDen: Replace telescope with different mesh
* Fonts: The Vision font was replaced with Roboto in the fallback input prompt text in `BPUI_HotbarItemInput`
* Presumed-Third-party `SM_Park{Pedestal,FlowerPot}` meshes have been replaced with greybox meshes with a similar silhouette
* Volumetric Fog Splines
	* `BP_Spline_UltraVolumetrics` was used a dozen or so times in levels for local fog effects (in addition to other fog stuff). This has been replaced with `BP_VolumetricFogSpline_Proxy`, which is an empty child BP which can be reparented to the desired implementation without touching any of the actor instances.
	* `BP_VolumetricFogSpline_Stub` has a spline component and just enough instance editable properties to avoid data loss (and nothing else).
	* `BP_VolumetricFogSpline_Mini` has a stripped-down partial re-implementation of the features used by the game in practice, using custom primitive data. It does not precisely match the original visuals, but is ballpark.
* FluidNinja
	* No assets should remain from the fluid ninja package, although the `/Game/Art/VFX/FluidNinja/WIP` folder contains baked output from the tool
	* All materials to play them have been removed, but a stripped-down reimplementation has been swapped in. It does not support velocity. Some flame and misc vfx will look a bit different.`
* Loot
	* Major
		* GoldGobletA, GoldCrownA meshes replaced with existing meshes
		* GoldUrnA replaced mesh with squashed bird urn loot mesh
		* ScrollGoldA (unused) replaced mesh with existing and gold material (silly)
		* `BP_LootActor_Major_GoldBars` mesh replaced with a gold-colored stretched cube
	* Minor
		* BowlSilverA, GobletSilverA, BP_LootActor_Minor_VaseA mesh replaced with existing mesh
		* BraceletA: mesh replaced with donut-like torus
		* CandlestickA: Mesh disconnected, and removed from `DA_SpawnBucket_MinorLoot_Dining` spawn bucket
		* `BP_LootActor_Minor_NecklaceA`: Mesh removed, and actor removed from `DA_SpawnBucket_MinorLoot_IndoorBehindGlass` and `DA_SpawnBucket_MinorLoot_Offices`
