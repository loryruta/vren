================================
GPU radix-sort
================================

VREN implements a useful parallel radix-sort algorithm. This algorithm works very well when sorting large buffers
but is constrained to sort only 32-bit integer keys. Moreover it's very easy to implement.

Let's see how it works by considering a practical sequential example:

How it works
------------

We have an array of 10 elements and a radix (or base) of 10:

.. code-block::

    (10, 25, 39, 92, 1, 5, 68, 23, 21, 10)

We iterate every element of the array and we count the occurrences of the symbols of the least significant (LS) digit.

======  =====  ======
Symbol  Count  Offset
======  =====  ======
0       2      0
1       2      2
2       1      4
3       1      5
4       0      6
5       2      6
6       0      8
7       0      8
8       1      8
9       1      9
======  =====  ======

We obtain the Offset column by running an exclusive-scan on the Count column.

Now we can iterate the array again, take the offset from the symbol of the LS digit, and place the element at its position.

Note that there could be more than one element falling on the same offset: e.g. there are 2 elements whose LS digit is 0 and both should be at offset 0.
For this reason we have to keep a counter that we increment when an element at the said offset is placed.

After this run the array will look as following:

.. code-block::

    (10, 10, 1, 21, 92, 23, 25, 5, 68, 39)

The array is sorted only for the first LS digit.

If we run back this algorithm considering the next LS digit until the most significant (MS) digit we would get a sorted array.
In this case we see all numbers are < 100, so we can say the MS digit is at the 1st position (starting from 0).

After another run we get:

.. code-block::

    (1, 5, 10, 10, 21, 23, 25, 39, 68, 92)

Which is the sorted array!

In this example we considered a radix of 10 but we can generalize it to a more computer-friendly radix such as any power-of-2 radix (e.g. 2, 4, 8 or 16...).

Parallelizing it
----------------

This algorithm can be parallelized fairly easily.

In the example above we saw that the sorting is split in many runs, one for every digit, right to left.
Every run can be split in turn in the following steps:

1. Count the number of symbols
2. Exclusive-scan to find global and local offsets
3. Re-order elements
4. Repeat 1. 2. 3. for every digit until MS digit included

Consider an array of ``N`` elements and a radix of 16 (so 16 symbols) and let's see these steps in detail:

Count the number of symbols
^^^^^^^^^^^^^^^^^^^^^^^^^^^

How can we efficiently count the number of occurrences for a certain symbol over the array?

It's trivially convenient to visit array elements in parallel. We take a workgroup size of 32 and we dispatch the
number of workgroups based on the array size (i.e. ``ceil(N / 32)``). 

Now, to count elements, a first dumb approach would be to create a 16 u32 buffer and perform an ``atomicIncrement(buffer[i])``
operation every time an occurrence of the symbol ``i`` is found. Considering we want ``N`` to be very large (> 1M), it's obviously
inefficient to have a huge amount of parallel invocations to work atomically on 16x4 memory slots.

We can do better by restricting the atomic operations within a workgroup and counting the occurrences for a sub-block of the original array in shared-memory.
Then we can get the global counts through a reduction operation (that leads to additional dispatches... but that's fine).

We can do even better! Vulkan 1.1 introduced an extension that provides a more efficient - but more restrictive -
alternative for intra-workgroup operations (`see this <https://www.khronos.org/blog/vulkan-subgroup-tutorial>`_),
that we can use instead of shared-memory ``atomicIncrement(symbol)``.

Exclusive-scan to find global and local offsets
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now we have a global counting buffer of size 16 holding the global count for every hexadecimal symbol.
As we introduced above, we run an exclusive-scan of it and we get 16 offsets, one for every symbol.

There's an evident issue we've not considered until now. Let's examine the following example:

As we're accessing the array in parallel, there could be many threads (also of different workgroups) whose working item
is a digit of value 5. Out of the previous step we only know the digit 5 has to go to the global offset e.g. 100. Those threads have to
talk to each other in order to reserve to their element a unique slot within the output array.

Also in this case, we have the dumb solution: 16 atomic counters. But we reject it for the same reason of before. 

We come up with a similar procedure: instead of having global atomic counters, we want the atomic counters to be local to the workgroup
to maximize the parallelism.

In the previous step we had a local counting buffer, stored in shared-memory that referred to a sub-block of the array. We then summed together the local
counting buffer to get a global counting buffer.

We can use the local counting buffer to get a "local offset buffer" (through workgroup exclusive-scan), that if used along with the global offset buffer,
can better index the element within the destination buffer.

Now atomic operations are restricted to the workgroup: if we have many threads (even of different workgroups) that want to place an element with the digit 5,
the global offset buffer will tell where the symbol has to be placed (e.g. 100), and the local offset buffer will tell the position according to
the workgroup (e.g. workgroup 0 at 0, workgroup 1 at 14...). If threads of the same workgroup want to place the same element, then an ``atomicIncrement``
on a shared-memory variable will be used (or better, Vulkan 1.1 subgroup arithmetics).

Re-order elements
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Finally we have to re-order the array and place elements at their correct offset as described earlier.

It's trivial to understand that we can't read and write concurrently from the same array. In fact, this parallel algorithm
requires an additional **temporary array** (at least as big as the input array) where we'll be writing to the sorted elements.

Repeat!
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now this procedure has to be repeated for every digit from LS to MS. For every run the temporary array is swapped with the input array:
one is used as the input, the other one as the output.

Considering we were working with 16 symbols (4-bit digits) per run and we want to sort 32-bit keys we would need 8 runs. While for 64-bit we would need 16 runs.

Resources
------------

- `Introduction to GPU Radix Sort - T. Harada, L. Howes <http://www.heterogeneouscompute.org/wordpress/wp-content/uploads/2011/06/RadixSort.pdf>`_ (link is broken)
- `Fast 4-way parallel radix sorting on GPUs - L. Ha, J. Kruger, C. T. Silva <https://vgc.poly.edu/~csilva/papers/cgf.pdf>`_
- `Nabla's RadixSort <https://github.com/Devsh-Graphics-Programming/Nabla/tree/42301f6861f928e4eff2c5305feffaa681a9e111/include/nbl/builtin/glsl/ext/RadixSort>`_
- `Udacity parallel programming tutorial <https://www.youtube.com/watch?v=F620ommtjqk&list=PLAwxTw4SYaPnFKojVQrmyOGFCqHTxfdv2>`_
- `Vulkan subgroup tutorial <https://www.khronos.org/blog/vulkan-subgroup-tutorial>`_





