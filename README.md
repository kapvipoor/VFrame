<img width="1437" alt="DemoReel_03232024" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/demoreel.PNG">

<img width="1444" alt="FixedShadowSceneFitting_2" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/demoreel_2.PNG">

# VFrame
 A compilation of few rasterization techniques using Vulkan API and C++

External Tools/Packages Used
1. tinygltf (fork, module)
2. NiceMath
3. tiny_obj_loader
4. ImGui (fork, module)
5. ImGuizmo (fork, module)
6. Imimgui-filebrowser (fork, module)

Feature Integrated
* [Array Of Textures, Non-Uniform Descriptor Indexing and Bindless](https://github.com/kapvipoor/VFrame/blob/main/notes/Bindless%20Descriptor%20and%20Material%20Management.md)
* [Asynchronous Asset Loading](https://github.com/kapvipoor/VFrame/blob/main/notes/Async%20Asset%20Loading.md)
* [Forward and Deferred Rendering](https://github.com/kapvipoor/VFrame/blob/main/notes/Forward%20and%20Deferred%20Rendering.md)
* [Screen Space Reflections](https://github.com/kapvipoor/VFrame/blob/screen_space_reflections/notes/Screen%20Space%20Reflections.md)
* [Temporal Anti Aliasing](https://github.com/kapvipoor/VFrame/blob/taa/notes/Temporal%20Anti-Aliasing.md)

4.	Object Picker
	- Challanges Faced:	- 
	- Algorithm Limitations: - 
	- Bugs:
		1. Object picking is not accurate in Forward Pass
		2. Object picking is not correctly implemented in Differed Pass

5.	Skybox
	- Challanges Faced:
	- Algorithm Limitations:
	- Bugs:

6.	Normal Maps
	- Challanges Faced:
		1. Did not use the correct texture format to store Normals and so computed wrong TBN matrix (used UNORM duh!)
	- Algorithm Limitations:
	- Bugs:
		1. Forward pass is also computing lighting in View Space however inverse-transposing the light vector (after multiplying with view materix) is giving incorrect results

7.	SSAO
	- Challanges Faced:
		1. Noise computation was bugy in my previous implementaiton. Now fixed
	- Algorithm Limitations:
		1. Quality of SSAO is not great. Migh want to implement AMD SSAO for comparision
	- Bugs:

8.	Shadow Maps (Orthographc Projection and Directonal Light)
	- Features: 
		1. Orthographic Shadow Maps implemented
		2. Implemented PCF - Fixed buggy PCF in Forward
		3. Used scene's bounding box to perfectly fit the directional light's shaodw camera for best shadow map results
		4. Reuseing shadow map from previous frame if the scene has not changed
	- Challanges Faced:
		1. Wrong Orthgraphic Matrics were implemented. Ensure there is an implemtation for Right Hand, Zero to One Z value
		2. Identifying Vulkan Vetex to Pixel pipeline - Z ranges [-1,1] and does not need to be normalised to [0,1]
		3. X and Y also ranges [-1,1] and did need to be normalised to [0,1] for depth sampling
		4. Y is inverted in Vulkan, so y = 1.0 - y for sampling the depth map
		5. Some wrong self shadowing issues related to low Depth Precision - FIXED by increasing precision from 16 bits to 24 bits
		6. Some other wrong self shadowing issues - FIXED by ensuring shadow is applied to (NdotL < 0)
	- Algorithm Limitations:
		1. Perspective shadow map needs linearization of depth values to work
		2. High Perspective and Projection Aliasing
	- Bugs:
		1. PCF is not correct in the differed pass
		2. Peter-Panning effect has not been addressed
		3. Front face culling in shadow pass is producing buggy results
		4. PCF Causing HAlo effect aroung intersecting geometry (Eg: Sponza and Suzzane)

9.	Physically Based Rendering
	- Features 
		1. Using Schlick GGX Geometry Shadowing and Geometry Occlusion
		2. Using Trowbridge-Reitz GGX normal distribution
		3. Schlick GGX to statistically estiamte light ray occlusion
		4. Fresnel Schlick Approximation for Fresnel Effect
	- Challanges Faced:	- 
	- Algorithm Limitations:
		1. Not implemented IBL for accuracy
		2. Only using Lambertian for diffuse computation
	- Bugs: - 

10.	Gltf loader with stb_image support
	- Challanges Faced:	- 
	- Algorithm Limitations: - 
	- Bugs: - 

11. Shader Compilation - GLSL to SPIRV
	- Challanges Faced:	- 
	- Algorithm Limitations:
		1. Currently inspired from Sascha Willems's offline python script using glslangValidator
        2. Will later move to a runtime glsl compilation process
	- Bugs: - 

12. Bounding Box Debug Display
	- Challanges Faced:	- 
	- Algorithm Limitations:
		1. Currently generating vertex and indices as 1 complete buffer and rending as single draw call (GPU effecient with some CPU overhead)
        2. Will later add support for Instanced support for displaying bounding boxes
		3. Will leter add support to display light type identifiers, cameras and other editor friendly visual tools depending on need
	- Bugs: - 

13. User Interface and Guizmo Control
	- Challanges Faced:	- 
	- Algorithm Limitations:
		1. Using ImGui and ImGuizmo for UI and transform editting
		2. Implemented simple participant architecture for willing classes to implement their UI display
		3. Will keep improving UI as I progress
	- Bugs: - 
