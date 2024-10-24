* All calculations are taking place in view space for both deferred and forward rendering
* Calculation of shadow is done in Light's view space (current position in world spce is multiplied with light's view matrix to compute shadows)
* Rendering results from both forward and deferred passes are almost identical. There are issues around specular computation between the 2 implemenations. And this is known issue to be addressed soon. 
* Deferred rendering is broken into differed pass that writes to GBuffers (Position, Normal, Albedo, Roughness Metal, Depth) and resolved in view space in a subsequent compute pass that performs Lighting (PBR), Shadows Computation, Point and Directional light implementation.
  
<video src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/Forward%20and%20Deferred%20Rendering.mp4" width="960" height="540" controls></video>
