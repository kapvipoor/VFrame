# Descriptor Management
* In this project, Descriptors are classified into
   1. Primary Descriptors - Resources needed to run the frame. Include debug resources, render targets and other fixed buffer initialized at load time
   2. Scene Descriptors - Resources needed to load and render a scene. This descriptor set is bindless and write-updated when assets are asynchronously loaded at runtime.
   3. UI Descriptors - Resources needed to load and render the UI.
* All these descriptors are allocated in sets of number of frames in flight. At the moment, this is hardcoded to 2.

## Scene Descriptors (Bindless and Array of Textures)
* All the descriptors in the set are assigned the flags - VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT
* All textures in the scene are loaded as an array of textures and also assigned the flag - VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
* This descriptor is write-updated at runtime when assets are loaded. Index of the texture in the array of texture is its index mentioned in the material storage buffer
* Material data is stored in a Storage Buffer. Each entry associates to the submesh in the scenegraph. Includes PBR coeffs and texture indices to Albedo, Normal and Roughness Metal maps

<img width="1444" alt="BindlessDescriptorandMaterialManagement" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/Bindless%20Descriptor%20and%20Material%20Management.PNG">
