# Async Asset Loading
* In VFrame, assets can be loaded at runtime. The load operation is carried out on new CPU thread and a Secondary GPU Queue. This is queue is VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT capable, similar to the Primary queue
* The asset loading thread is detached as there is no need to wait for an asset loading request to complete
* There is no explicit synchronization between the Primary and Secondary queue submission at the moment.
* Maximum supported Texture count is 2048 at the moment. New textures are asynchronously updated to next available index in the Array of Textures. The material storage buffer is then updated with the new sub-mesh's material properties including indices to Albedo, Normal and Roughness_Metal maps
* The Id of the submesh itself is updated via a push constant when the sub-mesh is drawn from the primary queue