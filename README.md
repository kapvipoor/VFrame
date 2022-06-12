# VFrame
 A compilation of few rasterization techniques using Vulkan API and C++

External Tools/Packages Used
1. tinygltf
2. NiceMath
3. tiny_obj_loader

Feature Integrated
1.	Vulkan Bindless - Descriptor Indexing
	- 
	- Challanges Faced:

	- Algorithm Limitations:
 
	- Bugs:
 
2.	Forward Rendering
	- Challanges Faced:
		- 
	- Algorithm Limitations:
		- 
	- Bugs:
		- 
3.	Deferred Rendering
	- Challanges Faced:
		- 
	- Algorithm Limitations:
		- 
	- Bugs:
		1. Deferred does not present to swapchain
		2. Must provide run time option to toggle between Forward and Defered
4.	Object Picker
	- Challanges Faced:
		- 
	- Algorithm Limitations:
		- 
	- Bugs:
		1. Object picking is not accurate in Forward Pass
		2. Object picking is not correctly implemented in Differed Pass
5.	Skybox
	- Challanges Faced:
		- 
	- Algorithm Limitations:
		- 
	- Bugs:
		- 
6.	Normal Maps
	- Challanges Faced:
		1. Did not use the correct texture format to store Normals and so computed wrong TBN matrix (used UNORM duh!)
	- Algorithm Limitations:
		- 
	- Bugs:
		- 
7.	SSAO
	- Challanges Faced:
		- 
	- Algorithm Limitations:
		1. Quality of SSAO is not great. Migh want to implement AMD SSAO for comparision
	- Bugs:
		- 
8.	Shadow Maps
	- Features:
		1. Orthographic Shadow Maps implemented
		2. Implemented PCF - but is very expensive
	- Challanges Faced:
		1. Wrong Orthgraphic Matrics were implemented. Ensure there is an implemtation for Right Hand, Zero to One Z value
		2. Identifying Vulkan Vetex to Pixel pipeline - Z ranges [-1,1] and does not need to be normalised to [0,1]
		3. X and Y also ranges [-1,1] and did need to be normalised to [0,1] for depth sampling
		4. Y is inverted in Vulkan, so y = 1.0 - y for sampling the depth map
		5. Some wrong self shadowing issues related to low Depth Precision - FIXED by increasing precision from 16 bits to 24 bits
		6. Some other wrong self shadowing issues - FIXED by ensuring shadow is applied to (NdotL < 0)
	- Algorithm Limitations:
		1. Shadow map quality at 1024x1024 for 1080p is terrible. Not good at 8Kx8K either
		2. Perspective shadow map needs linearization of depth values to work
		3. Directional Light is being applied to regions in shaodw as well.
		4. High Perspective and Projection Aliasing
	- Bugs:
		1. Deferred pass is not correctly utilising the light direction
		2. Peter-Panning effect has not been addressed.  Use front face culling on shadow pass to fix this 
		3. Shadow Acne has not been addresses
9.	Gltf loader with stb_image support
	- Challanges Faced:
		- 
	- Algorithm Limitations:
		- 
	- Bugs:
		- 
10. Shader Compilation - GLSL to SPIRV
	- Challanges Faced:
		- 
	- Algorithm Limitations:
		1. Currently inspired from Sascha Willems's offline python script using glslangValidator
        2. Will later move to a runtime glsl compilation process
	- Bugs:
		- 