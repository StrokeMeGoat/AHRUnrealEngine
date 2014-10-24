// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Events.generated.h"


class SWindow;


/**
 * Context for keyboard focus change
 */
UENUM()
namespace EKeyboardFocusCause
{
	enum Type
	{
		/** Keyboard focus was changed because of a mouse action */
		Mouse,

		/** Keyboard focus was changed in response to a keystroke, such as the TAB key. */
		Keyboard,

		/** Keyboard focus was changed because someone asked the application to change it. */
		SetDirectly,

		/** Keyboard focus was explicitly cleared via the escape key or other similar action */
		Cleared,

		/** Keyboard focus was changed because another widget lost focus, and focus moved to a new widget */
		OtherWidgetLostFocus,

		/** Keyboard focus was set in response to the owning window being activated */
		WindowActivate,
	};
}


/**
 * FKeyboardFocusEvent is used when notifying widgets about keyboard focus changes
 * It is passed to event handlers dealing with keyboard focus
 */
USTRUCT()
struct FKeyboardFocusEvent
{
	GENERATED_USTRUCT_BODY()

public:

	/**
	 * UStruct Constructor.  Not meant for normal usage.
	 */
	FKeyboardFocusEvent()
		: Cause(EKeyboardFocusCause::SetDirectly)
	{ }

	/**
	 * Constructor.  Events are immutable once constructed.
	 *
	 * @param  InCause  The cause of the focus event
	 */
	FKeyboardFocusEvent( const EKeyboardFocusCause::Type InCause )
		: Cause(InCause)
	{ }


	/**
	 * Queries the reason for the focus change
	 *
	 * @return  The cause of the keyboard focus change
	 */
	EKeyboardFocusCause::Type GetCause() const
	{
		return Cause;
	}

private:

	/** The cause of the focus change */
	EKeyboardFocusCause::Type Cause;
};

/**
 * Represents the current and last cursor position in a "virtual window" for events that are routed to widgets transformed in a 3D scene.
 */
struct FVirtualPointerPosition
{
	FVirtualPointerPosition()
	: CurrentCursorPosition(FVector2D::ZeroVector)
	, LastCursorPosition(FVector2D::ZeroVector)
	{}
	FVector2D CurrentCursorPosition;
	FVector2D LastCursorPosition;
};

/**
 * Base class for all mouse and keyboard events.
 */
USTRUCT(BlueprintType)
struct FInputEvent
{
	GENERATED_USTRUCT_BODY()

public:

	/**
	 * UStruct Constructor.  Not meant for normal usage.
	 */
	FInputEvent()
		: ModifierKeys(FModifierKeysState())
		, bIsRepeat(false)
		, EventPath(nullptr)
	{ }

	/**
	 * Constructor.  Events are immutable once constructed.
	 *
	 * @param  InModifierKeys  Modifier key state for this event
	 * @param  bInIsRepeat  True if this key is an auto-repeated keystroke
	 */
	FInputEvent( const FModifierKeysState& InModifierKeys, const bool bInIsRepeat )
		: ModifierKeys(InModifierKeys)
		, bIsRepeat(bInIsRepeat)
		, EventPath(nullptr)
	{ }

	/**
	 * Virtual destructor.
	 */
	virtual ~FInputEvent( ) { }

public:

	/**
	 * Returns whether or not this character is an auto-repeated keystroke
	 *
	 * @return  True if this character is a repeat
	 */
	bool IsRepeat() const
	{
		return bIsRepeat;
	}

	/**
	 * Returns true if either shift key was down when this event occurred
	 *
	 * @return  True if shift is pressed
	 */
	bool IsShiftDown() const
	{
		return ModifierKeys.IsShiftDown();
	}

	/**
	 * Returns true if left shift key was down when this event occurred
	 *
	 * @return True if left shift is pressed.
	 */
	bool IsLeftShiftDown() const
	{
		return ModifierKeys.IsLeftShiftDown();
	}

