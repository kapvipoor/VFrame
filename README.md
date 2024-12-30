<img width="1437" alt="DemoReel_03232024" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/demoreel.PNG">

<img width="1444" alt="FixedShadowSceneFitting_2" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/demoreel_2.PNG">

# VFrame
 A compilation of few rasterization and Hybrid Ray Tracing techniques using Vulkan API and C++

## External Tools/Packages Used
1. tinygltf (fork, module)
2. NiceMath
3. tiny_obj_loader
4. ImGui (fork, module)
5. ImGuizmo (fork, module)
6. Imimgui-filebrowser (fork, module)

## Features
* [Array Of Textures, Non-Uniform Descriptor Indexing and Bindless](https://github.com/kapvipoor/VFrame/blob/main/notes/Bindless%20Descriptor%20and%20Material%20Management.md)
* [Asset Loaders](https://github.com/kapvipoor/VFrame/blob/main/notes/Asset%20Loader%20Support.md)
* [Asynchronous Asset Loading](https://github.com/kapvipoor/VFrame/blob/main/notes/Async%20Asset%20Loading.md)
* [Forward and Deferred Rendering](https://github.com/kapvipoor/VFrame/blob/main/notes/Forward%20and%20Deferred%20Rendering.md)
* [Screen Space Reflections](https://github.com/kapvipoor/VFrame/blob/main/notes/Screen%20Space%20Reflections.md)
* [Temporal Anti Aliasing](https://github.com/kapvipoor/VFrame/blob/main/notes/Temporal%20Anti-Aliasing.md)
* [Tone Mapping](https://github.com/kapvipoor/VFrame/blob/main/notes/Tone%20Mapping.md)
* [PBR and IBL](https://github.com/kapvipoor/VFrame/blob/main/notes/IBL.md)
* [Rasterized and Ray Traced Shadows](https://github.com/kapvipoor/VFrame/blob/main/notes/Rasterized%20and%20Ray%20Traced%20Shadows.md)

4.	Object Picker
	- Challanges Faced:	- 
	- Algorithm Limitations: - 
	- Bugs:
		1. Object picking is not accurate in Forward Pass
		2. Object picking is not correctly implemented in Differed Pass
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
