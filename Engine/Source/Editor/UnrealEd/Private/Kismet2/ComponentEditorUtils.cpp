// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Editor/UnrealEd/Public/Kismet2/ComponentEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Runtime/Engine/Classes/Components/SceneComponent.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"
#include "Factories.h"
#include "UnrealExporter.h"
#include "GenericCommands.h"
#include "SourceCodeNavigation.h"
#include "ClassIconFinder.h"
#include "AssetEditorManager.h"

#define LOCTEXT_NAMESPACE "ComponentEditorUtils"

// Text object factory for pasting components
struct FComponentObjectTextFactory : public FCustomizableTextObjectFactory
{
	// Child->Parent name map
	TMap<FName, FName> ParentMap;

	// Name->Instance object mapping
	TMap<FName, UActorComponent*> NewObjectMap;

	// Determine whether or not scene components in the new object set can be attached to the given scene root component
	bool CanAttachComponentsTo(USceneComponent* InRootComponent)
	{
		check(InRootComponent);

		// For each component in the set, check against the given root component and break if we fail to validate
		bool bCanAttachToRoot = true;
		for (auto NewComponentIt = NewObjectMap.CreateConstIterator(); NewComponentIt && bCanAttachToRoot; ++NewComponentIt)
		{
			// If this is a scene component, and it does not already have a parent within the set
			USceneComponent* SceneComponent = Cast<USceneComponent>(NewComponentIt->Value);
			if (SceneComponent != NULL && !ParentMap.Contains(SceneComponent->GetFName()))
			{
				// Determine if we are allowed to attach the scene component to the given root component
				bCanAttachToRoot = InRootComponent->CanAttachAsChild(SceneComponent, NAME_None)
					&& SceneComponent->Mobility >= InRootComponent->Mobility
					&& ( !InRootComponent->IsEditorOnly() || SceneComponent->IsEditorOnly() );
			}
		}

		return bCanAttachToRoot;
	}

	// Constructs a new object factory from the given text buffer
	static TSharedRef<FComponentObjectTextFactory> Get(const FString& InTextBuffer, bool bPasteAsArchetypes = false)
	{
		// Construct a new instance
		TSharedPtr<FComponentObjectTextFactory> FactoryPtr = MakeShareable(new FComponentObjectTextFactory());
		check(FactoryPtr.IsValid());

		// Create new objects if we're allowed to
		if (FactoryPtr->CanCreateObjectsFromText(InTextBuffer))
		{
			EObjectFlags ObjectFlags = RF_Transactional;
			if (bPasteAsArchetypes)
			{
				ObjectFlags |= RF_ArchetypeObject;
			}

			// Use the transient package initially for creating the objects, since the variable name is used when copying
			FactoryPtr->ProcessBuffer(GetTransientPackage(), ObjectFlags, InTextBuffer);
		}

		return FactoryPtr.ToSharedRef();
	}

	virtual ~FComponentObjectTextFactory() {}

protected:
	// Constructor; protected to only allow this class to instance itself
	FComponentObjectTextFactory()
		:FCustomizableTextObjectFactory(GWarn)
	{
	}

	// FCustomizableTextObjectFactory implementation

	virtual bool CanCreateClass(UClass* ObjectClass) const override
	{
		// Only allow actor component types to be created
		return ObjectClass->IsChildOf(UActorComponent::StaticClass());
	}

	virtual void ProcessConstructedObject(UObject* NewObject) override
	{
		check(NewObject);

		// Add it to the new object map
		NewObjectMap.Add(NewObject->GetFName(), Cast<UActorComponent>(NewObject));

		// If this is a scene component and it has a parent
		USceneComponent* SceneComponent = Cast<USceneComponent>(NewObject);
		if (SceneComponent && SceneComponent->AttachParent)
		{
			// Add an entry to the child->parent name map
			ParentMap.Add(NewObject->GetFName(), SceneComponent->AttachParent->GetFName());

			// Clear this so it isn't used when constructing the new SCS node
			SceneComponent->AttachParent = NULL;
		}
	}

