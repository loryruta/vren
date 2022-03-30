---
title: "Describing shaders"
---

Thoughts about how to organize descriptors:
31/03/2022

## Shader-resource approach

A first approach for describing the buffers/textures that are bound to a shader, **is to associate a descriptor layout and a
descriptor pool to every resource**.

A **shader resource** is a set of buffers and textures that are bound together to the shader. These objects are described
through its descriptor set layout and every object is assigned to a certain binding. The shader resource' descriptor sets are
allocated from its personal descriptor pool whose size is shaped on it.

This design has the advantage of:
- Shaping the descriptor pool based on the shader resource it has to represent: we don't create descriptor pools larger than they
  should be that can potentially waste memory that can be relevant when dealing with many descriptor sets.
- Creating just one descriptor set layout (memory advantage? meh...).

Disadvantage:
- Verbosity: if you need a new shader resource (i.e. when writing a new shader) you have to create its descriptor pool and
its descriptor set layout.

In this case I considered the disadvantage to be more relevant than the advantages. Both took memory savings that can be 
ignored for the benefit of reducing verbosity.

---

## SPIR-V autogen descriptors approach

In order to reduce verbosity a possible solution can be to autogenerate descriptor set layouts from the shader SPIR-V code.
There's a library that comes in aid:
https://github.com/KhronosGroup/SPIRV-Reflect

We unbind **shader resource** with descriptor set layouts and the descriptor pool. Descriptor set layouts are now generated
on a per-shader basis while the descriptor pool is temporarily kept global with space for any kind of descriptor (WARN: possible memory waste if
many descriptor sets are allocated).

Out of a single loading function, given the SPIR-V code, we're able to generate the pipeline, the pipeline layout and its associated descriptor set layouts
that can eventually be queried for allocating descriptor sets (they must be queried by descriptor set ID). The latter are allocated from a global descriptor
pool.
