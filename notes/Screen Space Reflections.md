# Screen Space Reflections
## Current State
* SSR currently works for differed pipeline only
* The SSR buffer is then bullerd for sampling rough surfaces (experimental). This is not correctly done at the moment and is turning out to be too expensive.
* The composition of SSR into the color buffer goes through tone-mapping
* Might incorporate some Fresnel effect

## Challenges
* Missed the Y flip when moving from view to clip space. Diagnosis eluded me longer than I expected. One quick solution around this is to build a quick viewport space transition calls in Common.h so these problems are dealt with.
* The current performance of this implementation is a concern.

## Performance 
* The compute implementation is the most expensive shader in the implementation
* Sampling from position buffer instead of depth. Maybe can use High-Z for faster intersection. 

## Resources
* https://lettier.github.io/3d-game-shaders-for-beginners/screen-space-reflection.html
* https://imanolfotia.com/blog/1
  
## Screenshots
<img width="1444" alt="SSR_1" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/SSR_1.PNG">
<img width="1444" alt="SSR_2" src="https://github.com/kapvipoor/VFrame/blob/main/notes/assets/SSR_2.PNG">
