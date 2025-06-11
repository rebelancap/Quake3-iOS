/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../client/client.h"
#include "../sys/sys_local.h"
#include "../qcommon/qcommon.h"

// iOS GameController support
#ifdef __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IOS || TARGET_OS_TV
#include "ios_gamecontroller_bridge.h"
#define USE_IOS_GAMECONTROLLER 1
#endif
#endif

#ifdef USE_IOS_GAMECONTROLLER
#include "ios_gamecontroller_bridge.h"
void Sys_HideControls(SDL_Window *sdlWindow);  // Forward declaration
#endif

static cvar_t *in_keyboardDebug     = NULL;

static SDL_GameController *gamepad = NULL;
static SDL_Joystick *stick = NULL;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive = qfalse;

static cvar_t *in_mouse             = NULL;
static cvar_t *in_nograb;

static cvar_t *in_joystick          = NULL;
static cvar_t *in_joystickThreshold = NULL;
static cvar_t *in_joystickNo        = NULL;
static cvar_t *in_joystickUseAnalog = NULL;

#ifdef USE_IOS_GAMECONTROLLER
// iOS GameController specific cvars
static cvar_t *joy_deadzone         = NULL;
static cvar_t *joy_sensitivity      = NULL;
static cvar_t *joy_hapticStrength   = NULL;

// Button mapping cvars
static cvar_t *joy_buttonA_bind     = NULL;
static cvar_t *joy_buttonB_bind     = NULL;
static cvar_t *joy_buttonX_bind     = NULL;
static cvar_t *joy_buttonY_bind     = NULL;
static cvar_t *joy_leftShoulder_bind = NULL;
static cvar_t *joy_rightShoulder_bind = NULL;
static cvar_t *joy_leftTrigger_bind = NULL;
static cvar_t *joy_rightTrigger_bind = NULL;
static cvar_t *joy_dpadUp_bind      = NULL;
static cvar_t *joy_dpadDown_bind    = NULL;
static cvar_t *joy_dpadLeft_bind    = NULL;
static cvar_t *joy_dpadRight_bind   = NULL;
static cvar_t *joy_buttonMenu_bind  = NULL;
static cvar_t *joy_buttonOptions_bind = NULL;
static cvar_t *joy_leftThumbstickButton_bind = NULL;
static cvar_t *joy_rightThumbstickButton_bind = NULL;

// Axis sensitivity cvars
static cvar_t *joy_leftStickSensitivity = NULL;
static cvar_t *joy_rightStickSensitivity = NULL;

// joystick mouse sensitivity
static cvar_t *joy_menuMouseSpeed = NULL;

// Previous button states for edge detection
static struct {
    int buttonA;
    int buttonB;
    int buttonX;
    int buttonY;
    int leftShoulder;
    int rightShoulder;
    int leftTrigger;
    int rightTrigger;
    int dpadUp;
    int dpadDown;
    int dpadLeft;
    int dpadRight;
    int buttonMenu;
    int buttonOptions;
    int leftThumbstickButton;
    int rightThumbstickButton;
} ios_controller_state;
#endif

static int vidRestartTime = 0;

static int in_eventTime = 0;


//static SDL_Window *SDL_window = NULL;
extern SDL_Window *SDL_window; //

#define CTRL(a) ((a)-'a'+1)

/*
===============
IN_PrintKey
===============
*/
static void IN_PrintKey( const SDL_Keysym *keysym, keyNum_t key, qboolean down )
{
	if( down )
		Com_Printf( "+ " );
	else
		Com_Printf( "  " );

	Com_Printf( "Scancode: 0x%02x(%s) Sym: 0x%02x(%s)",
			keysym->scancode, SDL_GetScancodeName( keysym->scancode ),
			keysym->sym, SDL_GetKeyName( keysym->sym ) );

	if( keysym->mod & KMOD_LSHIFT )   Com_Printf( " KMOD_LSHIFT" );
	if( keysym->mod & KMOD_RSHIFT )   Com_Printf( " KMOD_RSHIFT" );
	if( keysym->mod & KMOD_LCTRL )    Com_Printf( " KMOD_LCTRL" );
	if( keysym->mod & KMOD_RCTRL )    Com_Printf( " KMOD_RCTRL" );
	if( keysym->mod & KMOD_LALT )     Com_Printf( " KMOD_LALT" );
	if( keysym->mod & KMOD_RALT )     Com_Printf( " KMOD_RALT" );
	if( keysym->mod & KMOD_LGUI )     Com_Printf( " KMOD_LGUI" );
	if( keysym->mod & KMOD_RGUI )     Com_Printf( " KMOD_RGUI" );
	if( keysym->mod & KMOD_NUM )      Com_Printf( " KMOD_NUM" );
	if( keysym->mod & KMOD_CAPS )     Com_Printf( " KMOD_CAPS" );
	if( keysym->mod & KMOD_MODE )     Com_Printf( " KMOD_MODE" );
	if( keysym->mod & KMOD_RESERVED ) Com_Printf( " KMOD_RESERVED" );

	Com_Printf( " Q:0x%02x(%s)\n", key, Key_KeynumToString( key ) );
}

#define MAX_CONSOLE_KEYS 16

/*
===============
IN_IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of
      both of these methods
===============
*/
static qboolean IN_IsConsoleKey( keyNum_t key, int character )
{
	typedef struct consoleKey_s
	{
		enum
		{
			QUAKE_KEY,
			CHARACTER
		} type;

		union
		{
			keyNum_t key;
			int character;
		} u;
	} consoleKey_t;

	static consoleKey_t consoleKeys[ MAX_CONSOLE_KEYS ];
	static int numConsoleKeys = 0;
	int i;

	// Only parse the variable when it changes
	if( cl_consoleKeys->modified )
	{
		char *text_p, *token;

		cl_consoleKeys->modified = qfalse;
		text_p = cl_consoleKeys->string;
		numConsoleKeys = 0;

		while( numConsoleKeys < MAX_CONSOLE_KEYS )
		{
			consoleKey_t *c = &consoleKeys[ numConsoleKeys ];
			int charCode = 0;

			token = COM_Parse( &text_p );
			if( !token[ 0 ] )
				break;

			charCode = Com_HexStrToInt( token );

			if( charCode > 0 )
			{
				c->type = CHARACTER;
				c->u.character = charCode;
			}
			else
			{
				c->type = QUAKE_KEY;
				c->u.key = Key_StringToKeynum( token );

				// 0 isn't a key
				if( c->u.key <= 0 )
					continue;
			}

			numConsoleKeys++;
		}
	}

	// If the character is the same as the key, prefer the character
	if( key == character )
		key = 0;

	for( i = 0; i < numConsoleKeys; i++ )
	{
		consoleKey_t *c = &consoleKeys[ i ];

		switch( c->type )
		{
			case QUAKE_KEY:
				if( key && c->u.key == key )
					return qtrue;
				break;

			case CHARACTER:
				if( c->u.character == character )
					return qtrue;
				break;
		}
	}

	return qfalse;
}