	/**
	 * Returns true if right shift key was down when this event occurred
	 *
	 * @return True if right shift is pressed.
	 */
	bool IsRightShiftDown() const
	{
		return ModifierKeys.IsRightShiftDown();
	}

	/**
	 * Returns true if either control key was down when this event occurred
	 *
	 * @return  True if control is pressed
	 */
	bool IsControlDown() const
	{
		return ModifierKeys.IsControlDown();
	}

	/**
	 * Returns true if left control key was down when this event occurred
	 *
	 * @return  True if left control is pressed
	 */
	bool IsLeftControlDown() const
	{
		return ModifierKeys.IsLeftControlDown();
	}

	/**
	 * Returns true if right control key was down when this event occurred
	 *
	 * @return  True if right control is pressed
	 */
	bool IsRightControlDown() const
	{
		return ModifierKeys.IsRightControlDown();
	}

	/**
	 * Returns true if either alt key was down when this event occurred
	 *
	 * @return  True if alt is pressed
	 */
	bool IsAltDown() const
	{
		return ModifierKeys.IsAltDown();
	}

	/**
	 * Returns true if left alt key was down when this event occurred
	 *
	 * @return  True if left alt is pressed
	 */
	bool IsLeftAltDown() const
	{
		return ModifierKeys.IsLeftAltDown();
	}

	/**
	 * Returns true if right alt key was down when this event occurred
	 *
	 * @return  True if right alt is pressed
	 */
	bool IsRightAltDown() const
	{
		return ModifierKeys.IsRightAltDown();
	}

	/**
	 * Returns true if either command key was down when this event occurred
	 *
	 * @return  True if command is pressed
	 */
	bool IsCommandDown() const
	{
		return ModifierKeys.IsCommandDown();
	}

	/**
	 * Returns true if left command key was down when this event occurred
	 *
	 * @return  True if left command is pressed
	 */
	bool IsLeftCommandDown() const
	{
		return ModifierKeys.IsLeftCommandDown();
	}

	/**
	 * Returns true if right command key was down when this event occurred
	 *
	 * @return  True if right command is pressed
	 */
	bool IsRightCommandDown() const
	{
		return ModifierKeys.IsRightCommandDown();
	}

	/**
	 * Returns true if caps lock was on when this event occurred
	 * 
	 * @return True if caps lock is on
	 */
	bool AreCapsLocked() const
	{
		return ModifierKeys.AreCapsLocked();
	}

	/** The event path provides additional context for handling */
	FGeometry FindGeometry( const TSharedRef<SWidget>& WidgetToFind ) const
	{
		return EventPath->FindArrangedWidget(WidgetToFind).Get(FArrangedWidget::NullWidget).Geometry;
	}

	TSharedRef<SWindow> GetWindow() const
	{
		return EventPath->GetWindow();
	}

	/** Set the widget path along which this event will be routed */
	void SetEventPath( const FWidgetPath& InEventPath )
	{
		EventPath = &InEventPath;
	}

	SLATECORE_API virtual FText ToText() const;

protected:

	// State of modifier keys when this event happened.
	FModifierKeysState ModifierKeys;

	// True if this key was auto-repeated.
	bool bIsRepeat;

	// Events are sent along paths. See GetEventPath().
	const FWidgetPath* EventPath;
};


/**
 * FKeyboardEvent describes a keyboard action (key pressed or released.)
 * It is passed to event handlers dealing with keyboard input.
 */
