# Mesh Deformation toolkit
Full mesh deformation toolkit inside UE4, including tools (Move, Scale, Rotate, Spherize etc) based on popular 3d modelling packages, and a set of utilities to control which parts of a mesh will be affected.

In order to allow complex effects this supports:
* Multiple transformations, either using the same selection set or different ones.
* Combining selections for greater control.
* Can be extended in Blueprint to add custom transformations and selection controls.

**At present this should be considered alpha quality as code needs more bullet-proofing, documentation, and any and all APIs may change**

## Introduction
Any 3d artist has done mesh deformation, even if they're not familiar with the term.

It's where you have some existing geometry, select some or all of the vertices, and move them about using the tools provided.

Mesh deformation isn't create or destroy geometry, and it's not changing the triangle data connecting the vertices so tearing and so on isn't supported, but it is a flexible toolset for manipulating geometry.

This plugin, the Mesh Deformation Toolkit (MDT for short) allows you to do this type of geometry manipulation inside of Unreal Engine via Blueprint.

This is done by providing a UE4 Component called **MeshDeformationComponent** which allows you to store and deform mesh  geometry, using the selection and transformation nodes provided, and then output it to your UE4 level.  All inside Unreal Engine, and available either inside the editor or at runtime.

If you like SplineMeshComponent you should love MeshDeformationComponent.

### Basic Workflow
The workflow for this plugin is based on that a 3d artist would use in their tools.

#### Step 1: Load the geometry
In a 3d package this is where you either use the tools provided to create a base mesh, or load one in.

In the plugin this is done by the "Load" nodes, although at present there is only one of these- "Load from Static Mesh" which uses a standard StaticMesh.

#### Step 2: Select the vertices to modify
In your 3d package this is where you drag your mouse over the vertices and choose those you want to move.

In the plugin this is the various "Select" nodes which each allow a different type of selection- such as vertices near a provided position, or near a provided spline path.  Similar to many 3d packages this selection isn't an all-or-nothing selection, some points can be 'partly selected' which makes it easier to gets smoother results.

There are also selection modifiers which allow you to invert selections, apply easing functions to make selections smoother, or combine selections by adding, subtracing, blending, or more.

#### Step 3: Transform the vertices
There are a lot of tools for this in a 3d package, some allowing basic operations such as Move, Scale, Rotate, and other stranger ones such as Inflate which moves points along their normals, or Spherize which blends a mesh towards a spherical shape.

This plugin provides a set of transform nodes based on the tools 3d packages provide, each of which can also take a selection to control their influence on the vertices.

A selection can also be used to drive multiple transformations so if you want to move your selected points upwards and then rotate them that's fine.

#### Step 4: Rinse and repeat...
It's rare that you'll get exactly the shape you want with a single transformation, you'll want to to keep selecting and moving different parts of the mesh.

This can be done by just chaining selections/transformations together.

#### Step 5: Save the result
In a 3d package this is where you save your results to disk ready for rendering, or to import into another tool such as UE4.

In the plugin this is the "Save" nodes.  Again at present there's only one of these, "Update Procedural Mesh Component" (*NB:* This is going to get renamed to Save To Procedrual Mesh Component) which takes the transformed geometry and writes it out to one of Unreal's ProceduralMeshComponents which can then be added to the scene.

## Installing the plugin
When the plugin hits public beta I'll be releasing compiled plugins for Windows and Mac, and possibly other platforms too.

For now though to get it working you'll need the *Plugins/MeshDeformationToolkit* directory from this repo and copy it to the same path in your own project, then when you run UE4 you should be able to enable/compile the plugin (This will need a C++ compiler).

## How It Works
When everything is working and finished I will be taking the time to document how exactly this plugin works, including how you can extend it yourself by writing new selection/transform nodes either in C++ or Blueprint.

