================================
Render-graph
================================

The sequence of commands needed to render a frame can grow as the pipeline's logic becomes more complex.
These commands involve objects (e.g. buffers and images) whose access must be correctly synchronized to avoid undefined behaviors
such as readings happening concurrently with writes.

The Vulkan way to ensure synchronization for memory accesses is by using pipeline memory barriers.

Quick overview to pipeline barriers
-----------------------------

*If you already know what a pipeline memory barrier is you can skip this section.*

Assume we have a simple pipeline made of a compute task ``C1`` that writes to an image ``I1`` and another compute task ``C2`` that has to read
from the same image:

.. image:: img/render_graph_1.png
    :height: 150
    :align: center

And consider the following pseudo-code used to dispatch the compute tasks:

.. code-block:: cpp

    bind(C1, I1);
    dispatch(C1); // Write to I1

    bind(C2, I1);
    dispatch(C2); // Read from I1

Dispatching ``C2`` after ``C1`` [1] will only ensure that ``C2`` operations will start after ``C1`` operations have started.
Therefore we have no assurance that ``C2`` reads will happen only after all ``C1`` writes completed, so this code's behavior is undefined.

What Vulkan offers to ensure synchronization are **pipeline memory barriers**: a pipeline memory barrier is placed between the two
commands of interest, i.e. ``C1`` and ``C2``, for which you define a set of source operations ``C1_OP`` for ``C1``, and a set of destination operations ``C2_OP`` for ``C2``.
When the commands are dispatched along with the pipeline memory barrier you have the confidence that ``C1_OP`` are completed before ``C2_OP`` start.

Let's set up the memory barrier to accomplish our problem:

.. code-block:: cpp

    bind(C1, I1);
    dispatch(C1); // Write to I1

    place_pipeline_memory_barrier(/* C1_OP */ READ, /* C2_OP */ WRITE);

    bind(C2, I1);
    dispatch(C2); // Read from I1

This code is valid and doesn't represent an undefined behavior.

As the complexity of our pipeline grows, we may want to dispatch many commands before ``C2`` that do not depend on ``I1``.
In such case there would be an obvious "over-synchronization" issue because the pipeline is now waiting for the reads of ``C1`` to happen before the writes issued by
any command on any object, while we are only interested in ``C2`` writes to ``I1``.  

.. code-block:: cpp

    bind(C1, I1);
    dispatch(C1); // Write to I1

    place_pipeline_memory_barrier(/* C1_OP */ READ, /* C2_OP */ WRITE);

    dispatch(D1); // If D1 wants to write to a second image I2 it will be blocked by
                  // the barrier and forced to wait for C1 reads
    dispatch(D2);
    dispatch(D3);
    dispatch(D4);

    bind(C2, I1);
    dispatch(C2); // Read from I1 (we want the barrier to act here)


For this purpose Vulkan offers the possibility to narrow the memory barrier to a specified object, such as an image or a buffer,
this way wherever we place the barrier (as long as it's between ``C1`` and ``C2``) we're sure it will synchronize read/write(s) that involve ``I1``.


.. code-block:: cpp

    place_pipeline_memory_barrier(I1, /* C1_OP */ READ, /* C2_OP */ WRITE);


Actually there are lot more settings that can be specified for a pipeline barrier, for example the exact stage of ``C1`` and ``C2`` that source/destination operations belong to,
the exact type of read/write...

Moreover **pipeline barriers are used for image layout transitions**: an image to be used in some tasks must be/it's better to have it in a certain
layout and to handle each of these layout transitions a pipeline barrier has to be placed.

How do they look in Vulkan
-----------------------------

The actual Vulkan command used to place memory barriers is the following:

.. code-block:: cpp

    void vkCmdPipelineBarrier(
        VkCommandBuffer                             commandBuffer,
        VkPipelineStageFlags                        srcStageMask,
        VkPipelineStageFlags                        dstStageMask,
        VkDependencyFlags                           dependencyFlags,
        uint32_t                                    memoryBarrierCount,
        const VkMemoryBarrier*                      pMemoryBarriers,
        uint32_t                                    bufferMemoryBarrierCount,
        const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
        uint32_t                                    imageMemoryBarrierCount,
        const VkImageMemoryBarrier*                 pImageMemoryBarriers
    );

For which the following structs have to be instantiated:

.. code-block:: cpp

    typedef struct VkImageMemoryBarrier {
        VkStructureType            sType;
        const void*                pNext;
        VkAccessFlags              srcAccessMask;
        VkAccessFlags              dstAccessMask;
        VkImageLayout              oldLayout;
        VkImageLayout              newLayout;
        uint32_t                   srcQueueFamilyIndex;
        uint32_t                   dstQueueFamilyIndex;
        VkImage                    image;
        VkImageSubresourceRange    subresourceRange;
    } VkImageMemoryBarrier;

.. code-block:: cpp

    typedef struct VkBufferMemoryBarrier {
        VkStructureType    sType;
        const void*        pNext;
        VkAccessFlags      srcAccessMask;
        VkAccessFlags      dstAccessMask;
        uint32_t           srcQueueFamilyIndex;
        uint32_t           dstQueueFamilyIndex;
        VkBuffer           buffer;
        VkDeviceSize       offset;
        VkDeviceSize       size;
    } VkBufferMemoryBarrier;

Conclusion
-----------------

To keep it simple, we identify two usage approaches:

1. Use every field and therefore taking advantage of the low-level provided by the Vulkan API.
2. Use a few number of fields just to avoid undefined behavior but almost certainly over-synchronizing.

It's obvious that to achieve the best performance we would choose ``1.`` but it's really hard to maintain as the structure of our pipeline changes.
If we insert, move, delete commands we could need to re-define some pipeline barrier and probably deal with tedious validation errors.

The render-graph architecture comes in aid for this purpose: **it permits to define pipeline memory barriers
on the fly trying to get rid of over-synchronization and therefore to be as detailed as possible**.
