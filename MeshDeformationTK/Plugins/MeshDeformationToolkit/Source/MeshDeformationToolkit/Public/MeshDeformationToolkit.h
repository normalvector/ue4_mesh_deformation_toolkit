// (c)2017 Paul Golds, released under MIT License.

#pragma once

#include "ModuleManager.h"

// All log messages will be passed through this logger
DECLARE_LOG_CATEGORY_EXTERN(MDTLog, Log, All);

/// The interface for loading and unloading the toolkit
class FMeshDeformationToolkitModule: public IModuleInterface
{
public:

	/// Called when the module is loaded into memory
	virtual void StartupModule() override;

	/// Called when the module is removed from memory
	virtual void ShutdownModule() override;
};