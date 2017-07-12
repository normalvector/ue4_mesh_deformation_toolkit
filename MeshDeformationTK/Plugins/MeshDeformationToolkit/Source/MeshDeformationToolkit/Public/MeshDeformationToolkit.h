// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

/// The interface for loading and unloading the toolkit
/// \todo Think about renaming the whole thing to a smaller-scoped name
/// \todo Think about putting all BP functions in a global category, "Mesh Deformer|Subcategory"
/// \todo Go through code and check limits are enforced
/// \todo Go through code and check error messages are presented where applicable

class FMeshDeformationToolkitModule : public IModuleInterface
{
public:

	/// Called when the module is loaded into memory
	virtual void StartupModule() override;

	/// Called when the module is removed from memory
	virtual void ShutdownModule() override;
};