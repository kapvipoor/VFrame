# Screen Space Reflections
## Current State
* SSR currently works for differed pipeline only
* The composition of SSR into the color buffer does not go through tone-mapping at the moment
* There are Moire artifacts currently in the implementation potentially due to high normal map and color buffer resolution. Might also experiment with reducing SSR resolution as well.
* There are issue with SSR aliasing as well. 
* There is a lot of spatio-temporal flickering which I am currently assuming is related to Moire artifacts and will have a look at it once i have fixed them
* Might incorporate some Fresnel effect

## Challenges
* Missed the Y flip when moving from view to clip space. Diagnosis eluded me longer than I expected. One quick solution around this is to build a quick viewport space transition calls in Common.h so these problems are dealt with.
* The current performance of this implementation is a concern.

## Performance 
* The compute implementation is the most expensive shader in the implementation
* Fixing the Moire artifacts will reduce some texture fetch stress. Will address performance concerns when the visual quality issues are addressed

## Screenshots