USTRUCT(BlueprintType)
struct FKeyboardEvent
	: public FInputEvent
{
	GENERATED_USTRUCT_BODY()

public:
	/**
	 * UStruct Constructor.  Not meant for normal usage.
	 */
	FKeyboardEvent()
		: FInputEvent(FModifierKeysState(), false)
		, Key(EKeys::SpaceBar)
		, CharacterCode(0)
		, KeyCode(0)
	{
	}

	/**
	 * Constructor.  Events are immutable once constructed.
	 *
	 * @param  InKeyName  Character name
	 * @param  InModifierKeys  Modifier key state for this event
	 * @param  bInIsRepeat  True if this key is an auto-repeated keystroke
	 */
	FKeyboardEvent( const FKey InKey,
					const FModifierKeysState& InModifierKeys, 
					const bool bInIsRepeat,
					const uint32 InCharacterCode,
					const uint32 InKeyCode
	)
		: FInputEvent(InModifierKeys, bInIsRepeat)
		, Key(InKey)
		, CharacterCode(InCharacterCode)
		, KeyCode(InKeyCode)
	{ }
	
	/**
	 * Returns the name of the key for this event
	 *
	 * @return  Key name
	 */
	FKey GetKey() const
	{
		return Key;
	}

	/**
	 * Returns the character code for this event
	 *
	 * @return  Character code or 0 if this event was not a character key press
	 */
	uint32 GetCharacter() const
	{
		return CharacterCode;
	}

	uint32 GetKeyCode() const
	{
		return KeyCode;
	}

	SLATECORE_API virtual FText ToText() const override;

private:

	// Name of the key that was pressed.
	FKey Key;

	// The character code of the key that was pressed.  Only applicable to typed character keys, 0 otherwise.
	uint32 CharacterCode;

	// Original key code received from hardware before any conversion/mapping
	uint32 KeyCode;
};


/**
 * FCharacterEvent describes a keyboard action where the utf-16 code is given.  Used for OnKeyChar messages
 */
USTRUCT()
struct FCharacterEvent
	: public FInputEvent
{
	GENERATED_USTRUCT_BODY()

public:
	/**
	 * UStruct Constructor.  Not meant for normal usage.
	 */
	FCharacterEvent()
		: FInputEvent(FModifierKeysState(), false)
		, Character(0)
	{
	}

	FCharacterEvent( const TCHAR InCharacter, const FModifierKeysState& InModifierKeys, const bool bInIsRepeat )
		: FInputEvent(InModifierKeys, bInIsRepeat)
		, Character(InCharacter)
	{ }

	/**
	 * Returns the character for this event
	 *
	 * @return  Character
	 */
	TCHAR GetCharacter() const
	{
		return Character;
	}

	SLATECORE_API virtual FText ToText() const override;	

private:

	// The character that was pressed.
	TCHAR Character;
};


/**
 * Helper class to auto-populate a set with the set of "keys" a touch represents
 */
class FTouchKeySet
	: public TSet<FKey>
{
public:

	/**
	 * Creates and initializes a new instance with the specified key.
	 *
	 * @param Key The key to insert into the set.
	 */
	FTouchKeySet(FKey Key)
	{
		this->Add(Key);
	}

public:

	// The standard set consists of just the left mouse button key.
	SLATECORE_API static const FTouchKeySet StandardSet;

	// The empty set contains no valid keys.
	SLATECORE_API static const FTouchKeySet EmptySet;
};


/**
 * FPointerEvent describes a mouse or touch action (e.g. Press, Release, Move, etc).
 * It is passed to event handlers dealing with pointer-based input.
 */