	// FCustomizableTextObjectFactory (end)
};

bool FComponentEditorUtils::IsValidVariableNameString(const UActorComponent* InComponent, const FString& InString)
{
	// First test to make sure the string is not empty and does not equate to the DefaultSceneRoot node name
	bool bIsValid = !InString.IsEmpty() && !InString.Equals(USceneComponent::GetDefaultSceneRootVariableName().ToString());
	if(bIsValid && InComponent != NULL)
	{
		// Next test to make sure the string doesn't conflict with the format that MakeUniqueObjectName() generates
		const FString ClassNameThatWillBeUsedInGenerator = FBlueprintEditorUtils::GetClassNameWithoutSuffix(InComponent->GetClass());
		const FString MakeUniqueObjectNamePrefix = FString::Printf(TEXT("%s_"), *ClassNameThatWillBeUsedInGenerator);
		if(InString.StartsWith(MakeUniqueObjectNamePrefix))
		{
			bIsValid = !InString.Replace(*MakeUniqueObjectNamePrefix, TEXT("")).IsNumeric();
		}
	}

	return bIsValid;
}

bool FComponentEditorUtils::IsComponentNameAvailable(const FString& InString, AActor* ComponentOwner, const UActorComponent* ComponentToIgnore)
{
	UObject* Object = FindObjectFast<UObject>(ComponentOwner, *InString);

	bool bNameIsAvailable = Object == nullptr || Object == ComponentToIgnore;

	return bNameIsAvailable;
}

FString FComponentEditorUtils::GenerateValidVariableName(TSubclassOf<UActorComponent> ComponentClass, AActor* ComponentOwner)
{
	check(ComponentOwner);

	FString ComponentTypeName = FBlueprintEditorUtils::GetClassNameWithoutSuffix(ComponentClass);

	// Strip off 'Component' if the class ends with that.  It just looks better in the UI.
	FString SuffixToStrip( TEXT( "Component" ) );
	if( ComponentTypeName.EndsWith( SuffixToStrip ) )
	{
		ComponentTypeName = ComponentTypeName.Left( ComponentTypeName.Len() - SuffixToStrip.Len() );
	}

	// Strip off 'Actor' if the class ends with that so as not to confuse actors with components
	SuffixToStrip = TEXT( "Actor" );
	if( ComponentTypeName.EndsWith( SuffixToStrip ) )
	{
		ComponentTypeName = ComponentTypeName.Left( ComponentTypeName.Len() - SuffixToStrip.Len() );
	}

	// Try to create a name without any numerical suffix first
	int32 Counter = 1;
	FString ComponentInstanceName = ComponentTypeName;
	while (!IsComponentNameAvailable(ComponentInstanceName, ComponentOwner))
	{
		// Assign the lowest possible numerical suffix
		ComponentInstanceName = FString::Printf(TEXT("%s%d"), *ComponentTypeName, Counter++);
	}

	return ComponentInstanceName;
}