/*
===============
IN_TranslateSDLToQ3Key
===============
*/
static keyNum_t IN_TranslateSDLToQ3Key( SDL_Keysym *keysym, qboolean down )
{
	keyNum_t key = 0;

	if( keysym->scancode >= SDL_SCANCODE_1 && keysym->scancode <= SDL_SCANCODE_0 )
	{
		// Always map the number keys as such even if they actually map
		// to other characters (eg, "1" is "&" on an AZERTY keyboard).
		// This is required for SDL before 2.0.6, except on Windows
		// which already had this behavior.
		if( keysym->scancode == SDL_SCANCODE_0 )
			key = '0';
		else
			key = '1' + keysym->scancode - SDL_SCANCODE_1;
	}
	else if( keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE )
	{
		// These happen to match the ASCII chars
		key = (int)keysym->sym;
	}
	else
	{
		switch( keysym->sym )
		{
			case SDLK_PAGEUP:       key = K_PGUP;          break;
			case SDLK_KP_9:         key = K_KP_PGUP;       break;
			case SDLK_PAGEDOWN:     key = K_PGDN;          break;
			case SDLK_KP_3:         key = K_KP_PGDN;       break;
			case SDLK_KP_7:         key = K_KP_HOME;       break;
			case SDLK_HOME:         key = K_HOME;          break;
			case SDLK_KP_1:         key = K_KP_END;        break;
			case SDLK_END:          key = K_END;           break;
			case SDLK_KP_4:         key = K_KP_LEFTARROW;  break;
			case SDLK_LEFT:         key = K_LEFTARROW;     break;
			case SDLK_KP_6:         key = K_KP_RIGHTARROW; break;
			case SDLK_RIGHT:        key = K_RIGHTARROW;    break;
			case SDLK_KP_2:         key = K_KP_DOWNARROW;  break;
			case SDLK_DOWN:         key = K_DOWNARROW;     break;
			case SDLK_KP_8:         key = K_KP_UPARROW;    break;
			case SDLK_UP:           key = K_UPARROW;       break;
			case SDLK_ESCAPE:       key = K_ESCAPE;        break;
			case SDLK_KP_ENTER:     key = K_KP_ENTER;      break;
			case SDLK_RETURN:       key = K_ENTER;         break;
			case SDLK_TAB:          key = K_TAB;           break;
			case SDLK_F1:           key = K_F1;            break;
			case SDLK_F2:           key = K_F2;            break;
			case SDLK_F3:           key = K_F3;            break;
			case SDLK_F4:           key = K_F4;            break;
			case SDLK_F5:           key = K_F5;            break;
			case SDLK_F6:           key = K_F6;            break;
			case SDLK_F7:           key = K_F7;            break;
			case SDLK_F8:           key = K_F8;            break;
			case SDLK_F9:           key = K_F9;            break;
			case SDLK_F10:          key = K_F10;           break;
			case SDLK_F11:          key = K_F11;           break;
			case SDLK_F12:          key = K_F12;           break;
			case SDLK_F13:          key = K_F13;           break;
			case SDLK_F14:          key = K_F14;           break;
			case SDLK_F15:          key = K_F15;           break;

			case SDLK_BACKSPACE:    key = K_BACKSPACE;     break;
			case SDLK_KP_PERIOD:    key = K_KP_DEL;        break;
			case SDLK_DELETE:       key = K_DEL;           break;
			case SDLK_PAUSE:        key = K_PAUSE;         break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:       key = K_SHIFT;         break;

			case SDLK_LCTRL:
			case SDLK_RCTRL:        key = K_CTRL;          break;

#ifdef __APPLE__
			case SDLK_RGUI:
			case SDLK_LGUI:         key = K_COMMAND;       break;
#else
			case SDLK_RGUI:
			case SDLK_LGUI:         key = K_SUPER;         break;
#endif

			case SDLK_RALT:
			case SDLK_LALT:         key = K_ALT;           break;

			case SDLK_KP_5:         key = K_KP_5;          break;
			case SDLK_INSERT:       key = K_INS;           break;
			case SDLK_KP_0:         key = K_KP_INS;        break;
			case SDLK_KP_MULTIPLY:  key = K_KP_STAR;       break;
			case SDLK_KP_PLUS:      key = K_KP_PLUS;       break;
			case SDLK_KP_MINUS:     key = K_KP_MINUS;      break;
			case SDLK_KP_DIVIDE:    key = K_KP_SLASH;      break;

			case SDLK_MODE:         key = K_MODE;          break;
			case SDLK_HELP:         key = K_HELP;          break;
			case SDLK_PRINTSCREEN:  key = K_PRINT;         break;
			case SDLK_SYSREQ:       key = K_SYSREQ;        break;
			case SDLK_MENU:         key = K_MENU;          break;
			case SDLK_APPLICATION:	key = K_MENU;          break;
			case SDLK_POWER:        key = K_POWER;         break;
			case SDLK_UNDO:         key = K_UNDO;          break;
			case SDLK_SCROLLLOCK:   key = K_SCROLLOCK;     break;
			case SDLK_NUMLOCKCLEAR: key = K_KP_NUMLOCK;    break;
			case SDLK_CAPSLOCK:     key = K_CAPSLOCK;      break;

			default:
				if( !( keysym->sym & SDLK_SCANCODE_MASK ) && keysym->scancode <= 95 )
				{
					// Map Unicode characters to 95 world keys using the key's scan code.
					// FIXME: There aren't enough world keys to cover all the scancodes.
					// Maybe create a map of scancode to quake key at start up and on
					// key map change; allocate world key numbers as needed similar
					// to SDL 1.2.
					key = K_WORLD_0 + (int)keysym->scancode;
				}
				break;
		}
	}

	if( in_keyboardDebug->integer )
		IN_PrintKey( keysym, key, down );

	if( IN_IsConsoleKey( key, 0 ) )
	{
		// Console keys can't be bound or generate characters
		key = K_CONSOLE;
	}

	return key;
}

/*
===============
IN_GobbleMotionEvents
===============
*/
static void IN_GobbleMotionEvents( void )
{
	SDL_Event dummy[ 1 ];
	int val = 0;

	// Gobble any mouse motion events
	SDL_PumpEvents( );
	while( ( val = SDL_PeepEvents( dummy, 1, SDL_GETEVENT,
		SDL_MOUSEMOTION, SDL_MOUSEMOTION ) ) > 0 ) { }

	if ( val < 0 )
		Com_Printf( "IN_GobbleMotionEvents failed: %s\n", SDL_GetError( ) );
}

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( qboolean isFullscreen )
{
	if (!mouseAvailable || !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

	if( !mouseActive )
	{
//		SDL_SetRelativeMouseMode( SDL_TRUE );
		SDL_SetWindowGrab( SDL_window, SDL_TRUE );

		IN_GobbleMotionEvents( );
	}

	// in_nograb makes no sense in fullscreen mode
	if( !isFullscreen )
	{
		if( in_nograb->modified || !mouseActive )
		{
			if( in_nograb->integer ) {
				SDL_SetRelativeMouseMode( SDL_FALSE );
				SDL_SetWindowGrab( SDL_window, SDL_FALSE );
			} else {
				SDL_SetRelativeMouseMode( SDL_TRUE );
				SDL_SetWindowGrab( SDL_window, SDL_TRUE );
			}

			in_nograb->modified = qfalse;
		}
	}

	mouseActive = qtrue;
}

/*
===============
IN_DeactivateMouse
===============
*/
static void IN_DeactivateMouse( qboolean isFullscreen )
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

	// Always show the cursor when the mouse is disabled,
	// but not when fullscreen
	if( !isFullscreen )
		SDL_ShowCursor( SDL_TRUE );

	if( !mouseAvailable )
		return;

	if( mouseActive )
	{
		IN_GobbleMotionEvents( );

		SDL_SetWindowGrab( SDL_window, SDL_FALSE );
		SDL_SetRelativeMouseMode( SDL_FALSE );

		// Don't warp the mouse unless the cursor is within the window
		if( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_MOUSE_FOCUS )
			SDL_WarpMouseInWindow( SDL_window, cls.glconfig.vidWidth / 2, cls.glconfig.vidHeight / 2 );

		mouseActive = qfalse;
	}
}

// We translate axes movement into keypresses
static int joy_keys[16] = {
	K_LEFTARROW, K_RIGHTARROW,
	K_UPARROW, K_DOWNARROW,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28
};

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static int hat_keys[16] = {
	K_JOY29, K_JOY30,
	K_JOY31, K_JOY32,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20
};


struct
{
	qboolean buttons[SDL_CONTROLLER_BUTTON_MAX + 1]; // +1 because old max was 16, current SDL_CONTROLLER_BUTTON_MAX is 15
	unsigned int oldaxes;
	int oldaaxes[MAX_JOYSTICK_AXIS];
	unsigned int oldhats;
} stick_state;

#ifdef USE_IOS_GAMECONTROLLER
/*
===============
IN_ApplyDeadzone
===============
*/
static float IN_ApplyDeadzone(float value, float deadzone)
{
	if (fabs(value) < deadzone)
		return 0.0f;
	
	// Scale from deadzone to 1.0
	float sign = value < 0 ? -1.0f : 1.0f;
	return sign * ((fabs(value) - deadzone) / (1.0f - deadzone));
}

/*
===============
IN_GetButtonBinding
===============
*/
static keyNum_t IN_GetButtonBinding(cvar_t *bindCvar)
{
	if (!bindCvar || !bindCvar->string[0])
		return 0;
	
	return Key_StringToKeynum(bindCvar->string);
}

/*
===============
IN_ProcessButtonBinding
Handles both key bindings and direct command bindings
===============
*/
static void IN_ProcessButtonBinding(cvar_t *bindCvar, qboolean pressed)
{
    if (!bindCvar || !bindCvar->string[0])
        return;
    
    const char *binding = bindCvar->string;
    
    
    // Check if this is a command (contains space, starts with +/-, or is a known command)
    if (strchr(binding, ' ') || binding[0] == '+' || binding[0] == '-' ||
        !Q_stricmp(binding, "weapnext") || !Q_stricmp(binding, "weapprev") ||
        !Q_stricmp(binding, "centerview") || !Q_stricmp(binding, "togglemenu"))
    {
        // Handle +/- commands
        if (binding[0] == '+')
        {
            char cmd[MAX_STRING_CHARS];
            if (pressed)
            {
                Cbuf_AddText(binding);
                Cbuf_AddText("\n");
            }
            else
            {
                // Convert +command to -command for release
                Q_strncpyz(cmd, binding, sizeof(cmd));
                cmd[0] = '-';
                Cbuf_AddText(cmd);
                Cbuf_AddText("\n");
            }
        }
        else
        {
            // Regular command, only execute on press
            if (pressed)
            {
                Cbuf_AddText(binding);
                Cbuf_AddText("\n");
            }
        }
    }
    else
    {
        // It's a key binding
        keyNum_t key = Key_StringToKeynum(binding);
        if (key > 0)
        {
            Com_QueueEvent(in_eventTime, SE_KEY, key, pressed, 0, NULL);
        }
    }
}

