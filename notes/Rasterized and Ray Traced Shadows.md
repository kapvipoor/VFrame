# Dynamic Shadows
## Current State
* The scene graph flags its state to SceneMoved/BoundsChange if any entity moves around. This status helps identifying when to rebuild the shadow map (for Rasterization Pipeline) or rebuild the TLAS (for Ray tracing Pipeline)
* When a new entity with a mesh is introduced to the scene graph at runtime, the same process applies and updates the shadow resources
* Lights are entities in the scene graph, hence the primary directional light can be rotated, hence changing the direction of the shadow dynamically. 
* A AABB scene fitting bounding box is computed and applied to the light camera to make sure the entire scene is with-in the light frustum, hence ensuring all the objects in the scene can participate for shadows (Rasterized). This is however not needed for the ray tracing pipeline. 

# Rasterized Shadows using Shadow Maps (Directional Light)
## Current State
* Currently implements directional light shadow using shadow map. The light perspective depth pre-pass is done before the primary g buffer pass
* In forward pipeline, shadow depth comparison is done in the fragment shader of the gbuffer pass
* In deferred pipeline, shadow depth comparison is done in the light computation compute shader post g buffer pass
* Percentage Close Filtering (PCF) is applied by sampling 3x3 neighbors to give a blur to edges of the hard shadows

# Ray Traced Shadows (Directional Light)
* Entities with meshes that are loaded in the scene have their own BLAS. The entire scene builds its TLAS at loading time and updates every time there is any change to the BLAS transforms. This enables dynamic Ray Traced shadows
* Ray tracing pipeline are not in use. To avoid that complexity, rayQueryEXT is used. Shadow is computed in the Differed Light Computation compute shader by collecting the view space positions from the rasterized position buffer.
* Since the TLAS and BLAS are built using wold space positions, the positions are converted to world space and applied as origin to the ray.
* There is some flickering in the shadow pass noticed when the BLAS is updated. There is a hard sync after BLAS update but I am unsure at the moment to why the flicker is noticed. 
* Apart from this, the quality of RT shadows can be improved by providing soft shadows along with a good TAA solution

## Challenges
* Wrong Orthgraphic Matrics were implemented. Ensure there is an implemtation for Right Hand, Zero to One Z value
* Identifying Vulkan Vetex to Pixel pipeline - Z ranges [-1,1] and does not need to be normalized to [0,1]
    X and Y also ranges [-1,1] and did need to be normalized to [0,1] for depth sampling
* Y is inverted in Vulkan, so y = 1.0 - y for sampling the depth map
* Some wrong self shadowing issues related to low Depth Precision - FIXED by increasing precision from 16 bits to 24 bits
* Some other wrong self shadowing issues - FIXED by ensuring shadow is applied to (NdotL < 0)

## Performance 
* No effort has been put to reduce the Shadow depth pre-pass load. Having a good frustum and occlusion system will help performance there.
* Moving to Cascade Shadow Maps will provide a better balance to quality and performance. ATM, the shadow map resolution is 4k.

## Resources
* https://www.youtube.com/watch?v=N1OVfBEcyb8&list=PL0JVLUVCkk-l7CWCn3-cdftR0oajugYvd&index=24
* https://github.com/SaschaWillems/Vulkan/tree/master/examples/raytracingshadows
  
## Screenshots
<img width="1444" alt="RasterVRTDirShadow" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/Ray%20Traced%20Directional%20Shadow.PNG">


