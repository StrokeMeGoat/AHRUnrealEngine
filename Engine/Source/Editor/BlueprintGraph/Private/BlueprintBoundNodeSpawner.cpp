// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintBoundNodeSpawner.h"

#define LOCTEXT_NAMESPACE "BlueprintBoundNodeSpawner"

/*******************************************************************************
 * UBlueprintBoundNodeSpawner
 ******************************************************************************/

//------------------------------------------------------------------------------
UBlueprintBoundNodeSpawner* UBlueprintBoundNodeSpawner::Create(TSubclassOf<UEdGraphNode> NodeClass, UObject* Outer/* = nullptr*/)
{
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UBlueprintBoundNodeSpawner* NodeSpawner = NewObject<UBlueprintBoundNodeSpawner>(Outer);
	NodeSpawner->NodeClass = NodeClass;

	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintBoundNodeSpawner::UBlueprintBoundNodeSpawner(class FPostConstructInitializeProperties const& PCIP)
	: Super(PCIP)
{
}

//------------------------------------------------------------------------------
bool UBlueprintBoundNodeSpawner::IsBindingCompatible(UObject const* BindingCandidate) const
{
	if(CanBindObjectDelegate.IsBound())
	{
		return CanBindObjectDelegate.Execute(BindingCandidate);
	}
	return false;
}

//------------------------------------------------------------------------------
bool UBlueprintBoundNodeSpawner::BindToNode(UEdGraphNode* Node, UObject* Binding) const
{
	if(OnBindObjectDelegate.IsBound())
	{
		return OnBindObjectDelegate.Execute(Node, Binding);
	}
	return false;
}

#undef LOCTEXT_NAMESPACE