FString FComponentEditorUtils::GenerateValidVariableNameFromAsset(UObject* Asset, AActor* ComponentOwner)
{
	int32 Counter = 1;
	FString AssetName = Asset->GetName();

	UClass* Class = Cast<UClass>(Asset);
	if (Class)
	{
		if (!Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
		{
			AssetName.RemoveFromEnd(TEXT("Component"));
		}
		else
		{
			AssetName.RemoveFromEnd("_C");
		}
	}

	// Try to create a name without any numerical suffix first
	FString ComponentInstanceName = AssetName;

	if (ComponentOwner)
	{
		while (!IsComponentNameAvailable(ComponentInstanceName, ComponentOwner))
		{
			// Assign the lowest possible numerical suffix
			ComponentInstanceName = FString::Printf(TEXT("%s%d"), *AssetName, Counter++);
		}
	}

	return ComponentInstanceName;
}

USceneComponent* FComponentEditorUtils::FindClosestParentInList(UActorComponent* ChildComponent, const TArray<UActorComponent*>& ComponentList)
{
	USceneComponent* ClosestParentComponent = nullptr;
	for (auto Component : ComponentList)
	{
		auto ChildAsScene = Cast<USceneComponent>(ChildComponent);
		auto SceneComponent = Cast<USceneComponent>(Component);
		if (ChildAsScene && SceneComponent)
		{
			// Check to see if any parent is also in the list
			USceneComponent* Parent = ChildAsScene->GetAttachParent();
			while (Parent != nullptr)
			{
				if (ComponentList.Contains(Parent))
				{
					ClosestParentComponent = SceneComponent;
					break;
				}

				Parent = Parent->GetAttachParent();
			}
		}
	}

	return ClosestParentComponent;
}

bool FComponentEditorUtils::CanCopyComponents(const TArray<UActorComponent*>& ComponentsToCopy)
{
	bool bCanCopy = ComponentsToCopy.Num() > 0;
	if (bCanCopy)
	{
		for (int32 i = 0; i < ComponentsToCopy.Num() && bCanCopy; ++i)
		{
			// Check for the default scene root; that cannot be copied/duplicated
			UActorComponent* Component = ComponentsToCopy[i];
			bCanCopy = Component != nullptr && Component->GetFName() != USceneComponent::GetDefaultSceneRootVariableName();
			if (bCanCopy)
			{
				UClass* ComponentClass = Component->GetClass();
				check(ComponentClass != nullptr);

				// Component class cannot be abstract and must also be tagged as BlueprintSpawnable
				bCanCopy = !ComponentClass->HasAnyClassFlags(CLASS_Abstract)
					&& ComponentClass->HasMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent);
			}
		}
	}

	return bCanCopy;
}

void FComponentEditorUtils::CopyComponents(const TArray<UActorComponent*>& ComponentsToCopy)
{
	FStringOutputDevice Archive;
	const FExportObjectInnerContext Context;

	// Clear the mark state for saving.
	UnMarkAllObjects(EObjectMark(OBJECTMARK_TagExp | OBJECTMARK_TagImp));

	// Duplicate the selected component templates into temporary objects that we can modify
	TMap<FName, FName> ParentMap;
	TMap<FName, UActorComponent*> ObjectMap;
	for (auto Component : ComponentsToCopy)
	{
		// Duplicate the component into a temporary object
		UObject* DuplicatedComponent = StaticDuplicateObject(Component, GetTransientPackage(), *Component->GetName(), RF_AllFlags & ~RF_ArchetypeObject);
		if (DuplicatedComponent)
		{
			// Find the closest parent component of the current component within the list of components to copy
			USceneComponent* ClosestSelectedParent = FindClosestParentInList(Component, ComponentsToCopy);
			if (ClosestSelectedParent)
			{
				// If the parent is included in the list, record it into the node->parent map
				ParentMap.Add(Component->GetFName(), ClosestSelectedParent->GetFName());
			}

			// Record the temporary object into the name->object map
			ObjectMap.Add(Component->GetFName(), CastChecked<UActorComponent>(DuplicatedComponent));
		}
	}

	// Export the component object(s) to text for copying
	for (auto ObjectIt = ObjectMap.CreateIterator(); ObjectIt; ++ObjectIt)
	{
		// Get the component object to be copied
		UActorComponent* ComponentToCopy = ObjectIt->Value;
		check(ComponentToCopy);

		// If this component object had a parent within the selected set
		if (ParentMap.Contains(ComponentToCopy->GetFName()))
		{
			// Get the name of the parent component
			FName ParentName = ParentMap[ComponentToCopy->GetFName()];
			if (ObjectMap.Contains(ParentName))
			{
				// Ensure that this component is a scene component
				USceneComponent* SceneComponent = Cast<USceneComponent>(ComponentToCopy);
				if (SceneComponent)
				{
					// Set the attach parent to the matching parent object in the temporary set. This allows us to preserve hierarchy in the copied set.
					SceneComponent->AttachParent = Cast<USceneComponent>(ObjectMap[ParentName]);
				}
			}
		}

		// Export the component object to the given string
		UExporter::ExportToOutputDevice(&Context, ComponentToCopy, NULL, Archive, TEXT("copy"), 0, PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited, false, ComponentToCopy->GetOuter());
	}

	// Copy text to clipboard
	FString ExportedText = Archive;
	FPlatformMisc::ClipboardCopy(*ExportedText);
}