USTRUCT(BlueprintType)
struct FPointerEvent
	: public FInputEvent
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	 * UStruct Constructor.  Not meant for normal usage.
	 */
	FPointerEvent()
		: ScreenSpacePosition(FVector2D(0, 0))
		, LastScreenSpacePosition(FVector2D(0, 0))
		, CursorDelta(FVector2D(0, 0))
		, PressedButtons(FTouchKeySet::EmptySet)
		, EffectingButton(EKeys::LeftMouseButton)
		, UserIndex(0)
		, PointerIndex(0)
		, TouchpadIndex(0)
		, bIsTouchEvent(false)
		, GestureType(EGestureEvent::None)
		, WheelOrGestureDelta(0.0f, 0)
	{ }

	/** Events are immutable once constructed. */
	FPointerEvent(
		uint32 InPointerIndex,
		const FVector2D& InScreenSpacePosition,
		const FVector2D& InLastScreenSpacePosition,
		const TSet<FKey>& InPressedButtons,
		FKey InEffectingButton,
		float InWheelDelta,
		const FModifierKeysState& InModifierKeys
	)
		: FInputEvent(InModifierKeys, false)
		, ScreenSpacePosition(InScreenSpacePosition)
		, LastScreenSpacePosition(InLastScreenSpacePosition)
		, CursorDelta(InScreenSpacePosition - InLastScreenSpacePosition)
		, PressedButtons(InPressedButtons)
		, EffectingButton(InEffectingButton)
		, UserIndex(0)
		, PointerIndex(InPointerIndex)
		, TouchpadIndex(0)
		, bIsTouchEvent(false)
		, GestureType(EGestureEvent::None)
		, WheelOrGestureDelta(0.0f, InWheelDelta)
	{ }

	/** A constructor for raw mouse events */
	FPointerEvent(
		uint32 InPointerIndex,
		const FVector2D& InScreenSpacePosition,
		const FVector2D& InLastScreenSpacePosition,
		const FVector2D& InDelta,
		const TSet<FKey>& InPressedButtons,
		const FModifierKeysState& InModifierKeys
	)
		: FInputEvent(InModifierKeys, false)
		, ScreenSpacePosition(InScreenSpacePosition)
		, LastScreenSpacePosition(InLastScreenSpacePosition)
		, CursorDelta(InDelta)
		, PressedButtons(InPressedButtons)
		, UserIndex(0)
		, PointerIndex(InPointerIndex)
		, TouchpadIndex(0)
		, bIsTouchEvent(false)
		, GestureType(EGestureEvent::None)
		, WheelOrGestureDelta(0.0f, 0.0f)
	{ }

	/** A constructor for touch events */
	FPointerEvent(
		uint32 InUserIndex,
		uint32 InPointerIndex,
		const FVector2D& InScreenSpacePosition,
		const FVector2D& InLastScreenSpacePosition,
		bool bPressLeftMouseButton,
		const FModifierKeysState& InModifierKeys = FModifierKeysState(),
		uint32 InTouchpadIndex=0
	)
		: FInputEvent(InModifierKeys, false)
		, ScreenSpacePosition(InScreenSpacePosition)
		, LastScreenSpacePosition(InLastScreenSpacePosition)
		, CursorDelta(InScreenSpacePosition - InLastScreenSpacePosition)
		, PressedButtons(bPressLeftMouseButton ? FTouchKeySet::StandardSet : FTouchKeySet::EmptySet)
		, EffectingButton(EKeys::LeftMouseButton)
		, UserIndex(InUserIndex)
		, PointerIndex(InPointerIndex)
		, TouchpadIndex(InTouchpadIndex)
		, bIsTouchEvent(true)
		, GestureType(EGestureEvent::None)
		, WheelOrGestureDelta(0.0f, 0.0f)
	{ }

	/** A constructor for gesture events */
	FPointerEvent(
		const FVector2D& InScreenSpacePosition,
		const FVector2D& InLastScreenSpacePosition,
		const TSet<FKey>& InPressedButtons,
		const FModifierKeysState& InModifierKeys,
		EGestureEvent::Type InGestureType,
		const FVector2D& InGestureDelta
	)
		: FInputEvent(InModifierKeys, false)
		, ScreenSpacePosition(InScreenSpacePosition)
		, LastScreenSpacePosition(InLastScreenSpacePosition)
		, CursorDelta(LastScreenSpacePosition - ScreenSpacePosition)
		, PressedButtons(InPressedButtons)
		, UserIndex(0)
		, PointerIndex(0)
		, bIsTouchEvent(false)
		, GestureType(InGestureType)
		, WheelOrGestureDelta(InGestureDelta)
	{ }
	
