# Mesh Deformation toolkit
Full mesh deformation toolkit inside UE4, including tools (Move, Scale, Rotate, Spherize etc) based on popular 3d modelling packages, and a set of utilities to control which parts of a mesh will be affected.

In order to allow complex effects this supports:
* Multiple transformations, either using the same selection set or different ones.
* Combining selection sets for greater control.
* Can be extended in Blueprint to add custom transformations and selection controls.

**At present this should be considered beta quality.  Most known bugs are fixed but I'm working on docs/demos and final tweaking before release.**

# Introduction
Any 3d artist has done mesh deformation, even if they're not familiar with the term.

It's where you have some existing geometry, select some or all of the points making up the model, and move them about using the tools provided.

Mesh deformation doesn't create or destroy geometry, and it won't change the triangle data connecting the vertices so tearing and so on isn't supported, but it is a flexible toolset for manipulating geometry.

This plugin, the Mesh Deformation Toolkit (MDT for short) allows you to do this type of geometry manipulation inside of Unreal Engine via Blueprint.

This is done by providing a UE4 Component called **MeshDeformationComponent** which allows you to store and deform mesh  geometry, using the selection and transformation nodes provided, and then output it to your UE4 level.  All inside Unreal Engine, and available either inside the editor or at runtime.

If you like SplineMeshComponent you should love MeshDeformationComponent.

## Basic Workflow
The workflow for this plugin is based on that a 3d artist would use in their tools.

### Step 1: Load the geometry
In a 3d package this is where you either use the tools provided to create a base mesh, or load one in.

In the plugin this is done by the "Load" nodes, although at present there is only one of these- "Load from Static Mesh" which uses a standard StaticMesh.

### Step 2: Select the vertices to modify
In your 3d package this is where you drag your mouse over the vertices and choose those you want to move.

In the plugin this is the various "Select" nodes which each allow a different type of selection- such as vertices near a provided position, or near a provided spline path.  Similar to many 3d packages this selection isn't an all-or-nothing selection, some points can be 'partly selected' which makes it easier to gets smoother results.

There are also selection modifiers which allow you to invert selections, apply easing functions to make selections smoother, or combine selections by adding, subtracing, blending, or more.

### Step 3: Transform the vertices
There are a lot of tools for this in a 3d package, some allowing basic operations such as Move, Scale, Rotate, and other stranger ones such as Inflate which moves points along their normals, or Spherize which blends a mesh towards a spherical shape.

This plugin provides a set of transform nodes based on the tools 3d packages provide, each of which can also take a selection to control their influence on the vertices.

A selection can also be used to drive multiple transformations so if you want to move your selected points upwards and then rotate them that's fine.

### Step 4: Rinse and repeat...
It's rare that you'll get exactly the shape you want with a single transformation, you'll want to to keep selecting and moving different parts of the mesh.

This can be done by just chaining selections/transformations together.

#### Step 5: Save the result
In a 3d package this is where you save your results to disk ready for rendering, or to import into another tool such as UE4.

In the plugin this is the "Save" nodes.  Again at present there's only one of these, "Update Procedural Mesh Component" (*NB:* This is going to get renamed to Save To Procedrual Mesh Component) which takes the transformed geometry and writes it out to one of Unreal's ProceduralMeshComponents which can then be added to the scene.

# Installing the plugin
When the plugin hits public beta I'll be releasing compiled plugins for Windows and Mac, and possibly other platforms too.

For now though to get it working you'll need the *Plugins/MeshDeformationToolkit* directory from this repo and copy it to the same path in your own project, then when you run UE4 you should be able to enable/compile the plugin (This will need a C++ compiler).

# Node Dictionary
Here is a list of all of the nodes the system provides, broken down into six categories, five of which roughly correspond to the steps listed in the intro and 'Utility' which is a grab-bag of everything else:

1. *Loading Geometry Data*.  Before we do anything else we need some geometry data to operate on.
2. *Select Vertices*.  While we can do some useful operations on every vertex that makes up the mesh the ability to only change parts of the mesh is much more powerful, and for that we need to select the vertices to control the operation.  These selections aren't limited to a simple yes/no, it's possible to have vertices which are partially selected to allow an operation to be applied at varying strengths on different parts of the mesh.
3. *Modifying And Combining Selections*.  For even greater control over which parts of the mesh we're affecting it's possible to take selections and alter them to get the exact control needed.
4. *Transforming Vertices*.  The actual deformations, whether we're moving and rotating points or something stranger this is where the actual work of changing the geometry happens.
5. *Output Data*.  There's no point in changing the geometry if we then don't do something with it and these nodes are where that happens.
6. *Utility*.  This is everything else, including tools which allow you to build your own selection and transform tools.

## Load Geometry Data
All of these nodes are provided on the Mesh Deformation Component, any many of them can also be called on the MeshGeometry directly.

* **Load From Mesh Deformation Component**: Load the geometry from a another MDC, replacing anything currently stored.
* **Load from Mesh Geometry**: Load the geometry stored in a MeshGeometry object, replacing anything currently stored.
* **Load from Static Mesh**: Load geometry from a StaticMesh, replacing anything currently stored.