For now though I can only point you towards [a tutorial I wrote which started all of this](http://normalvector.com/tutorials/low_poly_world/) which has the basic approach to deforming geometry (In pure Blueprint too!), and add the following quick notes.
* Geometry is basically stored in the format which ProceduralMeshComponent wants, and so a mesh contains a list of sections- each of which has it's own vertex list.  It is this vertex list which the plugin manipulates.
* All any Selection node does is iterate over all of the vertices in a mesh and apply a selection rule to each to obtain the selection weight, storing these in an array in SelectionSet.  The array is ordered the same as the mesh vertices to prevent any other metadata being needed.
* The selection modification nodes just take one (or more) SelectionSets and return another SelectionSet with the weights tweaked as needed.
* Every Transform node is actually currently implemented as a method on the mesh geometry which iterates over the vertices in the mesh and applies the transformation to each of them in order.  Selections are handled by basically acting as a lerp between the original position and the fully transformed position, although the nature of the lerp can vary between methods (Translate has a straightfoward spatial lerp, but Rotate actually lerps the rotational angle for better results).  Most of these are implemented in a Blueprint Library class as they're utility functions rather than being called on a class.
* The MeshDeformationComponent itself just contains a set of mesh geometry and offers BP-callable selection and transform nodes which just delegate the call to the mesh.

## Code documentation
All C++ code has been documented using [Doxygen](http://www.stack.nl/~dimitri/doxygen/) together with [my Doxygen source filter to remove UE4 macros](https://github.com/normalvector/ue4_doxygen_source_filter), and will be made available as API docs online at some point.

## Thanks Go To..
[Jordan Peck](https://github.com/Auburns) for his splendid [FastNoise](https://github.com/Auburns/FastNoise) C++ library, which is used for all of the noise generation in the plugin.

## ToDo List
### General Work
* Check that functions are Pure when needed...
* Move content over from the ProceduralToolkit repo
* Build a demo scene for each node
* Write a better intro doc...
* ...write a short doc for each node building on the intro doc and referring to the demo scene.
* Write a 'How it works' doc, also documenting how to write new selections and transformations.
* Make Doxygen API docs available on either Github or Normalvector.
* Put variable declarations inside loops- let the C++ optimizer deal with those.
* Enforce UE4-style naming (UpperCamelCase for variables)
* Check the passing of FRandomStream, should it be reference?
* Support for [Runtime Mesh Component](https://forums.unrealengine.com/showthread.php?113432-Runtime-Mesh-Component-Rendering-high-performance-runtime-generated-meshes!).
* Sort nodes and docs in a sensible order
* A transform node to allow the production of face-weighted normals.
* Check SelectionSet size matches that of the geometry provided.
* Check SelectionSet sizes in operations which take multiple sets.
* Can I move to [FastNoiseSIMD](https://github.com/Auburns/FastNoiseSIMD) to speed up noise creation, or at least have a #define to control whether it should be enabled on a particular platform and fallback to FastNoise where it's not available.
* Investigate whether FTransforms can be passed in from BP with an identity default (The SelectByNoise node would appreciate it...)
* Give FitToSpline limits on which portion of the spline it should fit to- defaulting to 0-1 for the full range.

### New Nodes to write
* Load From File- Load a mesh from a .obj (.fbx possible?) and use it as a base.
* Load From Procedural Mesh Component- Use an existing PCM as a source of geometry.
* Save To Static Mesh- It should be possible to copy PCM's functionality and allow the plugin to actually write static meshes, at least inside the editor.
* Save To File- Write mesh geometry out as a .obj (.fbx possible?).
* Follow Spline- A transform node which makes the geometry follow the shape of a provided SplineComponent.
* Spline Lerp- A Lerp where all vertices follow a spline based on the position/normal of the start/end positions to allow smooth curve interpolation.  Very heavy CPU use but will make for a very nice animation trick.
* Conform- Make the geometry conform to guide geometry by projecting all vertices along a vector until they hit a collision.  This was the original "Floor Waffles" demo.
* A 'three-way blend' with blends two SelectionSets based on the weights of a third.

## Change Log
13<sup>th</sup> July 2017:
* Added the controls to SelectByNoise to pass in a transform and allow tiled terrain.
* Added the SelectByVolume node, and a demo for it.
* Added the SelectBySection node, and a demo for it.

12<sup>th</th> July 2017:
* Finished rename from Procedural Toolkit.
* Created new Github repo.
* Created structure for new demo scenes, and did a lot of initial demos.
* Renamed UpdateProcedcuralMeshComponent to SaveToProceduralMeshComponent to allow all 'output' node to start with Save.

## LICENSE ([MIT License](https://en.wikipedia.org/wiki/MIT_License))

Copyright 2017 Paul Golds.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
