// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "SViewportToolBar.h"

#define LOCTEXT_NAMESPACE "ViewportToolBar"

namespace ToolBarConstants
{
	/** The opacity when we are hovered */
	const float HoveredOpacity = 1.0f;
	/** The opacity when we are not hovered */
	const float NonHoveredOpacity = .75f;
	/** The amount of time to wait before fading out the toolbar after the mouse leaves it (to reduce poping when mouse moves in and out frequently */
	const float TimeToFadeOut = 1.0f;
	/** The amount of time spent actually fading in or out */
	const float FadeTime = .15f;
}

void SViewportToolBar::Construct( const FArguments& InArgs )
{
	bIsHovered = false;

	FadeInSequence = FCurveSequence( 0.0f, ToolBarConstants::FadeTime );
	FadeOutSequence = FCurveSequence( ToolBarConstants::TimeToFadeOut, ToolBarConstants::FadeTime );
	FadeOutSequence.Play();
}

TWeakPtr<SMenuAnchor> SViewportToolBar::GetOpenMenu() const
{
	return OpenedMenu;
}

void SViewportToolBar::SetOpenMenu( TSharedPtr< SMenuAnchor >& NewMenu )
{
	if( OpenedMenu.IsValid() && OpenedMenu.Pin() != NewMenu )
	{
		// Close any other open menus
		OpenedMenu.Pin()->SetIsOpen( false );
	}
	OpenedMenu = NewMenu;
}

FLinearColor SViewportToolBar::OnGetColorAndOpacity() const
{
	FLinearColor Color = FLinearColor::White;
	
	if( OpenedMenu.IsValid() && OpenedMenu.Pin()->IsOpen() )
	{
		// Never fade out the toolbar if a menu is open
		Color.A = ToolBarConstants::HoveredOpacity;
	}
	else if( FadeOutSequence.IsPlaying() || !bIsHovered )
	{
		Color.A = FMath::Lerp( ToolBarConstants::HoveredOpacity, ToolBarConstants::NonHoveredOpacity, FadeOutSequence.GetLerp() );
	}
	else
	{
		Color.A = FMath::Lerp( ToolBarConstants::NonHoveredOpacity, ToolBarConstants::HoveredOpacity, FadeInSequence.GetLerp() );
	}

	return Color;
}


void SViewportToolBar::OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// The viewport could potentially be moved around inside the toolbar when the mouse is captured
	// If that is the case we do not play the fade transition
	if( !FSlateApplication::Get().IsUsingHighPrecisionMouseMovment() )
	{
		bIsHovered = true;
		if( FadeOutSequence.IsPlaying() )
		{
			// Fade out is already playing so just force the fade in curve to the end so we dont have a "pop" 
			// effect from quickly resetting the alpha
			FadeInSequence.JumpToEnd();
		}
		else
		{
			FadeInSequence.Play();
		}
	}

}

void SViewportToolBar::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	// The viewport could potentially be moved around inside the toolbar when the mouse is captured
	// If that is the case we do not play the fade transition
	if( !FSlateApplication::Get().IsUsingHighPrecisionMouseMovment() )
	{
		bIsHovered = false;
		FadeOutSequence.Play();
	}
}

#undef LOCTEXT_NAMESPACE