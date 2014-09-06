// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "GraphActionNode.h"

/*******************************************************************************
 * Static FGraphActionNode Helpers
 ******************************************************************************/

struct FGraphActionNodeImpl
{
	static const int32 DEFAULT_GROUPING = 0;

	/**
	 * Utility sort function. Compares nodes based off of section, grouping, and
	 * type.
	 * 
	 * @param  LhsMenuNodePtr	The node to determine if it should come first.
	 * @param  RhsMenuNodePtr	The node to determine if it should come second.
	 * @return True if LhsMenuNodePtr should come before RhsMenuNodePtr.
	 */
	static bool NodeCompare(TSharedPtr<FGraphActionNode> const& LhsMenuNodePtr, TSharedPtr<FGraphActionNode> const& RhsMenuNodePtr);

	/**
	 * Utility sort function. Compares nodes based off of section, grouping, 
	 * type, and then alphabetically.
	 * 
	 * @param  LhsMenuNodePtr	The node to determine if it should come first.
	 * @param  RhsMenuNodePtr	The node to determine if it should come second.
	 * @return True if LhsMenuNodePtr should come before RhsMenuNodePtr.
	 */
	static bool AlphabeticalNodeCompare(TSharedPtr<FGraphActionNode> const& LhsMenuNodePtr, TSharedPtr<FGraphActionNode> const& RhsMenuNodePtr);
};

//------------------------------------------------------------------------------
bool FGraphActionNodeImpl::NodeCompare(TSharedPtr<FGraphActionNode> const& LhsMenuNodePtr, TSharedPtr<FGraphActionNode> const& RhsMenuNodePtr)
{
	FGraphActionNode* LhsMenuNode = LhsMenuNodePtr.Get();
	FGraphActionNode* RhsMenuNode = RhsMenuNodePtr.Get();

	bool const bLhsIsCategory  = LhsMenuNode->IsCategoryNode();
	bool const bRhsIsCategory  = RhsMenuNode->IsCategoryNode();
	bool const bLhsIsSeparator = LhsMenuNode->IsSeparator();
	bool const bRhsIsSeparator = RhsMenuNode->IsSeparator();
	bool const bLhsIsSectionHeader = LhsMenuNode->IsSectionHeadingNode();
	bool const bRhsIsSectionHeader = RhsMenuNode->IsSectionHeadingNode();

	if (LhsMenuNode->SectionID != RhsMenuNode->SectionID)
	{
		// since we don't add section headers for children that have the same
		// section as their parents (the header is above the parent), we need to
		// organize them first (so they're seemingly under the same header)
		if ((LhsMenuNode->SectionID == LhsMenuNode->ParentNode.Pin()->SectionID) && 
			(LhsMenuNode->SectionID != FGraphActionNode::INVALID_SECTION_ID))
		{
			return true;
		}
		else // otherwise...
		{
			// sections are ordered in ascending order
			return (LhsMenuNode->SectionID < RhsMenuNode->SectionID);
		}		
	}
	else if (bLhsIsSectionHeader != bRhsIsSectionHeader)
	{
		// section headers go to the top of that section
		return bLhsIsSectionHeader;
	}
	else if (LhsMenuNode->Grouping != RhsMenuNode->Grouping)
	{
		// groups are ordered in descending order
		return (LhsMenuNode->Grouping >= RhsMenuNode->Grouping);
	}
	// next, make sure separators are preserved
	else if (bLhsIsSeparator != bRhsIsSeparator)
	{
		// separators with the same grouping go to the bottom of that "group"
		return bRhsIsSeparator;
	}
	// next, categories get listed before action nodes
	else if (bLhsIsCategory != bRhsIsCategory)
	{
		return bLhsIsCategory;
	}
	else
	{
		// both lhs and rhs are seemingly the same, so to keep them menu from 
		// jumping around everytime an entry is added, we sort by the order they
		// were inserted
		return (LhsMenuNode->InsertOrder < RhsMenuNode->InsertOrder);
	}
}