bool FComponentEditorUtils::CanPasteComponents(USceneComponent* RootComponent, bool bOverrideCanAttach)
{
	FString ClipboardContent;
	FPlatformMisc::ClipboardPaste(ClipboardContent);

	// Obtain the component object text factory for the clipboard content and return whether or not we can use it
	TSharedRef<FComponentObjectTextFactory> Factory = FComponentObjectTextFactory::Get(ClipboardContent);
	return Factory->NewObjectMap.Num() > 0 && ( bOverrideCanAttach || Factory->CanAttachComponentsTo(RootComponent) );
}

void FComponentEditorUtils::PasteComponents(TArray<UActorComponent*>& OutPastedComponents, AActor* TargetActor, USceneComponent* TargetComponent)
{
	check(TargetActor);

	// Get the text from the clipboard
	FString TextToImport;
	FPlatformMisc::ClipboardPaste(TextToImport);

	// Get a new component object factory for the clipboard content
	TSharedRef<FComponentObjectTextFactory> Factory = FComponentObjectTextFactory::Get(TextToImport);

	TargetActor->Modify();

	USceneComponent* TargetParent = TargetComponent ? TargetComponent->GetAttachParent() : nullptr;
	for (auto& NewObjectPair : Factory->NewObjectMap)
	{
		// Get the component object instance
		UActorComponent* NewActorComponent = NewObjectPair.Value;
		check(NewActorComponent);

		// Relocate the instance from the transient package to the Actor and assign it a unique object name
		FString NewComponentName = FComponentEditorUtils::GenerateValidVariableName(NewActorComponent->GetClass(), TargetActor);
		NewActorComponent->Rename(*NewComponentName, TargetActor, REN_DontCreateRedirectors | REN_DoNotDirty);

		if (auto NewSceneComponent = Cast<USceneComponent>(NewActorComponent))
		{
			// Default to attaching to the target component's parent if possible, otherwise attach to the root
			USceneComponent* NewComponentParent = TargetParent ? TargetParent : TargetActor->GetRootComponent();
			
			// Check to see if there's an entry for the current component in the set of parent components
			if (Factory->ParentMap.Contains(NewObjectPair.Key))
			{
				// Get the parent component name
				FName ParentName = Factory->ParentMap[NewObjectPair.Key];
				if (Factory->NewObjectMap.Contains(ParentName))
				{
					// The parent should by definition be a scene component
					NewComponentParent = CastChecked<USceneComponent>(Factory->NewObjectMap[ParentName]);
				}
			}

			//@todo: Fix pasting when the pasted component was a root
			//NewSceneComponent->UpdateComponentToWorld();
			if (NewComponentParent)
			{
				// Reattach the current node to the parent node
				NewSceneComponent->AttachTo(NewComponentParent, NAME_None/*, EAttachLocation::KeepWorldPosition*/);
			}
			else
			{
				// There is no root component and this component isn't the child of another component in the map, so make it the root
				TargetActor->SetRootComponent(NewSceneComponent);
			}
		}

		TargetActor->AddInstanceComponent(NewActorComponent);
		NewActorComponent->RegisterComponent();

		OutPastedComponents.Add(NewActorComponent);
	}
}