## Select Vertices
Each of these functions returns a Selection which can then be either be either modified further, or passed into any of the nodes for transforming vertices to control their behavior.

Many of these can be called either on a MeshDeformationComponent, or directly the MeshGeometry that the MDC contains.

* **Select All**: Selects all of the vertices at full strength.
* **Select by Noise**: Select vertices based on a configurable noise function, useful for terrain or adding controlled randomness to a model. *This can return values outside of the standard zero to one range, if this causes problems use a 'Remap By Range' node to remap between zero and one.*
* **Select by Normal**: Select vertices with a given normal facing.
* **Select by Section**: Select all of the vertices in one of the Sections making up a mesh.
* **Select by Texture**: Select vertices based on a channel from a texture.
* **Select by Vertex Range**: Select vertices based on their index in the mesh.  This is most useful when a mesh has been authored to have their vertices in a known order.
* **Select in Volume**: Select vertices based on a channel from a texture
* **Select Linear**: Select vertices with strength blended linearly between two points.
* **Select Near**: Select the vertices near a point in space.
* **Select Near Line**: Select vertices near a line with the provided start/end points.
* **Select Near Spline**: Select all vertices near a [Spline Component](https://docs.unrealengine.com/latest/INT/Engine/BlueprintSplines/Overview/).

## Modifying And Combining Selections
These aren't actually on the Mesh Deformation Component and instead are provided in a Blueprint Function Library.

All of these return a new Selection and don't modify the ones provided to them.  The short descriptions below are often phrased as though they do but this is because writing 'Return a Selection which is the provided Selection' in each one would make them a lot harder to read.

* *Clamp (SelectionSet)*: Clamp all values in a SelectionSet to the minimum and maximum provided.
* *Ease*: Remaps a selection by applying a configurable easing function to each value.  Useful for taking the linear fall-off from a selection and turning it into something smoother and more natural looking.
* *Float - Selection*: Subtract all value in a selection from a constant value, providing a simple reverse and remap.
* *Float / Selection*: Divide a constant value by all values in a selection.
* *Lerp (Selection, Float, Float)*: Blends a Selection between its original value and a constant value with a set strength.
* *Lerp (Selection, Float, Selection)*: Blends a Selection between its original value and a constant value with a strength controlled by a second Selection.
* *Lerp (Selection, Selection, Float)*: Blend two Selections together with a given strength.
* *Lerp (Selection, Selection, Selection)*: Blend two Selections together with a strength controlled by a third Selection.
* *Max (Selection, Float)*: Make sure all values in a Selection are at least equal to a given value.
* *Max (Selection, Selection)*: Combine two Selections together by taking the maximum value from each one of them.
* *Min (Selection, Float)*: Make sure all values in a Selection are at most equal to a given value.
* *Min (Selection, Selection)*: Combined two Selections together by taking the mium value from each of them.
* *Randomize (Selection, Float, Float)*: Randomize the values in a Selection between the minimum and maximum values provided.
* *Remap To Curve (Selection, Curve)*:
* *Remap To Range (Selection, Float, Float)*: Remaps a selection between the minimum and maximum values provided.
* *SelectionSet + Float*: Adds the given value to all values in a Selection.
* *Selection + Selection*: Combine two Selections together by adding their values.
* *Selection - Float*: Subtract a given value from all values in a Selection.
* *Selection - Selection*: Combine two Selections by subtracting all values in the second Selection from the values in the first Selection.
* *Selection * Float*: Multiply all values in a Selection by a given value.
* *Selection * Selection*: Combine two Selections by multiplying their values.
* *Selection / Float*: Divide all of the values in a Selection by the given value.
* *Selection / Selection*: Combine two Selections by dividing all the values from the first Selection by those of the second Selection.
* *Set (Selection, Float)*: Sets all values in a Selection to the given value.

## Transforming Vertices
All of these transform operations can be controlled by providing an optional Selection.  While the actual use of the Selection can vary method nodes it's intended that each one uses it in the most obvious and flexible way for that node's own purpose.

MDC		Yes	Yes
MDC	Inflate	Yes	Yes
MDC	Jitter	Yes	Yes
MDC	Lerp	Yes	Yes
MDC	Lerp Vector	Yes	Yes
MDC	Rotate	Yes	Yes
MDC	Rotate Around Axis	Yes	Yes
MDC	Scale	Yes	Yes
MDC	ScaleAlongAxis	Yes	Yes
MDC	Spherize	Yes	Yes
MDC	Transform	Yes	Yes
MDC	Transform UV	Yes	Yes
MDC	Translate	Yes	Yes

* **Conform**: Conforms the mesh against collision geometry by projecting along a specified vector. This is a difficult node to get to grips with but is very useful for making roads which follow the underlying terrain and similar effects.
* **Conform Down**: Conforms the mesh against collision geometry by projecting downwards (-Z). This is a difficult node to get to grips with but is very useful for making roads which follow the underlying terrain and similar effects.
* **Fit To Spline**: Bend the mesh to follow a [Spline Component](https://docs.unrealengine.com/latest/INT/Engine/BlueprintSplines/Overview/), with controls for the profile of the geometry for more useful effects.  This is more powerful than UE4's own [Spline Mesh Component](https://docs.unrealengine.com/latest/INT/Engine/BlueprintSplines/Overview/) in that it follows an entire curve rather than just having the two control points at the ends, along with additional controls.
* **Flip Normals**: Flip the surface normals.  As it's impossible to 'partly flip' a normal the SelectionSet is used a simple filter here with a flip only happening with weighting >=0.5.
* **Flip Texture UV**: Flip the texture map channel in U (horizontal), V(vertical), both, or neither.
* **Inflate**: Move vertices a specified offset along their own normals.
* **Jitter**: Add random jitter to the position of the vertices.  This is a fairly crude approach and often you'll get better results using *SelectByNoise*.
* **Lerp**: A linear interpolation against the geometry stored in another MeshDeformationComponent.
* **Lerp Vector**: Blend vertices towards the position provided.
* **Rotate**: Rotate the vertices round a specified center using a standard UE4 [Rotator](https://docs.unrealengine.com/latest/INT/BlueprintAPI/Math/Rotator/index.html).
* **Rotate Around Axis**: Rotate vertices around an arbitrary axis.  This is more difficult to use than *Rotate*, but is more flexible in what's possible.
* **Scale**: Scale the mesh using normal XYZ scaling about a specified center.
* **Scale Along Axis**: Scale along an arbitrary axis.
* **Spherize**: Morph geometry into a sphere by moving points along their normals.
* **Transform**: Applies Scale/Rotate/Translate as a single operation using a [Transform](https://docs.unrealengine.com/latest/INT/BlueprintAPI/Math/Transform/index.html).
* **Transform UV**: Apply a [transformation](https://docs.unrealengine.com/latest/INT/BlueprintAPI/Math/Transform/index.html) to the UV mapping, changing the way textures will be mapped
* **Translate**: Move all vertices by the provided vector.

## Output Data
* *Save To Procedural Mesh Component*: Write the geometry data to a standard UE4 [Procedural Mesh Component](https://wiki.unrealengine.com/Procedural_Mesh_Component_in_C%2B%2B:Getting_Started).

## Utility

* *Get Bounding Box*: Return the bounds of the mesh as a [Box](https://docs.unrealengine.com/latest/INT/API/Runtime/Core/Math/FBox/index.html) which.
* *Get Summary*: A simple text string summary of how many sections, vertices and triangles are in the current geometry.
* *Has Geometry*: Returns whether we have any geometry loaded or not
* *Get Total Triangle Count*: Return the total number of triangles in the mesh.
* *Get Total Vertex Count*: Return the total number of vertices in the mesh.

# How It Works
When everything is working and finished I will be taking the time to document how exactly this plugin works, including how you can extend it yourself by writing new selection/transform nodes either in C++ or Blueprint.

For now though I can only point you towards [a tutorial I wrote which started all of this](http://normalvector.com/tutorials/low_poly_world/) which has the basic approach to deforming geometry (In pure Blueprint too!), and add the following quick notes.
* Geometry is basically stored in the format which ProceduralMeshComponent wants, and so a mesh contains a list of sections- each of which has it's own vertex list.  It is this vertex list which the plugin manipulates.
* All any Selection node does is iterate over all of the vertices in a mesh and apply a selection rule to each to obtain the selection weight, storing these in an array in SelectionSet.  The array is ordered the same as the mesh vertices to prevent any other metadata being needed.
* The selection modification nodes just take one (or more) SelectionSets and return another SelectionSet with the weights tweaked as needed.
* Every Transform node is actually currently implemented as a method on the mesh geometry which iterates over the vertices in the mesh and applies the transformation to each of them in order.  Selections are handled by basically acting as a lerp between the original position and the fully transformed position, although the nature of the lerp can vary between methods (Translate has a straightfoward spatial lerp, but Rotate actually lerps the rotational angle for better results).  Most of these are implemented in a Blueprint Library class as they're utility functions rather than being called on a class.
* The MeshDeformationComponent itself just contains a set of mesh geometry and offers BP-callable selection and transform nodes which just delegate the call to the mesh.

# Code documentation
All C++ code has been documented using [Doxygen](http://www.stack.nl/~dimitri/doxygen/) together with [my Doxygen source filter to remove UE4 macros](https://github.com/normalvector/ue4_doxygen_source_filter), and will be made available as API docs online at some point.

# Thanks Go To..
* [Jordan Peck](https://github.com/Auburns) for his splendid [FastNoise](https://github.com/Auburns/FastNoise) C++ library, which is used for all of the noise generation in the plugin.
* [NASA](https://www.nasa.gov/) for the height map used in the "Small World" demo, and generally being too nifty for words.

# ToDo List
The ToDo list for this project is on [Trello](https://trello.com/b/eWCcepmW/mesh-deformation-component).

This also doubles as a changelog with the 'Done' list.

# LICENSE ([MIT License](https://en.wikipedia.org/wiki/MIT_License))

Copyright 2017 Paul Golds.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