public:

	/** @return The position of the cursor in screen space */
	const FVector2D& GetScreenSpacePosition() const { return ScreenSpacePosition; }

	/** @return The position of the cursor in screen space last time we handled an input event */
	const FVector2D& GetLastScreenSpacePosition() const { return LastScreenSpacePosition; }

	/** @return the distance the mouse traveled since the last event was handled. */
	FVector2D GetCursorDelta() const { return CursorDelta; }

	/** Mouse buttons that are currently pressed */
	bool IsMouseButtonDown( FKey MouseButton ) const { return PressedButtons.Contains( MouseButton ); }

	/** Mouse button that caused this event to be raised (possibly EB_None) */
	FKey GetEffectingButton() const { return EffectingButton; }
	
	/** How much did the mouse wheel turn since the last mouse event */
	float GetWheelDelta() const { return WheelOrGestureDelta.Y; }

	/** @return The index of the user that caused the event */
	int32 GetUserIndex() const { return UserIndex; }

	/** @return The unique identifier of the pointer (e.g., finger index) */
	uint32 GetPointerIndex() const { return PointerIndex; }

	/** @return The index of the touch pad that generated this event (for platforms with multiple touch pads per user) */
	uint32 GetTouchpadIndex() const { return TouchpadIndex; }

	/** @return Is this event a result from a touch (as opposed to a mouse) */
	bool IsTouchEvent() const { return bIsTouchEvent; }

	/** @return The type of touch gesture */
	EGestureEvent::Type GetGestureType() const { return GestureType; }

	/** @return The change in gesture value since the last gesture event of the same type. */
	const FVector2D& GetGestureDelta() const { return WheelOrGestureDelta; }

	/** We override the assignment operator to allow generated code to compile with the const ref member. */
	void operator=( const FPointerEvent& Other )
	{
		ScreenSpacePosition = Other.ScreenSpacePosition;
		LastScreenSpacePosition = Other.LastScreenSpacePosition;
		CursorDelta = Other.CursorDelta;
		const_cast<TSet<FKey>&>(PressedButtons) = Other.PressedButtons;
		EffectingButton = Other.EffectingButton;
		UserIndex = Other.UserIndex;
		PointerIndex = Other.PointerIndex;
		TouchpadIndex = Other.TouchpadIndex;
		bIsTouchEvent = Other.bIsTouchEvent;
		GestureType = Other.GestureType;
		WheelOrGestureDelta = Other.WheelOrGestureDelta;
	}

	SLATECORE_API virtual FText ToText() const override;

	template<typename PointerEventType>
	static PointerEventType MakeTranslatedEvent( const PointerEventType& InPointerEvent, const FVirtualPointerPosition& VirtualPosition )
	{
		PointerEventType NewEvent = InPointerEvent;
		NewEvent.ScreenSpacePosition = VirtualPosition.CurrentCursorPosition;
		NewEvent.LastScreenSpacePosition = VirtualPosition.LastCursorPosition;
		//NewEvent.CursorDelta = VirtualPosition.GetDelta();
		return NewEvent;
	}

private:

	FVector2D ScreenSpacePosition;
	FVector2D LastScreenSpacePosition;
	FVector2D CursorDelta;
	const TSet<FKey>& PressedButtons;
	FKey EffectingButton;
	uint32 UserIndex;
	uint32 PointerIndex;
	uint32 TouchpadIndex;
	bool bIsTouchEvent;
	EGestureEvent::Type GestureType;
	FVector2D WheelOrGestureDelta;
};


/**
 * FControllerEvent describes a controller action (e.g. Button Press, Release, Analog stick move, etc).
 * It is passed to event handlers dealing with controller input.
 */
USTRUCT(BlueprintType)
struct FControllerEvent
	: public FInputEvent
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	 * UStruct Constructor.  Not meant for normal usage.
	 */
	FControllerEvent()
		: EffectingButton(EKeys::Gamepad_RightTrigger)
		, UserIndex(0)
		, AnalogValue(0)
	{ }

	FControllerEvent( FKey InEffectingButton, int32 InUserIndex, float InAnalogValue, bool bIsRepeat )
		: FInputEvent(FModifierKeysState(), bIsRepeat)
		, EffectingButton(InEffectingButton)
		, UserIndex(InUserIndex)
		, AnalogValue(InAnalogValue)
	{ }