/*
===============
IN_ToggleiOSKeyboard
===============
*/

static void IN_ToggleiOSKeyboard(void)
{
    extern void Con_ToggleVirtualKeyboard(void);
    Con_ToggleVirtualKeyboard();
}

/*
===============
IN_iOSGameControllerMove
===============
*/

static void IN_iOSGameControllerMove(void)
{
    static qboolean keyboardComboActive = qfalse;
    
    extern qboolean Con_VirtualKeyboardActive(void);
    extern void Con_VirtualKeyboardMove(int dx, int dy);
    extern void Con_VirtualKeyboardSelect(void);
    
    if (!iOS_IsControllerConnected())
        return;
    
    float deadzone = joy_deadzone->value;
    float leftStickSens = joy_leftStickSensitivity->value;
    float rightStickSens = joy_rightStickSensitivity->value;
    
    // Handle analog sticks
    float leftX = IN_ApplyDeadzone(iOS_GetLeftStickX(), deadzone) * leftStickSens;
    float leftY = IN_ApplyDeadzone(iOS_GetLeftStickY(), deadzone) * leftStickSens;
    float rightX = IN_ApplyDeadzone(iOS_GetRightStickX(), deadzone) * rightStickSens;
    float rightY = IN_ApplyDeadzone(iOS_GetRightStickY(), deadzone) * rightStickSens;
    
    // Send analog stick events
    if (in_joystickUseAnalog->integer)
    {
        // Left stick for movement
        Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, j_side_axis->integer, (int)(leftX * 10240), 0, NULL);
        Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, j_forward_axis->integer, (int)(-leftY * 10240), 0, NULL);
        
        // Right stick for looking
        Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, j_yaw_axis->integer, (int)(rightX * 10240), 0, NULL);
        Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, j_pitch_axis->integer, (int)(-rightY * 10240), 0, NULL);
    }
    
    // Check for L3 + R3 combo to toggle console AND virtual keyboard
    if (iOS_GetLeftThumbstickButton() && iOS_GetRightThumbstickButton() && !keyboardComboActive) {
        IN_ToggleiOSKeyboard();  // This will open console AND virtual keyboard
        keyboardComboActive = qtrue;
    } else if (!iOS_GetLeftThumbstickButton() || !iOS_GetRightThumbstickButton()) {
        keyboardComboActive = qfalse;
    }
    
    // Check if we're in game or menu
    int keyCatcher = Key_GetCatcher();
    qboolean inGame = !(keyCatcher & (KEYCATCH_UI | KEYCATCH_CONSOLE));
    
    // VIRTUAL KEYBOARD HANDLING HERE - BEFORE THE BUTTON STRUCTS
    if (Con_VirtualKeyboardActive()) {
        // Virtual keyboard navigation
        static int prevDpadLeft = 0;
        static int prevDpadRight = 0;
        static int prevDpadUp = 0;
        static int prevDpadDown = 0;
        static int prevButtonA = 0;
        
        int currentDpadLeft = iOS_GetDpadLeft();
        int currentDpadRight = iOS_GetDpadRight();
        int currentDpadUp = iOS_GetDpadUp();
        int currentDpadDown = iOS_GetDpadDown();
        int currentButtonA = iOS_GetButtonA();
        
        // D-pad ONLY navigates keyboard, doesn't send console commands
        if (currentDpadLeft && !prevDpadLeft) {
            Con_VirtualKeyboardMove(-1, 0);
        }
        if (currentDpadRight && !prevDpadRight) {
            Con_VirtualKeyboardMove(1, 0);
        }
        if (currentDpadUp && !prevDpadUp) {
            Con_VirtualKeyboardMove(0, -1);
        }
        if (currentDpadDown && !prevDpadDown) {
            Con_VirtualKeyboardMove(0, 1);
        }
        
        // A button selects
        if (currentButtonA && !prevButtonA) {
            Con_VirtualKeyboardSelect();
        }
        
        // Update previous states
        prevDpadLeft = currentDpadLeft;
        prevDpadRight = currentDpadRight;
        prevDpadUp = currentDpadUp;
        prevDpadDown = currentDpadDown;
        prevButtonA = currentButtonA;
        
        // B button is backspace (make sure this is in the virtual keyboard section)
        static int prevButtonB = 0;
        int currentButtonB = iOS_GetButtonB();
        if (currentButtonB && !prevButtonB) {
            extern void Con_VirtualKeyboardBackspace(void);
            Con_VirtualKeyboardBackspace();  // Make sure this is being called
        }
        prevButtonB = currentButtonB;
        
        return; // IMPORTANT: Don't process normal D-pad input while keyboard is active
    }
    
    // Handle buttons with custom bindings
    struct {
        int (*getState)(void);
        cvar_t *bindCvar;
        int *prevState;
        keyNum_t defaultKey;
    } buttonMappings[] = {
        { iOS_GetButtonA, joy_buttonA_bind, &ios_controller_state.buttonA, K_SPACE },
        { iOS_GetButtonB, joy_buttonB_bind, &ios_controller_state.buttonB, K_ENTER },
        { iOS_GetButtonX, joy_buttonX_bind, &ios_controller_state.buttonX, K_CTRL },
        { iOS_GetButtonY, joy_buttonY_bind, &ios_controller_state.buttonY, K_HOME },
        { iOS_GetLeftShoulder, joy_leftShoulder_bind, &ios_controller_state.leftShoulder, K_JOY5 },
        { iOS_GetRightShoulder, joy_rightShoulder_bind, &ios_controller_state.rightShoulder, K_JOY6 },
        { iOS_GetButtonMenu, joy_buttonMenu_bind, &ios_controller_state.buttonMenu, K_ESCAPE },
        { iOS_GetButtonOptions, joy_buttonOptions_bind, &ios_controller_state.buttonOptions, K_TAB },
        { iOS_GetLeftThumbstickButton, joy_leftThumbstickButton_bind, &ios_controller_state.leftThumbstickButton, K_SHIFT },
        { iOS_GetRightThumbstickButton, joy_rightThumbstickButton_bind, &ios_controller_state.rightThumbstickButton, K_HOME }
    };
    
    // D-pad handling
    struct {
        int (*getState)(void);
        int *prevState;
        keyNum_t gameKey;
        keyNum_t menuKey;
        cvar_t *bindCvar;
    } dpadMappings[] = {
        { iOS_GetDpadUp, &ios_controller_state.dpadUp, 0, K_UPARROW, joy_dpadUp_bind },
        { iOS_GetDpadDown, &ios_controller_state.dpadDown, 0, K_DOWNARROW, joy_dpadDown_bind },
        { iOS_GetDpadLeft, &ios_controller_state.dpadLeft, 0, K_LEFTARROW, joy_dpadLeft_bind },
        { iOS_GetDpadRight, &ios_controller_state.dpadRight, 0, K_RIGHTARROW, joy_dpadRight_bind }
    };
    
    int i;
    for (i = 0; i < sizeof(buttonMappings) / sizeof(buttonMappings[0]); i++)
    {
        int currentState = buttonMappings[i].getState();
        if (currentState != *buttonMappings[i].prevState)
        {
            // Normal button processing
            if (buttonMappings[i].bindCvar && buttonMappings[i].bindCvar->string[0])
            {
                IN_ProcessButtonBinding(buttonMappings[i].bindCvar, currentState ? qtrue : qfalse);
            }
            else
            {
                Com_QueueEvent(in_eventTime, SE_KEY, buttonMappings[i].defaultKey,
                              currentState ? qtrue : qfalse, 0, NULL);
            }
            *buttonMappings[i].prevState = currentState;
        }
    }
    
    // Handle D-pad with command support
    for (i = 0; i < sizeof(dpadMappings) / sizeof(dpadMappings[0]); i++)
    {
        int currentState = dpadMappings[i].getState();
        if (currentState != *dpadMappings[i].prevState)
        {
            keyNum_t key = 0;
            qboolean useCommand = qfalse;
            
            if (inGame && dpadMappings[i].bindCvar && dpadMappings[i].bindCvar->string[0])
            {
                // In game, try command binding
                IN_ProcessButtonBinding(dpadMappings[i].bindCvar, currentState ? qtrue : qfalse);
                useCommand = qtrue;
            }
            else if (!inGame)
            {
                // In menu, use arrow keys
                key = dpadMappings[i].menuKey;
            }
            
            if (!useCommand && key)
            {
                Com_QueueEvent(in_eventTime, SE_KEY, key, currentState ? qtrue : qfalse, 0, NULL);
            }
            *dpadMappings[i].prevState = currentState;
        }
    }
    
    // Handle triggers with command support
    float leftTriggerValue = iOS_GetLeftTrigger();
    float rightTriggerValue = iOS_GetRightTrigger();
    
    // MOUSE SIMULATION FOR MENUS
    if (!inGame) {  // Only in menus, not in game
        static float mouseAccumX = 0.0f;
        static float mouseAccumY = 0.0f;
        
        // Use left stick for mouse movement in menus
        float mouseSensitivity = joy_menuMouseSpeed->value;
        mouseAccumX += leftX * mouseSensitivity;
        mouseAccumY += -leftY * mouseSensitivity;  // FLIP Y AXIS with negative
        
        // Send mouse events when we've accumulated enough movement
        int deltaX = (int)mouseAccumX;
        int deltaY = (int)mouseAccumY;
        
        if (deltaX != 0 || deltaY != 0) {
            Com_QueueEvent(in_eventTime, SE_MOUSE, deltaX, deltaY, 0, NULL);
            mouseAccumX -= deltaX;
            mouseAccumY -= deltaY;
        }
        
        // Right trigger acts as mouse click in menus
        static int prevRightTriggerMouse = 0;
        int rightTriggerMousePressed = rightTriggerValue > 0.5f;
        
        if (rightTriggerMousePressed != prevRightTriggerMouse) {
            Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE1,
                          rightTriggerMousePressed ? qtrue : qfalse, 0, NULL);
            prevRightTriggerMouse = rightTriggerMousePressed;
        }
        
        // A button as ENTER in menus
        static int prevMenuButtonA = 0;
        int currentMenuButtonA = iOS_GetButtonA();
        if (currentMenuButtonA != prevMenuButtonA) {
            Com_QueueEvent(in_eventTime, SE_KEY, K_ENTER,
                          currentMenuButtonA ? qtrue : qfalse, 0, NULL);
            prevMenuButtonA = currentMenuButtonA;
        }
        
        // B button as ESCAPE in menus
        static int prevMenuButtonB = 0;
        int currentMenuButtonB = iOS_GetButtonB();
        if (currentMenuButtonB != prevMenuButtonB) {
            Com_QueueEvent(in_eventTime, SE_KEY, K_ESCAPE,
                          currentMenuButtonB ? qtrue : qfalse, 0, NULL);
            prevMenuButtonB = currentMenuButtonB;
        }
    }
    
    // Left trigger
    int leftTriggerPressed = leftTriggerValue > 0.5f;
    if (leftTriggerPressed != ios_controller_state.leftTrigger)
    {
        if (joy_leftTrigger_bind && joy_leftTrigger_bind->string[0])
        {
            IN_ProcessButtonBinding(joy_leftTrigger_bind, leftTriggerPressed ? qtrue : qfalse);
        }
        else
        {
            Com_QueueEvent(in_eventTime, SE_KEY, K_JOY7, leftTriggerPressed ? qtrue : qfalse, 0, NULL);
        }
        ios_controller_state.leftTrigger = leftTriggerPressed;
    }
    
    // Right trigger
    int rightTriggerPressed = rightTriggerValue > 0.5f;
    if (rightTriggerPressed != ios_controller_state.rightTrigger)
    {
        if (joy_rightTrigger_bind && joy_rightTrigger_bind->string[0])
        {
            IN_ProcessButtonBinding(joy_rightTrigger_bind, rightTriggerPressed ? qtrue : qfalse);
        }
        else
        {
            Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE1, rightTriggerPressed ? qtrue : qfalse, 0, NULL);
        }
        ios_controller_state.rightTrigger = rightTriggerPressed;
    }
}