//------------------------------------------------------------------------------
bool FGraphActionNodeImpl::AlphabeticalNodeCompare(TSharedPtr<FGraphActionNode> const& LhsMenuNodePtr, TSharedPtr<FGraphActionNode> const& RhsMenuNodePtr)
{
	FGraphActionNode* LhsMenuNode = LhsMenuNodePtr.Get();
	FGraphActionNode* RhsMenuNode = RhsMenuNodePtr.Get();

	bool const bLhsIsCategory  = LhsMenuNode->IsCategoryNode();
	bool const bRhsIsCategory  = RhsMenuNode->IsCategoryNode();
	bool const bLhsIsSeparator = LhsMenuNode->IsSeparator();
	bool const bRhsIsSeparator = RhsMenuNode->IsSeparator();
	bool const bLhsIsSectionHeader = LhsMenuNode->IsSectionHeadingNode();
	bool const bRhsIsSectionHeader = RhsMenuNode->IsSectionHeadingNode();

	if (LhsMenuNode->SectionID != RhsMenuNode->SectionID)
	{
		// since we don't add section headers for children that have the same
		// section as their parents (the header is above the parent), we need to
		// organize them first (so they're seemingly under the same header)
		if ((LhsMenuNode->SectionID == LhsMenuNode->ParentNode.Pin()->SectionID) &&
			(LhsMenuNode->SectionID != FGraphActionNode::INVALID_SECTION_ID))
		{
			return true;
		}
		else // otherwise...
		{
			// sections are ordered in ascending order
			return (LhsMenuNode->SectionID < RhsMenuNode->SectionID);
		}
	}
	else if (bLhsIsSectionHeader != bRhsIsSectionHeader)
	{
		// section headers go to the top of that section
		return bLhsIsSectionHeader;
	}
	else if (LhsMenuNode->Grouping != RhsMenuNode->Grouping)
	{
		// groups are ordered in descending order
		return (LhsMenuNode->Grouping >= RhsMenuNode->Grouping);
	}
	// next, make sure separators are preserved
	else if (bLhsIsSeparator != bRhsIsSeparator)
	{
		// separators with the same grouping go to the bottom of that "group"
		return bRhsIsSeparator;
	}
	// next, categories get listed before action nodes
	else if (bLhsIsCategory != bRhsIsCategory)
	{
		return bLhsIsCategory;
	}
	else if (bLhsIsCategory) // if both nodes are category nodes
	{
		// @TODO: Should we be doing localized compares for categories? Probably.
		return (LhsMenuNode->GetDisplayName().ToString() <= RhsMenuNode->GetDisplayName().ToString());
	}
	else // both nodes are action nodes
	{
		return (LhsMenuNode->GetDisplayName().CompareTo(RhsMenuNode->GetDisplayName()) <= 0);
	}
}

/*******************************************************************************
 * FGraphActionNode
 ******************************************************************************/

//------------------------------------------------------------------------------
TSharedPtr<FGraphActionNode> FGraphActionNode::NewRootNode()
{
	// same as a group-divider node, just with an invalid parent
	return MakeShareable(new FGraphActionNode(FGraphActionNodeImpl::DEFAULT_GROUPING, INVALID_SECTION_ID));
}

//------------------------------------------------------------------------------
FGraphActionNode::FGraphActionNode(int32 Grouping, int32 SectionID)
	: SectionID(SectionID)
	, Grouping(Grouping)
	, bPendingRenameRequest(false)
	, InsertOrder(0)
{
}

//------------------------------------------------------------------------------
FGraphActionNode::FGraphActionNode(TArray< TSharedPtr<FEdGraphSchemaAction> > const& ActionList, int32 Grouping, int32 SectionID)
	: SectionID(SectionID)
	, Grouping(Grouping)
	, Actions(ActionList)
	, bPendingRenameRequest(false)
	, InsertOrder(0)
{
}

//------------------------------------------------------------------------------
TSharedPtr<FGraphActionNode> FGraphActionNode::AddChild(FGraphActionListBuilderBase::ActionGroup const& ActionSet)
{
	TArray<FString> CategoryStack;
	ActionSet.GetCategoryChain(CategoryStack);

	TSharedPtr<FGraphActionNode> ActionNode = FGraphActionNode::NewActionNode(ActionSet.Actions);
	AddChildRecursively(CategoryStack, ActionNode);

	return ActionNode;
}