public:

	/** @return The controller button that caused this event */
	FKey GetEffectingButton() const { return EffectingButton; }

	/** @return The index of the user that caused the event */
	uint32 GetUserIndex() const { return UserIndex; }

	/** @return Analog value between 0 and 1.  1 being fully pressed, 0 being not pressed at all */
	float GetAnalogValue() const { return AnalogValue; }

	SLATECORE_API virtual FText ToText() const override;

private:

	// The controller button that caused this event.
	FKey EffectingButton;

	// The index of the user that caused the event.
	uint32 UserIndex;
	
	//  Analog value between 0 and 1 (1 being fully pressed, 0 being not pressed at all).
	float AnalogValue;
};


/**
 * FMotionEvent describes a touch pad action (press, move, lift)
 * It is passed to event handlers dealing with touch input.
 */
USTRUCT(BlueprintType)
struct FMotionEvent
	: public FInputEvent
{
	GENERATED_USTRUCT_BODY()

public:
	/**
	* UStruct Constructor.  Not meant for normal usage.
	*/
	FMotionEvent()
		: UserIndex(0)
		, Tilt(FVector(0, 0, 0))
		, RotationRate(FVector(0, 0, 0))
		, Gravity(FVector(0, 0, 0))
		, Acceleration(FVector(0, 0, 0))
	{ }

	FMotionEvent(
		uint32 InUserIndex,
		const FVector& InTilt, 
		const FVector& InRotationRate, 
		const FVector& InGravity, 
		const FVector& InAcceleration
	)
		: FInputEvent(FModifierKeysState(), false)
		, UserIndex(InUserIndex)
		, Tilt(InTilt)
		, RotationRate(InRotationRate)
		, Gravity(InGravity)
		, Acceleration(InAcceleration)
	{ }

public:

	/** @return The index of the user that caused the event */
	uint32 GetUserIndex() const { return UserIndex; }

	/** @return Current tilt of the device/controller */
	const FVector& GetTilt() const { return Tilt; }

	/** @return Rotation speed */
	const FVector& GetRotationRate() const { return RotationRate; }

	/** @return Gravity vector (pointing down into the ground) */
	const FVector& GetGravity() const { return Gravity; }

	/** @return 3D acceleration of the device */
	const FVector& GetAcceleration() const { return Acceleration; }

private:

	// The index of the user that caused the event.
	uint32 UserIndex;

	// The current tilt of the device/controller.
	FVector Tilt;

	// The rotation speed.
	FVector RotationRate;

	// The gravity vector (pointing down into the ground).
	FVector Gravity;

	// The 3D acceleration of the device.
	FVector Acceleration;
};


/**
 * FWindowActivateEvent describes a window being activated or deactivated.
 * (i.e. brought to the foreground or moved to the background)
 * This event is only passed to top level windows; most widgets are incapable
 * of receiving this event.
 */
class FWindowActivateEvent
{
public:

	enum EActivationType
	{
		EA_Activate,
		EA_ActivateByMouse,
		EA_Deactivate
	};

	FWindowActivateEvent( EActivationType InActivationType, TSharedRef<SWindow> InAffectedWindow )
		: ActivationType(InActivationType)
		, AffectedWindow(InAffectedWindow)
	{ }

public:

	/** Describes what actually happened to the window (e.g. Activated, Deactivated, Activated by a mouse click) */
	EActivationType GetActivationType() const
	{
		return ActivationType;
	}

	/** The window that this activation/deactivation happened to */
	TSharedRef<SWindow> GetAffectedWindow() const
	{
		return AffectedWindow;
	}

private:

	EActivationType ActivationType;
	TSharedRef<SWindow> AffectedWindow;
};