/*
===============
IN_HapticEvent
Trigger haptic feedback on iOS controllers that support it
===============
*/
void IN_HapticEvent(float intensity, int duration)
{
#ifdef USE_IOS_GAMECONTROLLER
    // Initialize iOS GameController FIRST, before any SDL initialization
    Com_Printf("Initializing iOS GameController (bypassing SDL)...\n");
    iOS_InitGameController();
    Com_Printf("iOS GameController support initialized\n");
	
	// Scale intensity by user preference
	intensity *= joy_hapticStrength->value;
	
	// Clamp intensity
	if (intensity > 1.0f) intensity = 1.0f;
	if (intensity < 0.0f) intensity = 0.0f;
	
	// This would call into the iOS GameController haptic API
	// The actual implementation would be in the Objective-C bridge
	iOS_TriggerHaptic(intensity, duration);
#endif
}
#endif // USE_IOS_GAMECONTROLLER

/*
===============
IN_InitJoystick
===============
*/
static void IN_InitJoystick( void )
{
	int i = 0;
	int total = 0;
	char buf[16384] = "";

#ifdef USE_IOS_GAMECONTROLLER
    // Initialize iOS GameController FIRST, before any SDL initialization
    Com_Printf("Initializing iOS GameController (bypassing SDL)...\n");
    iOS_InitGameController();
    Com_Printf("iOS GameController support initialized\n");
    
    // Force analog mode for iOS controllers
    in_joystickUseAnalog = Cvar_Get( "in_joystickUseAnalog", "1", CVAR_ARCHIVE );
    Cvar_Set( "in_joystickUseAnalog", "1" );
	
    // Initialize button mapping cvars with direct commands
    joy_deadzone = Cvar_Get("joy_deadzone", "0.15", CVAR_ARCHIVE);
    joy_sensitivity = Cvar_Get("joy_sensitivity", "1.0", CVAR_ARCHIVE);
    joy_hapticStrength = Cvar_Get("joy_hapticStrength", "1.0", CVAR_ARCHIVE);
    joy_menuMouseSpeed = Cvar_Get("joy_menuMouseSpeed", "20.0", CVAR_ARCHIVE);

    // Sensitivity adjustments
    joy_leftStickSensitivity = Cvar_Get("joy_leftStickSensitivity", "1.0", CVAR_ARCHIVE);
    joy_rightStickSensitivity = Cvar_Get("joy_rightStickSensitivity", "5.0", CVAR_ARCHIVE);

    // Button bindings with direct commands
    joy_buttonA_bind = Cvar_Get("joy_buttonA_bind", "+moveup", CVAR_ARCHIVE);  // Jump
    joy_buttonB_bind = Cvar_Get("joy_buttonB_bind", "+button2", CVAR_ARCHIVE);  // Use Item
    joy_buttonX_bind = Cvar_Get("joy_buttonX_bind", "+movedown", CVAR_ARCHIVE);  // Crouch
    joy_buttonY_bind = Cvar_Get("joy_buttonY_bind", "+gesture", CVAR_ARCHIVE);  // Make Gesture
    joy_leftShoulder_bind = Cvar_Get("joy_leftShoulder_bind", "+moveup", CVAR_ARCHIVE);  // Jump
    joy_rightShoulder_bind = Cvar_Get("joy_rightShoulder_bind", "weapnext", CVAR_ARCHIVE);  // Next weapon
    joy_leftTrigger_bind = Cvar_Get("joy_leftTrigger_bind", "+zoom", CVAR_ARCHIVE);  // Zoom
    joy_rightTrigger_bind = Cvar_Get("joy_rightTrigger_bind", "+attack", CVAR_ARCHIVE);  // Fire
    joy_dpadUp_bind = Cvar_Get("joy_dpadUp_bind", "", CVAR_ARCHIVE);  // Empty
    joy_dpadDown_bind = Cvar_Get("joy_dpadDown_bind", "drop", CVAR_ARCHIVE);  // drop
    joy_dpadLeft_bind = Cvar_Get("joy_dpadLeft_bind", "weapprev", CVAR_ARCHIVE);  // Previous weapon
    joy_dpadRight_bind = Cvar_Get("joy_dpadRight_bind", "weapnext", CVAR_ARCHIVE);  // Next weapon
    joy_buttonMenu_bind = Cvar_Get("joy_buttonMenu_bind", "togglemenu", CVAR_ARCHIVE);  // Menu toggle
    joy_buttonOptions_bind = Cvar_Get("joy_buttonOptions_bind", "+scores", CVAR_ARCHIVE);  // Show scores
    joy_leftThumbstickButton_bind = Cvar_Get("joy_leftThumbstickButton_bind", "+speed", CVAR_ARCHIVE);  // Hold for walk
    joy_rightThumbstickButton_bind = Cvar_Get("joy_rightThumbstickButton_bind", "centerview", CVAR_ARCHIVE);  // Center view
	
    // Force always run by default
    Cvar_Set("cl_run", "1");
	
	// Set up proper joystick axis mappings for analog movement
	// These cvars control which axis number is used for each movement direction
    Cvar_Get("j_forward_axis", "1", CVAR_ARCHIVE);    // Left stick Y axis
    Cvar_Get("j_side_axis", "0", CVAR_ARCHIVE);       // Left stick X axis
    Cvar_Get("j_pitch_axis", "3", CVAR_ARCHIVE);      // Right stick Y axis
    Cvar_Get("j_yaw_axis", "2", CVAR_ARCHIVE);        // Right stick X axis
    Cvar_Get("j_up_axis", "4", CVAR_ARCHIVE);         // Not typically used

    // Set axis sensitivities - reduce these back down since we're using 10240
    Cvar_Get("j_forward", "-25", CVAR_ARCHIVE);       // Movement forward/back
    Cvar_Get("j_side", "25", CVAR_ARCHIVE);           // Movement strafe
    Cvar_Get("j_pitch", "0.004", CVAR_ARCHIVE);      // Much lower since we're using 10240
    Cvar_Get("j_yaw", "-0.004", CVAR_ARCHIVE);       // Much lower since we're using 10240
    
    // joystick mouse sensitivity
    joy_menuMouseSpeed = Cvar_Get("joy_menuMouseSpeed", "20.0", CVAR_ARCHIVE);
	
	// Clear controller state
	memset(&ios_controller_state, 0, sizeof(ios_controller_state));
    	
	// Don't initialize SDL joystick system on iOS
	return;
#endif

	if (gamepad)
		SDL_GameControllerClose(gamepad);

	if (stick != NULL)
		SDL_JoystickClose(stick);

	stick = NULL;
	gamepad = NULL;
	memset(&stick_state, '\0', sizeof (stick_state));

	// SDL 2.0.4 requires SDL_INIT_JOYSTICK to be initialized separately from
	// SDL_INIT_GAMECONTROLLER for SDL_JoystickOpen() to work correctly,
	// despite https://wiki.libsdl.org/SDL_Init (retrieved 2016-08-16)
	// indicating SDL_INIT_JOYSTICK should be initialized automatically.
	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		Com_DPrintf("Calling SDL_Init(SDL_INIT_JOYSTICK)...\n");
		if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
		{
			Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) passed.\n");
	}

	if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER))
	{
		Com_DPrintf("Calling SDL_Init(SDL_INIT_GAMECONTROLLER)...\n");
		if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
		{
			Com_DPrintf("SDL_Init(SDL_INIT_GAMECONTROLLER) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_Init(SDL_INIT_GAMECONTROLLER) passed.\n");
	}

	total = SDL_NumJoysticks();
	Com_DPrintf("%d possible joysticks\n", total);

	// Print list and build cvar to allow ui to select joystick.
	for (i = 0; i < total; i++)
	{
		Q_strcat(buf, sizeof(buf), SDL_JoystickNameForIndex(i));
		Q_strcat(buf, sizeof(buf), "\n");
	}

	Cvar_Get( "in_availableJoysticks", buf, CVAR_ROM );

	if( !in_joystick->integer ) {
		Com_DPrintf( "Joystick is not active.\n" );
		SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
		return;
	}

	in_joystickNo = Cvar_Get( "in_joystickNo", "0", CVAR_ARCHIVE );
	if( in_joystickNo->integer < 0 || in_joystickNo->integer >= total )
		Cvar_Set( "in_joystickNo", "0" );

	in_joystickUseAnalog = Cvar_Get( "in_joystickUseAnalog", "0", CVAR_ARCHIVE );

	stick = SDL_JoystickOpen( in_joystickNo->integer );

	if (stick == NULL) {
		Com_DPrintf( "No joystick opened: %s\n", SDL_GetError() );
		return;
	}

	if (SDL_IsGameController(in_joystickNo->integer))
		gamepad = SDL_GameControllerOpen(in_joystickNo->integer);

	Com_DPrintf( "Joystick %d opened\n", in_joystickNo->integer );
	Com_DPrintf( "Name:       %s\n", SDL_JoystickNameForIndex(in_joystickNo->integer) );
	Com_DPrintf( "Axes:       %d\n", SDL_JoystickNumAxes(stick) );
	Com_DPrintf( "Hats:       %d\n", SDL_JoystickNumHats(stick) );
	Com_DPrintf( "Buttons:    %d\n", SDL_JoystickNumButtons(stick) );
	Com_DPrintf( "Balls:      %d\n", SDL_JoystickNumBalls(stick) );
	Com_DPrintf( "Use Analog: %s\n", in_joystickUseAnalog->integer ? "Yes" : "No" );
	Com_DPrintf( "Is gamepad: %s\n", gamepad ? "Yes" : "No" );

	SDL_JoystickEventState(SDL_QUERY);
	SDL_GameControllerEventState(SDL_QUERY);
}

