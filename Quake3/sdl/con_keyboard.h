//
//  con_keyboard.h
//  Quake3-iOS
//
//  Created by rebelancap on 6/9/25.
//  Copyright © 2025 Tom Kidd. All rights reserved.
//

#ifndef CON_KEYBOARD_H
#define CON_KEYBOARD_H

// Virtual keyboard dimensions - must match between files
#define VK_START_X 100  // More centered
#define VK_START_Y_OFFSET 350  // Higher up from bottom
#define VK_KEY_WIDTH 60  // Wider keys
#define VK_KEY_HEIGHT 50  // Taller keys
#define VK_KEY_SPACING 2  // Less spacing
#define VK_PADDING 10  // Border padding

#define VK_ROWS 5
#define VK_COLS 12

// Key width array
extern const int vk_keyWidths[VK_ROWS][VK_COLS];

// Functions
void Con_ToggleVirtualKeyboard(void);
void Con_VirtualKeyboardMove(int dx, int dy);
void Con_VirtualKeyboardSelect(void);
void Con_VirtualKeyboardBackspace(void);
qboolean Con_VirtualKeyboardActive(void);
void Con_GetVirtualKeyboardInfo(int *cursorX, int *cursorY, const char ***layout);
void Con_VirtualKeyboardTouch(float x, float y, float consoleHeight);
void Con_VirtualKeyboardSetShift(qboolean shift);
qboolean Con_VirtualKeyboardIsShifted(void);

#endif