void FComponentEditorUtils::GetComponentsFromClipboard(TMap<FName, FName>& OutParentMap, TMap<FName, UActorComponent*>& OutNewObjectMap, bool bGetComponentsAsArchetypes)
{
	// Get the text from the clipboard
	FString TextToImport;
	FPlatformMisc::ClipboardPaste(TextToImport);

	// Get a new component object factory for the clipboard content
	TSharedRef<FComponentObjectTextFactory> Factory = FComponentObjectTextFactory::Get(TextToImport, bGetComponentsAsArchetypes);

	// Return the created component mappings
	OutParentMap = Factory->ParentMap;
	OutNewObjectMap = Factory->NewObjectMap;
}

bool FComponentEditorUtils::CanDeleteComponents(const TArray<UActorComponent*>& ComponentsToDelete)
{
	bool bCanDelete = true;
	for (auto ComponentToDelete : ComponentsToDelete)
	{
		// We can't delete non-instance components or the default scene root
		if (ComponentToDelete->CreationMethod != EComponentCreationMethod::Instance 
			|| ComponentToDelete->GetFName() == USceneComponent::GetDefaultSceneRootVariableName())
		{
			bCanDelete = false;
			break;
		}
	}

	return bCanDelete;
}

int32 FComponentEditorUtils::DeleteComponents(const TArray<UActorComponent*>& ComponentsToDelete, UActorComponent*& OutComponentToSelect)
{
	int32 NumDeletedComponents = 0;

	for (auto ComponentToDelete : ComponentsToDelete)
	{
		if (ComponentToDelete->CreationMethod != EComponentCreationMethod::Instance)
		{
			// We can only delete instance components, so retain selection on the un-deletable component
			OutComponentToSelect = ComponentToDelete;
			continue;
		}

		// If necessary, determine the component that should be selected following the deletion of the indicated component
		if (!OutComponentToSelect || ComponentToDelete == OutComponentToSelect)
		{
			USceneComponent* RootComponent = ComponentToDelete->GetOwner()->GetRootComponent();
			if (RootComponent != ComponentToDelete)
			{
				// Worst-case, the root can be selected
				OutComponentToSelect = RootComponent;

				if (auto ComponentToDeleteAsSceneComp = Cast<USceneComponent>(ComponentToDelete))
				{
					if (USceneComponent* ParentComponent = ComponentToDeleteAsSceneComp->GetAttachParent())
					{
						// The component to delete has a parent, so we select that in the absence of an appropriate sibling
						OutComponentToSelect = ParentComponent;

						// Try to select the sibling that immediately precedes the component to delete
						TArray<USceneComponent*> Siblings;
						ParentComponent->GetChildrenComponents(false, Siblings);
						for (int32 i = 0; i < Siblings.Num() && ComponentToDelete != Siblings[i]; ++i)
						{
							if (!Siblings[i]->IsPendingKill())
							{
								OutComponentToSelect = Siblings[i];
							}
						}
					}
				}
				else
				{
					// For a non-scene component, try to select the preceding non-scene component
					TInlineComponentArray<UActorComponent*> ActorComponents;
					ComponentToDelete->GetOwner()->GetComponents(ActorComponents);
					for (int32 i = 0; i < ActorComponents.Num() && ComponentToDelete != ActorComponents[i]; ++i)
					{
						if (!ActorComponents[i]->IsA(USceneComponent::StaticClass()))
						{
							OutComponentToSelect = ActorComponents[i];
						}
					}
				}
			}
			else
			{
				OutComponentToSelect = nullptr;
			}
		}

		// Actually delete the component
		ComponentToDelete->Modify();
		ComponentToDelete->DestroyComponent(true);
		NumDeletedComponents++;
	}

	return NumDeletedComponents;
}