/*
===============
IN_ShutdownJoystick
===============
*/
static void IN_ShutdownJoystick( void )
{
#ifndef USE_IOS_GAMECONTROLLER
	if ( !SDL_WasInit( SDL_INIT_GAMECONTROLLER ) )
		return;

	if ( !SDL_WasInit( SDL_INIT_JOYSTICK ) )
		return;

	if (gamepad)
	{
		SDL_GameControllerClose(gamepad);
		gamepad = NULL;
	}

	if (stick)
	{
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
#endif
}


static qboolean KeyToAxisAndSign(int keynum, int *outAxis, int *outSign)
{
	char *bind;

	if (!keynum)
		return qfalse;

	bind = Key_GetBinding(keynum);

	if (!bind || *bind != '+')
		return qfalse;

	*outSign = 0;

	if (Q_stricmp(bind, "+forward") == 0)
	{
		*outAxis = j_forward_axis->integer;
		*outSign = j_forward->value > 0.0f ? 1 : -1;
	}
	else if (Q_stricmp(bind, "+back") == 0)
	{
		*outAxis = j_forward_axis->integer;
		*outSign = j_forward->value > 0.0f ? -1 : 1;
	}
	else if (Q_stricmp(bind, "+moveleft") == 0)
	{
		*outAxis = j_side_axis->integer;
		*outSign = j_side->value > 0.0f ? -1 : 1;
	}
	else if (Q_stricmp(bind, "+moveright") == 0)
	{
		*outAxis = j_side_axis->integer;
		*outSign = j_side->value > 0.0f ? 1 : -1;
	}
	else if (Q_stricmp(bind, "+lookup") == 0)
	{
		*outAxis = j_pitch_axis->integer;
		*outSign = j_pitch->value > 0.0f ? -1 : 1;
	}
	else if (Q_stricmp(bind, "+lookdown") == 0)
	{
		*outAxis = j_pitch_axis->integer;
		*outSign = j_pitch->value > 0.0f ? 1 : -1;
	}
	else if (Q_stricmp(bind, "+left") == 0)
	{
		*outAxis = j_yaw_axis->integer;
		*outSign = j_yaw->value > 0.0f ? 1 : -1;
	}
	else if (Q_stricmp(bind, "+right") == 0)
	{
		*outAxis = j_yaw_axis->integer;
		*outSign = j_yaw->value > 0.0f ? -1 : 1;
	}
	else if (Q_stricmp(bind, "+moveup") == 0)
	{
		*outAxis = j_up_axis->integer;
		*outSign = j_up->value > 0.0f ? 1 : -1;
	}
	else if (Q_stricmp(bind, "+movedown") == 0)
	{
		*outAxis = j_up_axis->integer;
		*outSign = j_up->value > 0.0f ? -1 : 1;
	}

	return *outSign != 0;
}

/*
===============
IN_GamepadMove
===============
*/
static void IN_GamepadMove( void )
{
	int i;
	int translatedAxes[MAX_JOYSTICK_AXIS];
	qboolean translatedAxesSet[MAX_JOYSTICK_AXIS];

	SDL_GameControllerUpdate();

	// check buttons
	for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
	{
		qboolean pressed = SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_A + i);
		if (pressed != stick_state.buttons[i])
		{
			Com_QueueEvent(in_eventTime, SE_KEY, K_PAD0_A + i, pressed, 0, NULL);
			stick_state.buttons[i] = pressed;
		}
	}

	// must defer translated axes until all real axes are processed
	// must be done this way to prevent a later mapped axis from zeroing out a previous one
	if (in_joystickUseAnalog->integer)
	{
		for (i = 0; i < MAX_JOYSTICK_AXIS; i++)
		{
			translatedAxes[i] = 0;
			translatedAxesSet[i] = qfalse;
		}
	}

	// check axes
	for (i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
	{
		int axis = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX + i);
		int oldAxis = stick_state.oldaaxes[i];

		// Smoothly ramp from dead zone to maximum value
		float f = ((float)abs(axis) / 32767.0f - in_joystickThreshold->value) / (1.0f - in_joystickThreshold->value);

		if (f < 0.0f)
			f = 0.0f;

		axis = (int)(32767 * ((axis < 0) ? -f : f));

		if (axis != oldAxis)
		{
			const int negMap[SDL_CONTROLLER_AXIS_MAX] = { K_PAD0_LEFTSTICK_LEFT,  K_PAD0_LEFTSTICK_UP,   K_PAD0_RIGHTSTICK_LEFT,  K_PAD0_RIGHTSTICK_UP, 0, 0 };
			const int posMap[SDL_CONTROLLER_AXIS_MAX] = { K_PAD0_LEFTSTICK_RIGHT, K_PAD0_LEFTSTICK_DOWN, K_PAD0_RIGHTSTICK_RIGHT, K_PAD0_RIGHTSTICK_DOWN, K_PAD0_LEFTTRIGGER, K_PAD0_RIGHTTRIGGER };

			qboolean posAnalog = qfalse, negAnalog = qfalse;
			int negKey = negMap[i];
			int posKey = posMap[i];

			if (in_joystickUseAnalog->integer)
			{
				int posAxis = 0, posSign = 0, negAxis = 0, negSign = 0;

				// get axes and axes signs for keys if available
				posAnalog = KeyToAxisAndSign(posKey, &posAxis, &posSign);
				negAnalog = KeyToAxisAndSign(negKey, &negAxis, &negSign);

				// positive to negative/neutral -> keyup if axis hasn't yet been set
				if (posAnalog && !translatedAxesSet[posAxis] && oldAxis > 0 && axis <= 0)
				{
					translatedAxes[posAxis] = 0;
					translatedAxesSet[posAxis] = qtrue;
				}

				// negative to positive/neutral -> keyup if axis hasn't yet been set
				if (negAnalog && !translatedAxesSet[negAxis] && oldAxis < 0 && axis >= 0)
				{
					translatedAxes[negAxis] = 0;
					translatedAxesSet[negAxis] = qtrue;
				}

				// negative/neutral to positive -> keydown
				if (posAnalog && axis > 0)
				{
					translatedAxes[posAxis] = axis * posSign;
					translatedAxesSet[posAxis] = qtrue;
				}

				// positive/neutral to negative -> keydown
				if (negAnalog && axis < 0)
				{
					translatedAxes[negAxis] = -axis * negSign;
					translatedAxesSet[negAxis] = qtrue;
				}
			}

			// keyups first so they get overridden by keydowns later

			// positive to negative/neutral -> keyup
			if (!posAnalog && posKey && oldAxis > 0 && axis <= 0)
				Com_QueueEvent(in_eventTime, SE_KEY, posKey, qfalse, 0, NULL);

			// negative to positive/neutral -> keyup
			if (!negAnalog && negKey && oldAxis < 0 && axis >= 0)
				Com_QueueEvent(in_eventTime, SE_KEY, negKey, qfalse, 0, NULL);

			// negative/neutral to positive -> keydown
			if (!posAnalog && posKey && oldAxis <= 0 && axis > 0)
				Com_QueueEvent(in_eventTime, SE_KEY, posKey, qtrue, 0, NULL);

			// positive/neutral to negative -> keydown
			if (!negAnalog && negKey && oldAxis >= 0 && axis < 0)
				Com_QueueEvent(in_eventTime, SE_KEY, negKey, qtrue, 0, NULL);

			stick_state.oldaaxes[i] = axis;
		}
	}

	// set translated axes
	if (in_joystickUseAnalog->integer)
	{
		for (i = 0; i < MAX_JOYSTICK_AXIS; i++)
		{
			if (translatedAxesSet[i])
				Com_QueueEvent(in_eventTime, SE_JOYSTICK_AXIS, i, translatedAxes[i], 0, NULL);
		}
	}
}


