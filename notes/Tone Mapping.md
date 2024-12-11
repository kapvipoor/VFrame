# Tone Mapping
## Current State
* Updated the skybox textures to HDR
* Fixed HDR content loading by using stbi_f. Otherwise, stbi tonemap's the asset at load time.
* Implemented Reinhard and AMD's tone mapper
* The lighting pass is computed in HDR. Following this, SSR and SSAO composition and tone mapping take place in the tone mapping pass. Followed by gamma correction to move to linear space before writing to swapchain buffer

## Challenges
* Some type of adaptive exposure might be a good option. Otherwise a lot of details are over-exposed at current exposure default of 1.0f.
* SSR composition looks broken.

## Performance 
* NONE

## Resources
* AMD Tone Mapping - https://gpuopen.com/learn/using-amd-freesync-2-hdr-tone-mapping/
* Downloaded HDRI skydome from - https://polyhaven.com/a/autumn_field_puresky
* Converted HDRI to CubeMap from - https://matheowis.github.io/HDRI-to-CubeMap/
* Used GIMP to rotate few mis-aligned .HDR faces and export back to .HDR
  
## Screenshots
<img width="1444" alt="ColorWritePipeline" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/Color%20Write%20Pipleine.PNG">

<img width="1444" alt="HDR_On_Off" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/HDR%20On_Off.PNG">