//------------------------------------------------------------------------------
void FGraphActionNode::SortChildren(bool bAlphabetically/* = true*/, bool bRecursive/* = true*/)
{
	if (bRecursive)
	{
		for (TSharedPtr<FGraphActionNode>& ChildNode : Children)
		{
			ChildNode->SortChildren(bAlphabetically, bRecursive);
		}
	}

	if (bAlphabetically)
	{
		Children.Sort(FGraphActionNodeImpl::AlphabeticalNodeCompare);
	}
	else
	{
		Children.Sort(FGraphActionNodeImpl::NodeCompare);
	}
}

//------------------------------------------------------------------------------
void FGraphActionNode::GetAllNodes(TArray< TSharedPtr<FGraphActionNode> >& OutNodeArray) const
{
	for (TSharedPtr<FGraphActionNode> const& ChildNode : Children)
	{
		OutNodeArray.Add(ChildNode);
		ChildNode->GetAllNodes(OutNodeArray);
	}
}

//------------------------------------------------------------------------------
void FGraphActionNode::GetLeafNodes(TArray< TSharedPtr<FGraphActionNode> >& OutLeafArray) const
{
	for (TSharedPtr<FGraphActionNode> const& ChildNode : Children)
	{
		if (ChildNode->IsCategoryNode())
		{
			ChildNode->GetLeafNodes(OutLeafArray);
		}
		else
		{
			// @TODO: sometimes, certain action nodes can have children as well
			//        (for sub-graphs in the "MyBlueprint" tab)
			OutLeafArray.Add(ChildNode);
		}
	}
}

//------------------------------------------------------------------------------
void FGraphActionNode::ExpandAllChildren(TSharedPtr< STreeView< TSharedPtr<FGraphActionNode> > > TreeView, bool bRecursive/*= true*/)
{
	if (Children.Num() > 0)
	{
		TreeView->SetItemExpansion(this->AsShared(), /*ShouldExpandItem =*/true);
		for (TSharedPtr<FGraphActionNode>& ChildNode : Children)
		{
			if (bRecursive)
			{
				ChildNode->ExpandAllChildren(TreeView);
			}
			else
			{
				TreeView->SetItemExpansion(ChildNode, /*ShouldExpandItem =*/true);
			}
		}
	}
}

//------------------------------------------------------------------------------
void FGraphActionNode::ClearChildren()
{
	Children.Empty();
	ChildGroupings.Empty();
	ChildSections.Empty();
}

//------------------------------------------------------------------------------
bool FGraphActionNode::IsRootNode() const
{
	return (!IsActionNode() && !IsCategoryNode() && !ParentNode.IsValid());
}

//------------------------------------------------------------------------------
bool FGraphActionNode::IsSectionHeadingNode() const
{
	return (!IsActionNode() && !IsCategoryNode() && !IsRootNode() && (SectionID != INVALID_SECTION_ID));
}

//------------------------------------------------------------------------------
bool FGraphActionNode::IsCategoryNode() const
{
	return (!IsActionNode() && !DisplayText.IsEmpty());
}

//------------------------------------------------------------------------------
bool FGraphActionNode::IsActionNode() const
{
	return HasValidAction();
}

//------------------------------------------------------------------------------
bool FGraphActionNode::IsGroupDividerNode() const
{
	return (!IsActionNode() && !IsCategoryNode() && !IsRootNode() && (SectionID == INVALID_SECTION_ID));
}

//------------------------------------------------------------------------------
bool FGraphActionNode::IsSeparator() const
{
	return IsSectionHeadingNode() || IsGroupDividerNode();
}

//------------------------------------------------------------------------------
FText const& FGraphActionNode::GetDisplayName() const
{
	return DisplayText;
}

//------------------------------------------------------------------------------
FString FGraphActionNode::GetCategoryPath() const
{
	FString CategoryPath;
	if (IsCategoryNode())
	{
		CategoryPath = DisplayText.ToString();
	}

	TWeakPtr<FGraphActionNode> AncestorNode = ParentNode;
	while (AncestorNode.IsValid())
	{
		CategoryPath = AncestorNode.Pin()->DisplayText.ToString() + TEXT("|") + CategoryPath;
	}
	return CategoryPath;
}