UActorComponent* FComponentEditorUtils::DuplicateComponent(UActorComponent* TemplateComponent)
{
	check(TemplateComponent);

	UActorComponent* NewCloneComponent = nullptr;
	AActor* Actor = TemplateComponent->GetOwner();
	if (!TemplateComponent->IsEditorOnly() && Actor)
	{
		Actor->Modify();
		UClass* ComponentClass = TemplateComponent->GetClass();
		FName NewComponentName = *FComponentEditorUtils::GenerateValidVariableName(ComponentClass, Actor);

		bool bKeepWorldLocationOnAttach = false;

		const bool bTemplateTransactional = TemplateComponent->HasAllFlags(RF_Transactional);
		TemplateComponent->SetFlags(RF_Transactional);

		NewCloneComponent = DuplicateObject<UActorComponent>(TemplateComponent, Actor, *NewComponentName.ToString() );
		
		if (!bTemplateTransactional)
		{
			TemplateComponent->ClearFlags(RF_Transactional);
		}
			
		USceneComponent* NewSceneComponent = Cast<USceneComponent>(NewCloneComponent);
		if (NewSceneComponent)
		{
			// Ensure the clone doesn't think it has children
			NewSceneComponent->AttachChildren.Empty();

			// If the clone is a scene component without an attach parent, attach it to the root (can happen when duplicating the root component)
			if (!NewSceneComponent->GetAttachParent())
			{
				USceneComponent* RootComponent = Actor->GetRootComponent();
				check(RootComponent);

				// ComponentToWorld is not a UPROPERTY, so make sure the clone has calculated it properly before attachment
				NewSceneComponent->UpdateComponentToWorld();

				NewSceneComponent->AttachTo(RootComponent, NAME_None, EAttachLocation::KeepWorldPosition);
			}
		}

		NewCloneComponent->OnComponentCreated();

		// Add to SerializedComponents array so it gets saved
		Actor->AddInstanceComponent(NewCloneComponent);
		
		// Register the new component
		NewCloneComponent->RegisterComponent();
	}

	return NewCloneComponent;
}

void FComponentEditorUtils::AdjustComponentDelta(USceneComponent* Component, FVector& Drag, FRotator& Rotation)
{
	USceneComponent* ParentSceneComp = Component->GetAttachParent();
	if (ParentSceneComp)
	{
		const FTransform ParentToWorldSpace = ParentSceneComp->GetSocketTransform(Component->AttachSocketName);

		if (!Component->bAbsoluteLocation)
		{
			Drag = ParentToWorldSpace.Inverse().TransformVector(Drag);
		}

		if (!Component->bAbsoluteRotation)
		{
			Rotation = ( ParentToWorldSpace.Inverse().GetRotation() * Rotation.Quaternion() * ParentToWorldSpace.GetRotation() ).Rotator();
		}
	}
}

void FComponentEditorUtils::BindComponentSelectionOverride(USceneComponent* SceneComponent, bool bBind)
{
	if (SceneComponent)
	{
		// If the scene component is a primitive component, set the override for it
		auto PrimComponent = Cast<UPrimitiveComponent>(SceneComponent);
		if (PrimComponent && PrimComponent->SelectionOverrideDelegate.IsBound() != bBind)
		{
			if (bBind)
			{
				PrimComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateUObject(GUnrealEd, &UUnrealEdEngine::IsComponentSelected);
			}
			else
			{
				PrimComponent->SelectionOverrideDelegate.Unbind();
			}
		}
		else
		{
			// Otherwise, make sure the override is set properly on any attached editor-only primitive components (ex: billboards)
			for (auto Component : SceneComponent->AttachChildren)
			{
				PrimComponent = Cast<UPrimitiveComponent>(Component);
				if (PrimComponent && PrimComponent->IsEditorOnly() && PrimComponent->SelectionOverrideDelegate.IsBound() != bBind)
				{
					if (bBind)
					{
						PrimComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateUObject(GUnrealEd, &UUnrealEdEngine::IsComponentSelected);
					}
					else
					{
						PrimComponent->SelectionOverrideDelegate.Unbind();
					}
				}
			}
		}
	}
}