/*
===============
IN_JoyMove
===============
*/
static void IN_JoyMove( void )
{
#ifdef USE_IOS_GAMECONTROLLER
	IN_iOSGameControllerMove();
	return;
#endif

	unsigned int axes = 0;
	unsigned int hats = 0;
	int total = 0;
	int i = 0;

	if (gamepad)
	{
		IN_GamepadMove();
		return;
	}

	if (!stick)
		return;

	SDL_JoystickUpdate();

	// update the ball state.
	total = SDL_JoystickNumBalls(stick);
	if (total > 0)
	{
		int balldx = 0;
		int balldy = 0;
		for (i = 0; i < total; i++)
		{
			int dx = 0;
			int dy = 0;
			SDL_JoystickGetBall(stick, i, &dx, &dy);
			balldx += dx;
			balldy += dy;
		}
		if (balldx || balldy)
		{
			// !!! FIXME: is this good for stick balls, or just mice?
			// Scale like the mouse input...
			if (abs(balldx) > 1)
				balldx *= 2;
			if (abs(balldy) > 1)
				balldy *= 2;
			Com_QueueEvent( in_eventTime, SE_MOUSE, balldx, balldy, 0, NULL );
		}
	}

	// now query the stick buttons...
	total = SDL_JoystickNumButtons(stick);
	if (total > 0)
	{
		if (total > ARRAY_LEN(stick_state.buttons))
			total = ARRAY_LEN(stick_state.buttons);
		for (i = 0; i < total; i++)
		{
			qboolean pressed = (SDL_JoystickGetButton(stick, i) != 0);
			if (pressed != stick_state.buttons[i])
			{
				Com_QueueEvent( in_eventTime, SE_KEY, K_JOY1 + i, pressed, 0, NULL );
				stick_state.buttons[i] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats(stick);
	if (total > 0)
	{
		if (total > 4) total = 4;
		for (i = 0; i < total; i++)
		{
			((Uint8 *)&hats)[i] = SDL_JoystickGetHat(stick, i);
		}
	}

	// update hat state
	if (hats != stick_state.oldhats)
	{
		for( i = 0; i < 4; i++ ) {
			if( ((Uint8 *)&hats)[i] != ((Uint8 *)&stick_state.oldhats)[i] ) {
				// release event
				switch( ((Uint8 *)&stick_state.oldhats)[i] ) {
					case SDL_HAT_UP:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					default:
						break;
				}
				// press event
				switch( ((Uint8 *)&hats)[i] ) {
					case SDL_HAT_UP:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						Com_QueueEvent( in_eventTime, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					default:
						break;
				}
			}
		}
	}

	// save hat state
	stick_state.oldhats = hats;

	// finally, look at the axes...
	total = SDL_JoystickNumAxes(stick);
	if (total > 0)
	{
		if (in_joystickUseAnalog->integer)
		{
			if (total > MAX_JOYSTICK_AXIS) total = MAX_JOYSTICK_AXIS;
			for (i = 0; i < total; i++)
			{
				Sint16 axis = SDL_JoystickGetAxis(stick, i);
				float f = ( (float) abs(axis) ) / 32767.0f;
				
				if( f < in_joystickThreshold->value ) axis = 0;

				if ( axis != stick_state.oldaaxes[i] )
				{
					Com_QueueEvent( in_eventTime, SE_JOYSTICK_AXIS, i, axis, 0, NULL );
					stick_state.oldaaxes[i] = axis;
				}
			}
		}
		else
		{
			if (total > 16) total = 16;
			for (i = 0; i < total; i++)
			{
				Sint16 axis = SDL_JoystickGetAxis(stick, i);
				float f = ( (float) axis ) / 32767.0f;
				if( f < -in_joystickThreshold->value ) {
					axes |= ( 1 << ( i * 2 ) );
				} else if( f > in_joystickThreshold->value ) {
					axes |= ( 1 << ( ( i * 2 ) + 1 ) );
				}
			}
		}
	}

	/* Time to update axes state based on old vs. new. */
	if (axes != stick_state.oldaxes)
	{
		for( i = 0; i < 16; i++ ) {
			if( ( axes & ( 1 << i ) ) && !( stick_state.oldaxes & ( 1 << i ) ) ) {
				Com_QueueEvent( in_eventTime, SE_KEY, joy_keys[i], qtrue, 0, NULL );
			}

			if( !( axes & ( 1 << i ) ) && ( stick_state.oldaxes & ( 1 << i ) ) ) {
				Com_QueueEvent( in_eventTime, SE_KEY, joy_keys[i], qfalse, 0, NULL );
			}
		}
	}

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}

/*
===============
 HandleTouchInput
===============
*/

static void HandleTouchInput(float normalizedX, float normalizedY, qboolean isPressed) {
    int screenWidth = cls.glconfig.vidWidth;
    int screenHeight = cls.glconfig.vidHeight;
    int screenX = (int)(normalizedX * screenWidth);
    int screenY = (int)(normalizedY * screenHeight);
    
    extern qboolean Con_VirtualKeyboardActive(void);
    
    if (Con_VirtualKeyboardActive()) {
        extern void Con_VirtualKeyboardTouch(float x, float y, float consoleHeight);
        float consoleHeight = screenHeight * 0.5f;
        Con_VirtualKeyboardTouch((float)screenX, (float)screenY, consoleHeight);
        return;
    }
    else if ((Key_GetCatcher() & KEYCATCH_UI) && !(Key_GetCatcher() & KEYCATCH_CONSOLE)) {
        if (isPressed == qtrue) {
            // Show navigation arrows on any touch
            extern void Con_TouchNavigation_SetVisible(void);
            Con_TouchNavigation_SetVisible();
            
            // Calculate touch zones based on new arrow positions
            float centerX = 0.5f;
            float centerY = 0.5f;
            
            // Convert pixel distance to normalized coordinates
            float arrowDistanceX = 300.0f / screenWidth;   // 300 pixels from center
            float arrowDistanceY = 300.0f / screenHeight;  // 300 pixels from center
            
            float arrowTouchSize = 0.2f;   // Touch zones for arrows
            float centerTouchSize = 0.15f; // Smaller zone specifically for center
                        
            qboolean actionTaken = qfalse;
            
            // CHECK CENTER FIRST (highest priority)
            float distanceFromCenterX = fabs(normalizedX - centerX);
            float distanceFromCenterY = fabs(normalizedY - centerY);
            
            if (distanceFromCenterX <= centerTouchSize/2 && distanceFromCenterY <= centerTouchSize/2) {
                Com_QueueEvent(in_eventTime, SE_KEY, K_ENTER, qtrue, 0, NULL);
                Com_QueueEvent(in_eventTime, SE_KEY, K_ENTER, qfalse, 0, NULL);
                actionTaken = qtrue;
            }
            
            // Only check arrows if center wasn't touched
            if (!actionTaken) {
                // UP arrow zone
                float upY = centerY - arrowDistanceY;
                float distanceFromUpX = fabs(normalizedX - centerX);
                float distanceFromUpY = fabs(normalizedY - upY);
                
                if (distanceFromUpX <= arrowTouchSize/2 && distanceFromUpY <= arrowTouchSize/2) {
                    Com_QueueEvent(in_eventTime, SE_KEY, K_UPARROW, qtrue, 0, NULL);
                    Com_QueueEvent(in_eventTime, SE_KEY, K_UPARROW, qfalse, 0, NULL);
                    actionTaken = qtrue;
                }
            }
            
            if (!actionTaken) {
                // DOWN arrow zone
                float downY = centerY + arrowDistanceY;
                float distanceFromDownX = fabs(normalizedX - centerX);
                float distanceFromDownY = fabs(normalizedY - downY);
                
                if (distanceFromDownX <= arrowTouchSize/2 && distanceFromDownY <= arrowTouchSize/2) {
                    Com_QueueEvent(in_eventTime, SE_KEY, K_DOWNARROW, qtrue, 0, NULL);
                    Com_QueueEvent(in_eventTime, SE_KEY, K_DOWNARROW, qfalse, 0, NULL);
                    actionTaken = qtrue;
                }
            }
            
            if (!actionTaken) {
                // LEFT arrow zone
                float leftX = centerX - arrowDistanceX;
                float distanceFromLeftX = fabs(normalizedX - leftX);
                float distanceFromLeftY = fabs(normalizedY - centerY);
                
                if (distanceFromLeftX <= arrowTouchSize/2 && distanceFromLeftY <= arrowTouchSize/2) {
                    Com_QueueEvent(in_eventTime, SE_KEY, K_LEFTARROW, qtrue, 0, NULL);
                    Com_QueueEvent(in_eventTime, SE_KEY, K_LEFTARROW, qfalse, 0, NULL);
                    actionTaken = qtrue;
                }
            }
            
            if (!actionTaken) {
                // RIGHT arrow zone
                float rightX = centerX + arrowDistanceX;
                float distanceFromRightX = fabs(normalizedX - rightX);
                float distanceFromRightY = fabs(normalizedY - centerY);
                
                if (distanceFromRightX <= arrowTouchSize/2 && distanceFromRightY <= arrowTouchSize/2) {
                    Com_QueueEvent(in_eventTime, SE_KEY, K_RIGHTARROW, qtrue, 0, NULL);
                    Com_QueueEvent(in_eventTime, SE_KEY, K_RIGHTARROW, qfalse, 0, NULL);
                    actionTaken = qtrue;
                }
            }
            
            // ESC zone (larger and more forgiving)
            if (!actionTaken) {
                float escLeft = 60.0f / screenWidth;    // 20 pixels more to the left
                float escRight = 220.0f / screenWidth;  // 20 pixels more to the right
                float escTop = 60.0f / screenHeight;    // 20 pixels higher
                float escBottom = 160.0f / screenHeight; // 20 pixels lower
                
                if (normalizedX >= escLeft && normalizedX <= escRight &&
                    normalizedY >= escTop && normalizedY <= escBottom) {
                    Com_QueueEvent(in_eventTime, SE_KEY, K_ESCAPE, qtrue, 0, NULL);
                    Com_QueueEvent(in_eventTime, SE_KEY, K_ESCAPE, qfalse, 0, NULL);
                    actionTaken = qtrue;
                }
            }

            // TAB zone (larger and positioned lower)
            if (!actionTaken) {
                float tabLeft = 60.0f / screenWidth;    // 20 pixels more to the left
                float tabRight = 220.0f / screenWidth;  // 20 pixels more to the right
                float tabTop = 310.0f / screenHeight;   // Updated for new position
                float tabBottom = 410.0f / screenHeight; // 20 pixels larger zone
                
                if (normalizedX >= tabLeft && normalizedX <= tabRight &&
                    normalizedY >= tabTop && normalizedY <= tabBottom) {
                    Com_QueueEvent(in_eventTime, SE_KEY, K_TAB, qtrue, 0, NULL);
                    Com_QueueEvent(in_eventTime, SE_KEY, K_TAB, qfalse, 0, NULL);
                    actionTaken = qtrue;
                }
            }
            // For navigation arrows, don't send mouse events either
            return;
        }
    }
    // Only send mouse events for non-virtual keyboard touches
    if (isPressed != -1) {
        Com_QueueEvent(in_eventTime, SE_KEY, K_MOUSE1, isPressed, 0, NULL);
    }
}

/*
===============
IN_ProcessEvents
===============
*/
static void IN_ProcessEvents( void )
{
	SDL_Event e;
	keyNum_t key = 0;
	static keyNum_t lastKeyDown = 0;
    static Uint32 consoleOpenTime = 0;

	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
			return;

	while( SDL_PollEvent( &e ) )
	{
		switch( e.type )
		{
            case SDL_KEYDOWN:
                if ( e.key.repeat && Key_GetCatcher( ) == 0 )
                    break;

                if( ( key = IN_TranslateSDLToQ3Key( &e.key.keysym, qtrue ) ) )
                {
                    // Check if this is the console key
                    if( key == K_CONSOLE )
                    {
                        consoleOpenTime = SDL_GetTicks();  // Record when console was opened
                    }
                    
                    Com_QueueEvent( in_eventTime, SE_KEY, key, qtrue, 0, NULL );
                }

                // Special handling for console input
                if( Key_GetCatcher() & KEYCATCH_CONSOLE ) {
                    // Don't send modifier keys as characters
                    if( key == K_SHIFT || key == K_CTRL || key == K_ALT )
                        break;
                        
                    if( key == K_BACKSPACE )
                        Com_QueueEvent( in_eventTime, SE_CHAR, CTRL('h'), 0, 0, NULL );
                    else if( keys[K_CTRL].down && key >= 'a' && key <= 'z' )
                        Com_QueueEvent( in_eventTime, SE_CHAR, CTRL(key), 0, 0, NULL );
                }

                lastKeyDown = key;
                break;

			case SDL_KEYUP:
				if( ( key = IN_TranslateSDLToQ3Key( &e.key.keysym, qfalse ) ) )
					Com_QueueEvent( in_eventTime, SE_KEY, key, qfalse, 0, NULL );

				lastKeyDown = 0;
				break;

            case SDL_TEXTINPUT:
                // Don't process text input if it's the console key
                if( e.text.text[0] == '`' || e.text.text[0] == '~' ) {
                    // Check if we just opened the console (within 100ms)
                    if( SDL_GetTicks() - consoleOpenTime < 100 ) {
                        break;  // Ignore this character
                    }
                }
                
                // Process all other text input
                {
                    char *c = e.text.text;
                    
                    // Quick and dirty UTF-8 to UTF-32 conversion
                    while( *c )
                    {
                        int utf32 = 0;
                        
                        if( ( *c & 0x80 ) == 0 )
                            utf32 = *c++;
                        else if( ( *c & 0xE0 ) == 0xC0 ) // 110x xxxx
                        {
                            utf32 |= ( *c++ & 0x1F ) << 6;
                            utf32 |= ( *c++ & 0x3F );
                        }
                        else if( ( *c & 0xF0 ) == 0xE0 ) // 1110 xxxx
                        {
                            utf32 |= ( *c++ & 0x0F ) << 12;
                            utf32 |= ( *c++ & 0x3F ) << 6;
                            utf32 |= ( *c++ & 0x3F );
                        }
                        else if( ( *c & 0xF8 ) == 0xF0 ) // 1111 0xxx
                        {
                            utf32 |= ( *c++ & 0x07 ) << 18;
                            utf32 |= ( *c++ & 0x3F ) << 12;
                            utf32 |= ( *c++ & 0x3F ) << 6;
                            utf32 |= ( *c++ & 0x3F );
                        }
                        else
                        {
                            Com_DPrintf( "Unrecognised UTF-8 lead byte: 0x%x\n", (unsigned int)*c );
                            c++;
                        }
                        
                        if( utf32 != 0 )
                        {
                            // Send the character - SDL has already handled shift
                            Com_QueueEvent( in_eventTime, SE_CHAR, utf32, 0, 0, NULL );
                        }
                    }
                }
                break;

			case SDL_MOUSEMOTION:
				if( mouseActive )
				{
					if( !e.motion.xrel && !e.motion.yrel )
						break;
					Com_QueueEvent( in_eventTime, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL );
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				{
					int b;
					switch( e.button.button )
					{
						case SDL_BUTTON_LEFT:   b = K_MOUSE1;     break;
						case SDL_BUTTON_MIDDLE: b = K_MOUSE3;     break;
						case SDL_BUTTON_RIGHT:  b = K_MOUSE2;     break;
						case SDL_BUTTON_X1:     b = K_MOUSE4;     break;
						case SDL_BUTTON_X2:     b = K_MOUSE5;     break;
						default:                b = K_AUX1 + ( e.button.button - SDL_BUTTON_X2 + 1 ) % 16; break;
					}
					Com_QueueEvent( in_eventTime, SE_KEY, b,
						( e.type == SDL_MOUSEBUTTONDOWN ? qtrue : qfalse ), 0, NULL );
				}
				break;

			case SDL_MOUSEWHEEL:
				if( e.wheel.y > 0 )
				{
					Com_QueueEvent( in_eventTime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
					Com_QueueEvent( in_eventTime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
				}
				else if( e.wheel.y < 0 )
				{
					Com_QueueEvent( in_eventTime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
					Com_QueueEvent( in_eventTime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
				}
				break;

			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
#ifndef USE_IOS_GAMECONTROLLER
				if (in_joystick->integer)
					IN_InitJoystick();
#endif
				break;

			case SDL_QUIT:
				Cbuf_ExecuteText(EXEC_NOW, "quit Closed window\n");
				break;

			case SDL_WINDOWEVENT:
				switch( e.window.event )
				{
					case SDL_WINDOWEVENT_RESIZED:
						{
							int width, height;

							width = e.window.data1;
							height = e.window.data2;

							// ignore this event on fullscreen
							if( cls.glconfig.isFullscreen )
							{
								break;
							}

							// check if size actually changed
							if( cls.glconfig.vidWidth == width && cls.glconfig.vidHeight == height )
							{
								break;
							}

							Cvar_SetValue( "r_customwidth", width );
							Cvar_SetValue( "r_customheight", height );
							Cvar_Set( "r_mode", "-1" );

							// Wait until user stops dragging for 1 second, so
							// we aren't constantly recreating the GL context while
							// he tries to drag...
							vidRestartTime = Sys_Milliseconds( ) + 1000;
						}
						break;

					case SDL_WINDOWEVENT_MINIMIZED:    Cvar_SetValue( "com_minimized", 1 ); break;
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_MAXIMIZED:    Cvar_SetValue( "com_minimized", 0 ); break;
					case SDL_WINDOWEVENT_FOCUS_LOST:   Cvar_SetValue( "com_unfocused", 1 ); break;
					case SDL_WINDOWEVENT_FOCUS_GAINED: Cvar_SetValue( "com_unfocused", 0 ); break;
				}
				break;
                
            case SDL_FINGERDOWN:
                #ifdef USE_IOS_GAMECONTROLLER
                // Skip touch if controller is active and we're in game
                if (iOS_IsControllerConnected() && !(Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CONSOLE))) {
                    break;
                }
                #endif
                
                HandleTouchInput(e.tfinger.x, e.tfinger.y, qtrue);
                break;

            case SDL_FINGERUP:
                #ifdef USE_IOS_GAMECONTROLLER
                if (iOS_IsControllerConnected() && !(Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CONSOLE))) {
                    break;
                }
                #endif
                
                // For virtual keyboard, don't process finger up at all
                extern qboolean Con_VirtualKeyboardActive(void);
                if (!Con_VirtualKeyboardActive()) {
                    HandleTouchInput(e.tfinger.x, e.tfinger.y, qfalse);
                }
                break;

            case SDL_FINGERMOTION:
                #ifdef USE_IOS_GAMECONTROLLER
                if (iOS_IsControllerConnected() && !(Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CONSOLE))) {
                    break;
                }
                #endif
                
                // Motion only (no click)
                HandleTouchInput(e.tfinger.x, e.tfinger.y, -1);
                break;


			default:
				break;
		}
	}
}

/*
===============
IN_Frame
===============
*/
void IN_Frame( void )
{
	qboolean loading;
    
    // Ensure text input is enabled when console is active
    static qboolean wasConsoleActive = qfalse;
    qboolean isConsoleActive = (Key_GetCatcher() & KEYCATCH_CONSOLE) != 0;
    
    if (isConsoleActive && !wasConsoleActive) {
        SDL_StartTextInput();
    } else if (!isConsoleActive && wasConsoleActive) {
        // Don't disable text input on iOS - we might need it
        #ifndef IOS
        SDL_StopTextInput();
        #endif
    }
    wasConsoleActive = isConsoleActive;
    
    IN_JoyMove( );

	// If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
	loading = ( clc.state != CA_DISCONNECTED && clc.state != CA_ACTIVE );

	// update isFullscreen since it might of changed since the last vid_restart
	cls.glconfig.isFullscreen = Cvar_VariableIntegerValue( "r_fullscreen" ) != 0;

	if( !cls.glconfig.isFullscreen && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) )
	{
		// Console is down in windowed mode
		IN_DeactivateMouse( cls.glconfig.isFullscreen );
	}
	else if( !cls.glconfig.isFullscreen && loading )
	{
		// Loading in windowed mode
		IN_DeactivateMouse( cls.glconfig.isFullscreen );
	}
	else if( !( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_INPUT_FOCUS ) )
	{
		// Window not got focus
		IN_DeactivateMouse( cls.glconfig.isFullscreen );
	}
	else
		IN_ActivateMouse( cls.glconfig.isFullscreen );

	IN_ProcessEvents( );

	// Set event time for next frame to earliest possible time an event could happen
	in_eventTime = Sys_Milliseconds( );

	// In case we had to delay actual restart of video system
	if( ( vidRestartTime != 0 ) && ( vidRestartTime < Sys_Milliseconds( ) ) )
	{
		vidRestartTime = 0;
		Cbuf_AddText( "vid_restart\n" );
	}
}

/*
===============
IN_Init
===============
*/
void IN_Init( void *windowData )
{
	int appState;

	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		Com_Error( ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )" );
		return;
	}

	SDL_window = (SDL_Window *)windowData;

	Com_DPrintf( "\n------- Input Initialization -------\n" );

#ifdef IOS
	// CRITICAL: Disable SDL's joystick/gamecontroller subsystem entirely on iOS
	// This MUST happen before any other SDL calls
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "0");
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
	SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "1");
	SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG, "");
	SDL_SetHint("SDL_JOYSTICK_DISABLED", "1");
	
	// Quit any already initialized joystick subsystems
	if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
		SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
		Com_DPrintf("iOS: Quit SDL gamecontroller subsystem\n");
	}
	if (SDL_WasInit(SDL_INIT_JOYSTICK)) {
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		Com_DPrintf("iOS: Quit SDL joystick subsystem\n");
	}
	
	Com_DPrintf( "iOS: Disabled SDL controller hints and subsystems\n" );
#endif

	in_keyboardDebug = Cvar_Get( "in_keyboardDebug", "0", CVAR_ARCHIVE );

	// mouse variables
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_nograb = Cvar_Get( "in_nograb", "0", CVAR_ARCHIVE );

#ifdef USE_IOS_GAMECONTROLLER
	// Force joystick to be enabled on iOS
	in_joystick = Cvar_Get( "in_joystick", "1", CVAR_ARCHIVE|CVAR_LATCH );
	Cvar_Set( "in_joystick", "1" );
#else
	in_joystick = Cvar_Get( "in_joystick", "0", CVAR_ARCHIVE|CVAR_LATCH );
#endif
	in_joystickThreshold = Cvar_Get( "joy_threshold", "0.15", CVAR_ARCHIVE );

#ifndef IOS
	SDL_StartTextInput( );
#else
    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    //    SDL_StartTextInput();  // Enable text input on iOS too! - not working
#endif

	mouseAvailable = ( in_mouse->value != 0 );
	IN_DeactivateMouse( Cvar_VariableIntegerValue( "r_fullscreen" ) != 0 );

	appState = SDL_GetWindowFlags( SDL_window );
	Cvar_SetValue( "com_unfocused",	!( appState & SDL_WINDOW_INPUT_FOCUS ) );
	Cvar_SetValue( "com_minimized", appState & SDL_WINDOW_MINIMIZED );

	IN_InitJoystick( );
	Com_DPrintf( "------------------------------------\n" );
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown( void )
{
	SDL_StopTextInput( );

	IN_DeactivateMouse( Cvar_VariableIntegerValue( "r_fullscreen" ) != 0 );
	mouseAvailable = qfalse;

	IN_ShutdownJoystick( );

	SDL_window = NULL;
}

/*
===============
IN_Restart
===============
*/
void IN_Restart( void )
{
	IN_ShutdownJoystick( );
	IN_Init( SDL_window );
}
