# Image Based Lighting and Physically Based Rendering
## Current State
* For PBR, currently using Schlick GGX Geometry Shadowing and Geometry Occlusion, Trowbridge-Reitz GGX normal distribution, Schlick GGX to statistically estiamte light ray occlusion and Fresnel Schlick Approximation for Fresnel Effect
* The ambient lighting is computed using Diffuse Irradiance and Specular Reflectance (Split Sum Approximation using Quasi Monte Carlo Approximation with Importance Sampling)
* The Diffuse Irradiance Map is borrowed from https://polyhaven.com/hdris, Specular precomputed IBL is also borrowed from https://polyhaven.com/hdris and both these resources are not generated procedurally.
* The BRDF Lut is generated from https://github.com/HectorMF/BRDFGenerator

## Challenges/Limitations
* IBL works only for viewing assets that need reflectance and irradiance info the skybox. For indoor objects such as Sponza the ambient light will get occluded by the walls. A more physically accurate solution for this needs to
be applied.

## Performance 
*

## Resources
* AMD FidelityFX Cauldron Sample for PBR and IBL
* https://learnopengl.com/PBR/IBL/Diffuse-irradiance
* https://learnopengl.com/PBR/IBL/Specular-IBL
  
## Screenshots
<img width="1444" alt="IBL" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/IBL.PNG">