bool FComponentEditorUtils::AttemptApplyMaterialToComponent(USceneComponent* SceneComponent, UMaterialInterface* MaterialToApply, int32 OptionalMaterialSlot)
{
	bool bResult = false;
	
	auto MeshComponent = Cast<UMeshComponent>(SceneComponent);
	auto DecalComponent = Cast<UDecalComponent>(SceneComponent);

	// We can only apply a material to a mesh or a decal
	if (MeshComponent || DecalComponent)
	{
		bResult = true;
		const FScopedTransaction Transaction(LOCTEXT("DropTarget_UndoSetComponentMaterial", "Assign Material to Component (Drag and Drop)"));
		SceneComponent->Modify();

		if (MeshComponent)
		{
			// OK, we need to figure out how many material slots this mesh component/static mesh has.
			// Start with the actor's material count, then drill into the static/skeletal mesh to make sure 
			// we have the right total.
			int32 MaterialCount = FMath::Max(MeshComponent->OverrideMaterials.Num(), MeshComponent->GetNumMaterials());

			// Do we have an overridable material at the appropriate slot?
			if (MaterialCount > 0 && OptionalMaterialSlot < MaterialCount)
			{
				if (OptionalMaterialSlot == -1)
				{
					// Apply the material to every slot.
					for (int32 CurMaterialIndex = 0; CurMaterialIndex < MaterialCount; ++CurMaterialIndex)
					{
						MeshComponent->SetMaterial(CurMaterialIndex, MaterialToApply);
					}
				}
				else
				{
					// Apply only to the indicated slot.
					MeshComponent->SetMaterial(OptionalMaterialSlot, MaterialToApply);
				}
			}
		}
		else
		{
			DecalComponent->SetMaterial(0, MaterialToApply);
		}

		SceneComponent->MarkRenderStateDirty();
	}

	return bResult;
}

