// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "BehaviorTreeEditorCommands.h"

#define LOCTEXT_NAMESPACE "BehaviorTreeEditorCommands"

FBTCommonCommands::FBTCommonCommands() 
	: TCommands<FBTCommonCommands>("BTEditor.Common", LOCTEXT("Common", "Common"), NAME_None, FEditorStyle::GetStyleSetName())
{
}

void FBTCommonCommands::RegisterCommands()
{
	UI_COMMAND(SearchBT, "Search", "Search this Behavior Tree.", EUserInterfaceActionType::Button, FInputGesture(EModifierKey::Control, EKeys::F));
	UI_COMMAND(NewBlackboard, "New Blackboard", "Create a new Blackboard Data Asset", EUserInterfaceActionType::Button, FInputGesture());
}

FBTDebuggerCommands::FBTDebuggerCommands() 
	: TCommands<FBTDebuggerCommands>("BTEditor.Debugger", LOCTEXT("Debugger", "Debugger"), NAME_None, FEditorStyle::GetStyleSetName())
{
}

void FBTDebuggerCommands::RegisterCommands()
{
	UI_COMMAND(BackInto, "Back: Into", "Show state from previous step, can go into subtrees", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(BackOver, "Back: Over", "Show state from previous step, don't go into subtrees", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ForwardInto, "Forward: Into", "Show state from next step, can go into subtrees", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ForwardOver, "Forward: Over", "Show state from next step, don't go into subtrees", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(StepOut, "Step Out", "Show state from next step, leave current subtree", EUserInterfaceActionType::Button, FInputGesture());
	
	UI_COMMAND(PausePlaySession, "Pause", "Pause simulation", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ResumePlaySession, "Resume", "Resume simulation", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND(StopPlaySession, "Stop", "Stop simulation", EUserInterfaceActionType::Button, FInputGesture());

	UI_COMMAND(CurrentValues, "Current", "View current values", EUserInterfaceActionType::RadioButton, FInputGesture());
	UI_COMMAND(SavedValues, "Saved", "View saved values", EUserInterfaceActionType::RadioButton, FInputGesture());
}

FBTBlackboardCommands::FBTBlackboardCommands() 
	: TCommands<FBTBlackboardCommands>("BTEditor.Blackboard", LOCTEXT("Blackboard", "Blackboard"), NAME_None, FEditorStyle::GetStyleSetName())
{
}

void FBTBlackboardCommands::RegisterCommands()
{
	UI_COMMAND(DeleteEntry, "Delete", "Delete this blackboard entry", EUserInterfaceActionType::Button, FInputGesture(EKeys::Platform_Delete));
}

#undef LOCTEXT_NAMESPACE
