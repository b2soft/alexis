#pragma once

#include "KeyCodes.h"

// Base class for all events
class EventArgs
{
public:
	EventArgs() {}
};

class KeyEventArgs : public EventArgs
{
public:
	enum KeyState
	{
		Released = 0,
		Pressed = 1
	};

	typedef EventArgs base;
	KeyEventArgs(KeyCode::Key key, unsigned int c, KeyState state, bool control, bool shift, bool alt)
		: Key(key)
		, Char(c)
		, State(state)
		, Control(control)
		, Shift(shift)
		, Alt(alt)
	{}

	KeyCode::Key	Key;		// The Key Code that was pressed/released
	unsigned int	Char;		// The 32-bit char code. Will be 0 for non-printable character
	KeyState		State;		// Was pressed or released
	bool			Control;	// Is Control pressed
	bool			Shift;		// Is Shift pressed
	bool			Alt;		// Is Alt pressed
};

class MouseMotionEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	MouseMotionEventArgs(bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y)
		: LeftButton(leftButton)
		, MiddleButton(middleButton)
		, RightButton(rightButton)
		, Control(control)
		, Shift(shift)
		, X(x)
		, Y(y)
		, RelX(0)
		, RelY(0)
	{}

	bool LeftButton;	// Is left btn pressed
	bool MiddleButton;	// Is Middle btn pressed
	bool RightButton;	// Is Right btn pressed
	bool Control;		// is Ctrl key pressed
	bool Shift;			// Is Shift key pressed

	int X;				// The X position of the cursor relative to the upper-left corner of the client area
	int Y;				// The Y position of the cursor relative to the upper-left corner of the client area
	int RelX;			// How far mouse moved across X since the last event
	int RelY;			// How far mouse moved across Y since the last event
};

class MouseButtonEventArgs : public EventArgs
{
public:
	enum MouseButton
	{
		None = 0,
		Left = 1,
		Right = 2,
		Middle = 3
	};

	enum ButtonState
	{
		Released = 0,
		Pressed = 1
	};

	typedef EventArgs base;
	MouseButtonEventArgs(MouseButton buttonID, ButtonState state, bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y)
		: Button(buttonID)
		, State(state)
		, LeftButton(leftButton)
		, MiddleButton(middleButton)
		, RightButton(rightButton)
		, Control(control)
		, Shift(shift)
		, X(x)
		, Y(y)
	{}

	MouseButton Button;	// Mouse button pressed/released
	ButtonState State;	// Pressed or released
	bool LeftButton;	// Is left btn pressed
	bool MiddleButton;	// Is Middle btn pressed
	bool RightButton;	// Is Right btn pressed
	bool Control;		// is Ctrl key pressed
	bool Shift;			// Is Shift key pressed

	int X;				// The X position of the cursor relative to the upper-left corner of the client area
	int Y;				// The Y position of the cursor relative to the upper-left corner of the client area
};

class MouseWheelEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	MouseWheelEventArgs(float wheelDelta, bool leftButton, bool middleButton, bool rightButton, bool control, bool shift, int x, int y)
		: WheelDelta(wheelDelta)
		, LeftButton(leftButton)
		, MiddleButton(middleButton)
		, RightButton(rightButton)
		, Control(control)
		, Shift(shift)
		, X(x)
		, Y(y)
	{}

	float WheelDelta;	// How much the mouse wheel has moved. A positive value indicates that the wheel was moved to the right. A negative value indicates the wheel was moved to the left
	bool LeftButton;	// Is left btn pressed
	bool MiddleButton;	// Is Middle btn pressed
	bool RightButton;	// Is Right btn pressed
	bool Control;		// is Ctrl key pressed
	bool Shift;			// Is Shift key pressed

	int X;				// The X position of the cursor relative to the upper-left corner of the client area
	int Y;				// The Y position of the cursor relative to the upper-left corner of the client area
};

class ResizeEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	ResizeEventArgs(int width, int height)
		: Width(width)
		, Height(height)
	{}

	int Width;
	int Height;
};

class UpdateEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	UpdateEventArgs(double deltaTime, double totalTime)
		: ElapsedTime(deltaTime)
		, TotalTime(totalTime)
	{}

	double ElapsedTime;
	double TotalTime;
};

class RenderEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	RenderEventArgs( double deltaTime, double totalTime)
		: ElapsedTime(deltaTime)
		, TotalTime(totalTime)
	{}

	double ElapsedTime;
	double TotalTime
};

class UserEventArgs : public EventArgs
{
public:
	typedef EventArgs base;
	UserEventArgs(int code, void* data1, void* data2)
		: Code(code)
		, Data1(data1)
		, Data2(data2)
	{}

	int Code;
	void* Data1;
	void* Data2;
};