//------------------------------------------------------------------------------
bool FGraphActionNode::HasValidAction() const
{
	return GetPrimaryAction().IsValid();
}

//------------------------------------------------------------------------------
TSharedPtr<FEdGraphSchemaAction> FGraphActionNode::GetPrimaryAction() const
{
	TSharedPtr<FEdGraphSchemaAction> PrimaryAction;
	for (TSharedPtr<FEdGraphSchemaAction> NodeAction : Actions)
	{
		if (NodeAction.IsValid())
		{
			PrimaryAction = NodeAction;
		}
	}
	return PrimaryAction;
}

//------------------------------------------------------------------------------
bool FGraphActionNode::BroadcastRenameRequest()
{
	if (RenameRequestEvent.IsBound())
	{
		RenameRequestEvent.Execute();
		bPendingRenameRequest = false;
	}
	else
	{
		bPendingRenameRequest = true;
	}
	return bPendingRenameRequest;
}

//------------------------------------------------------------------------------
bool FGraphActionNode::IsRenameRequestPending() const
{
	return bPendingRenameRequest;
}

//------------------------------------------------------------------------------
TSharedPtr<FGraphActionNode> FGraphActionNode::NewSectionHeadingNode(TWeakPtr<FGraphActionNode> Parent, int32 Grouping, int32 SectionID)
{
	checkSlow(SectionID != INVALID_SECTION_ID);

	FGraphActionNode* SectionNode = new FGraphActionNode(Grouping, SectionID);
	SectionNode->ParentNode = Parent;
	checkSlow(Parent.IsValid());

	return MakeShareable(SectionNode);
}

//------------------------------------------------------------------------------
TSharedPtr<FGraphActionNode> FGraphActionNode::NewCategoryNode(FString const& Category, int32 Grouping, int32 SectionID)
{
	FGraphActionNode* CategoryNode = new FGraphActionNode(Grouping, SectionID);
	CategoryNode->DisplayText = FText::FromString(Category);

	return MakeShareable(CategoryNode);
}

//------------------------------------------------------------------------------
TSharedPtr<FGraphActionNode> FGraphActionNode::NewActionNode(TArray< TSharedPtr<FEdGraphSchemaAction> > const& ActionList)
{
	int32 Grouping  = FGraphActionNodeImpl::DEFAULT_GROUPING;
	int32 SectionID = INVALID_SECTION_ID;

	for (TSharedPtr<FEdGraphSchemaAction> const& Action : ActionList)
	{
		Grouping = FMath::Max(Grouping, Action->Grouping);
		if (SectionID == INVALID_SECTION_ID)
		{
			// take the first non-zero section ID
			SectionID = Action->SectionID;
		}
	}

	FGraphActionNode* ActionNode = new FGraphActionNode(ActionList, Grouping, SectionID);
	TSharedPtr<FEdGraphSchemaAction> PrimeAction = ActionNode->GetPrimaryAction();
	checkSlow(PrimeAction.IsValid());
	ActionNode->DisplayText = PrimeAction->MenuDescription;

	return MakeShareable(ActionNode);
}

//------------------------------------------------------------------------------
TSharedPtr<FGraphActionNode> FGraphActionNode::NewGroupDividerNode(TWeakPtr<FGraphActionNode> Parent, int32 Grouping)
{
	FGraphActionNode* DividerNode = new FGraphActionNode(Grouping, INVALID_SECTION_ID);
	DividerNode->ParentNode = Parent;
	checkSlow(Parent.IsValid());

	return MakeShareable(DividerNode);
}

//------------------------------------------------------------------------------
void FGraphActionNode::AddChildRecursively(TArray<FString>& CategoryStack, TSharedPtr<FGraphActionNode> NodeToAdd)
{
	if (CategoryStack.Num() > 0)
	{
		FString CategorySection = CategoryStack[0];
		CategoryStack.RemoveAt(0, 1);

		// make sure we don't already have a child that this can nest under
		TSharedPtr<FGraphActionNode> ExistingNode = FindMatchingParent(CategorySection, NodeToAdd);
		if (ExistingNode.IsValid())
		{
			ExistingNode->AddChildRecursively(CategoryStack, NodeToAdd);
		}
		else
		{
			TSharedPtr<FGraphActionNode> CategoryNode = NewCategoryNode(CategorySection, NodeToAdd->Grouping, NodeToAdd->SectionID);
			InsertChild(CategoryNode);
			CategoryNode->AddChildRecursively(CategoryStack, NodeToAdd);
		}
	}
	else
	{
		InsertChild(NodeToAdd);
	}
}

