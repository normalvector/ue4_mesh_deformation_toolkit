// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "ModuleManager.h"

// All log messages will be passed through this logger
DECLARE_LOG_CATEGORY_EXTERN(MDTLog, Log, All);

/// \mainpage Mesh Deformation Toolkit
///
/// This is the API documentation for the Mesh Deformation Toolkit, which is
/// an Unreal Engine 4 plugin to allow meshes to be deformed inside of the engine.
///
/// The plugin itself, along with the main documentation, is in
/// [this GitHub repo](https://github.com/normalvector/ue4_mesh_deformation_toolkit).

/// The interface for loading and unloading the toolkit
class FMeshDeformationToolkitModule: public IModuleInterface
{
public:

	/// Called when the module is loaded into memory
	virtual void StartupModule() override;

	/// Called when the module is removed from memory
	virtual void ShutdownModule() override;
};