FName FComponentEditorUtils::FindVariableNameGivenComponentInstance(UActorComponent* ComponentInstance)
{
	check(ComponentInstance != nullptr);

	// First see if the name just works
	if (AActor* OwnerActor = ComponentInstance->GetOwner())
	{
		UClass* OwnerActorClass = OwnerActor->GetClass();
		if (UObjectProperty* TestProperty = FindField<UObjectProperty>(OwnerActorClass, ComponentInstance->GetFName()))
		{
			if (ComponentInstance->GetClass()->IsChildOf(TestProperty->PropertyClass))
			{
				return TestProperty->GetFName();
			}
		}
	}

	// Name mismatch, try finding a differently named variable pointing to the the component (the mismatch should only be possible for native components)
	if (UActorComponent* Archetype = Cast<UActorComponent>(ComponentInstance->GetArchetype()))
	{
		if (AActor* OwnerActor = Archetype->GetOwner())
		{
			UClass* OwnerClass = OwnerActor->GetClass();
			AActor* OwnerCDO = CastChecked<AActor>(OwnerClass->GetDefaultObject());
			check(OwnerCDO->HasAnyFlags(RF_ClassDefaultObject));

			for (TFieldIterator<UObjectProperty> PropIt(OwnerClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				UObjectProperty* TestProperty = *PropIt;
				if (Archetype->GetClass()->IsChildOf(TestProperty->PropertyClass))
				{
					void* TestPropertyInstanceAddress = TestProperty->ContainerPtrToValuePtr<void>(OwnerCDO);
					UObject* ObjectPointedToByProperty = TestProperty->GetObjectPropertyValue(TestPropertyInstanceAddress);
					if (ObjectPointedToByProperty == Archetype)
					{
						// This property points to the component archetype, so it's an anchor even if it was named wrong
						return TestProperty->GetFName();
					}
				}
			}
		}
	}

	return NAME_None;
}

void FComponentEditorUtils::FillComponentContextMenuOptions(FMenuBuilder& MenuBuilder, const TArray<UActorComponent*>& SelectedComponents)
{
	// Basic commands
	MenuBuilder.BeginSection("EditComponent", LOCTEXT("EditComponentHeading", "Edit"));
	{
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename);
	}
	MenuBuilder.EndSection();

	if (SelectedComponents.Num() == 1)
	{
		UActorComponent* Component = SelectedComponents[0];

		if (Component->GetClass()->ClassGeneratedBy)
		{
			MenuBuilder.BeginSection("ComponentAsset", LOCTEXT("ComponentAssetHeading", "Asset"));
			{
				MenuBuilder.AddMenuEntry(
					FText::Format(LOCTEXT("GoToBlueprintForComponent", "Edit {0}"), FText::FromString(Component->GetClass()->ClassGeneratedBy->GetName())),
					LOCTEXT("EditBlueprintForComponent_ToolTip", "Edits the Blueprint Class that defines this component."),
					FSlateIcon(FEditorStyle::GetStyleSetName(), FClassIconFinder::FindIconNameForClass(Component->GetClass())),
					FUIAction(
					FExecuteAction::CreateStatic(&FComponentEditorUtils::OnEditBlueprintComponent, Component->GetClass()->ClassGeneratedBy),
					FCanExecuteAction()));

				MenuBuilder.AddMenuEntry(
					LOCTEXT("GoToAssetForComponent", "Find Class in Content Browser"),
					LOCTEXT("GoToAssetForComponent_ToolTip", "Summons the content browser and goes to the class for this component."),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "SystemWideCommands.FindInContentBrowser"),
					FUIAction(
					FExecuteAction::CreateStatic(&FComponentEditorUtils::OnGoToComponentAssetInBrowser, Component->GetClass()->ClassGeneratedBy),
					FCanExecuteAction()));
			}
			MenuBuilder.EndSection();
		}
		else
		{
			MenuBuilder.BeginSection("ComponentCode", LOCTEXT("ComponentCodeHeading", "C++"));
			{
				if (FSourceCodeNavigation::IsCompilerAvailable())
				{
					FString ClassHeaderPath;
					if (FSourceCodeNavigation::FindClassHeaderPath(Component->GetClass(), ClassHeaderPath) && IFileManager::Get().FileSize(*ClassHeaderPath) != INDEX_NONE)
					{
						const FString CodeFileName = FPaths::GetCleanFilename(*ClassHeaderPath);

						MenuBuilder.AddMenuEntry(
							FText::Format(LOCTEXT("GoToCodeForComponent", "Open {0}"), FText::FromString(CodeFileName)),
							FText::Format(LOCTEXT("GoToCodeForComponent_ToolTip", "Opens the header file for this component ({0}) in a code editing program"), FText::FromString(CodeFileName)),
							FSlateIcon(),
							FUIAction(
							FExecuteAction::CreateStatic(&FComponentEditorUtils::OnOpenComponentCodeFile, ClassHeaderPath),
							FCanExecuteAction()));
					}

					MenuBuilder.AddMenuEntry(
						LOCTEXT("GoToAssetForComponent", "Find Class in Content Browser"),
						LOCTEXT("GoToAssetForComponent_ToolTip", "Summons the content browser and goes to the class for this component."),
						FSlateIcon(FEditorStyle::GetStyleSetName(), "SystemWideCommands.FindInContentBrowser"),
						FUIAction(
						FExecuteAction::CreateStatic(&FComponentEditorUtils::OnGoToComponentAssetInBrowser, (UObject*)Component->GetClass()),
						FCanExecuteAction()));
				}
			}
			MenuBuilder.EndSection();
		}
	}
}

void FComponentEditorUtils::OnGoToComponentAssetInBrowser(UObject* Asset)
{
	TArray<UObject*> Objects;
	Objects.Add(Asset);
	GEditor->SyncBrowserToObjects(Objects);
}

void FComponentEditorUtils::OnOpenComponentCodeFile(const FString CodeFileName)
{
	const FString AbsoluteHeaderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*CodeFileName);
	FSourceCodeNavigation::OpenSourceFile(AbsoluteHeaderPath);
}

void FComponentEditorUtils::OnEditBlueprintComponent(UObject* Blueprint)
{
	FAssetEditorManager::Get().OpenEditorForAsset(Blueprint);
}

#undef LOCTEXT_NAMESPACE