//------------------------------------------------------------------------------
TSharedPtr<FGraphActionNode> FGraphActionNode::FindMatchingParent(FString const& ParentName, TSharedPtr<FGraphActionNode> NodeToAdd)
{
	TSharedPtr<FGraphActionNode> FoundCategoryNode;

	// for the "MyBlueprint" tab, sub-graph actions can be nested under graph
	// actions (meaning that action node can have children).
	bool const bCanNestUnderActionNodes = NodeToAdd->IsActionNode() && NodeToAdd->GetPrimaryAction()->IsParentable();

	for (TSharedPtr<FGraphActionNode> const& ChildNode : Children)
	{
		if (ChildNode->IsCategoryNode())
		{
			// @TODO: should we be matching grouping as well?
			if ((NodeToAdd->SectionID == ChildNode->SectionID) &&
				(ParentName == ChildNode->DisplayText.ToString()))
			{
				FoundCategoryNode = ChildNode;
				break;
			}
		}
		else if (bCanNestUnderActionNodes && ChildNode->IsActionNode())
		{
			// make the action's name into a display name, all categories are 
			// set as such (to ensure that the action name best matches the 
			// category ParentName)
			FString ChildNodeName = FName::NameToDisplayString(ChildNode->DisplayText.ToString(), /*bIsBool =*/false);

			// @TODO: should we be matching section/grouping as well?
			if (ChildNodeName == ParentName)
			{
				FoundCategoryNode = ChildNode;
				break;
			}
		}
	}

	return FoundCategoryNode;
}

//------------------------------------------------------------------------------
void FGraphActionNode::InsertChild(TSharedPtr<FGraphActionNode> NodeToAdd)
{
	ensure(!NodeToAdd->IsRootNode());
	ensure(!IsSeparator());

	NodeToAdd->ParentNode = this->AsShared();

	if (NodeToAdd->SectionID != INVALID_SECTION_ID)
	{
		// don't need a section heading if the parent is under the same section
		bool const bAddSectionHeading = (NodeToAdd->SectionID != SectionID) && 
			// make sure we already haven't already added a heading for this section
			!ChildSections.Contains(NodeToAdd->SectionID) &&
			// if this node also has a category, use that over a section heading
			(!NodeToAdd->IsActionNode() || NodeToAdd->GetPrimaryAction()->Category.IsEmpty());

		if (bAddSectionHeading)
		{
			ChildSections.Add(NodeToAdd->SectionID); // to avoid recursion, add before we insert
			InsertChild(NewSectionHeadingNode(NodeToAdd->ParentNode, NodeToAdd->Grouping, NodeToAdd->SectionID));
		}
	}
	// we don't use group-dividers inside of sections (we use groups to more to
	// hardcode the order), but if this isn't in a section...
	else if (!ChildGroupings.Contains(NodeToAdd->Grouping))
	{
		// don't need a divider if this is the first group
		if (ChildGroupings.Num() > 0)
		{
			int32 LowestGrouping = MAX_int32;
			for (int32 Group : ChildGroupings)
			{
				LowestGrouping = FMath::Min(LowestGrouping, Group);
			}
			// dividers come at the end of a menu group, so it would be 
			// undesirable to add it for NodeToAdd->Grouping if that group is 
			// lower than all the others (the lowest group should not have a 
			// divider associated with it)
			int32 DividerGrouping = FMath::Max(LowestGrouping, NodeToAdd->Grouping);

			ChildGroupings.Add(NodeToAdd->Grouping); // to avoid recursion, add before we insert
			InsertChild(NewGroupDividerNode(NodeToAdd->ParentNode, DividerGrouping));
		}
		else
		{
			ChildGroupings.Add(NodeToAdd->Grouping);
		}
	}

	NodeToAdd->InsertOrder = Children.Num();
	Children.Add(NodeToAdd);
}





