# Temporal Anti Aliasing
## Current State
* TAA currently works for both deferred and forward pipeline
* Its applied post debug draw. Temporal color copy takes place in the same command buffer as tone mapping post its submission
* The jittering is computed and applied on the CPU side. The jittered view projection is passed as uniform to generate the G-Buffers
* The motion vectors are generated along with the G-Buffers and are un-jittered
* There is currently an optional flag to compute re-projection either from Motion buffer or through inverse viewproj within the TAA pass. Incase of inverse viewproj, positions are computed using the depth buffer only
* Disocclusion is solved though Color Clamping by sampling 3x3 neighbours from current color and clamping previous color to max and min of the sampled kernel. It seems too aggressive though.
* I ahve tried to solve Color Flickering using Inverse Luminance and Inverse Log algorithms, however they are broken at the moment. I think they need to applied when filtering the previous color
* To preserve some sharpness, I filtered the previous color using CatmullRom (4x4 neighbor sampling) however I am unable to see any difference between linear and Catmull ROm cubic sampling. This probably needs to be revaluated.
* The quality of the generated TAA can be improved by applying an sharpening filter

## Challenges
* TAA is a rabbit hole inself however introduces to some amazing filtering and image processing techniques. A lot of the skill learnt here can be used to build an intuition to solve other issues.

## Performance 
* At the moment, there is no memory prefetching applied when sampling neighbors during the CatmullRom filtering and Color Clamping. That will be addressed in near future
* Also, a lot of flags are being passed through uniforms and the shader code branches based on these uniform values. The performance an be improved by removing the branching, writing shaders into different shader combinations and applying those PSOs when switching using the UI settings

## Resources
* https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/
* https://alextardif.com/TAA.html

  
## Screenshots
https://github.com/user-attachments/assets/9dfa5806-1603-4ba4-8a34-e4a7b7e3661f


