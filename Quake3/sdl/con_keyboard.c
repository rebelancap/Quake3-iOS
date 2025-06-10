//
//  con_keyboard.c
//  Quake3-iOS
//
//  Created by rebelancap on 6/9/25.
//  Copyright © 2025 Tom Kidd. All rights reserved.
//

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../client/client.h"
#include "con_keyboard.h"

static qboolean vk_active = qfalse;
static int vk_cursorX = 0;
static int vk_cursorY = 0;
static qboolean vk_shifted = qfalse;

// Virtual keyboard layout - grid format for easier navigation
// Normal (unshifted) layout
static const char *vk_layout_normal[VK_ROWS][VK_COLS] = {
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="},
    {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]"},
    {"a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "\\"},
    {"z", "x", "c", "v", "b", "n", "m", "UP", ",", ".", "/", "DEL"},
    {"SHIFT", "SPACE", "SPACE", "SPACE", "SPACE", "SPACE", "SPACE", "LEFT", "DOWN", "RIGHT", "ENTER", "ENTER"}
};

// Shifted layout
static const char *vk_layout_shifted[VK_ROWS][VK_COLS] = {
    {"!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+"},
    {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}"},
    {"A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "|"},
    {"Z", "X", "C", "V", "B", "N", "M", "UP", "<", ">", "?", "DEL"},
    {"SHIFT", "SPACE", "SPACE", "SPACE", "SPACE", "SPACE", "SPACE", "LEFT", "DOWN", "RIGHT", "ENTER", "ENTER"}
};

// Update key widths - all 1 except special keys
const int vk_keyWidths[VK_ROWS][VK_COLS] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},     // Number row
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},     // QWERTY row
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},     // ASDF row
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},     // ZXCV row + arrows
    {2, 5, 0, 0, 0, 0, 0, 1, 1, 1, 2, 0}      // Bottom row - spacebar spans 6, arrows are 1 each, return spans 2
};

static const char *Con_GetKeyAtPosition(int row, int col) {
    if (vk_shifted) {
        return vk_layout_shifted[row][col];
    } else {
        return vk_layout_normal[row][col];
    }
}

void Con_ToggleVirtualKeyboard(void) {
    vk_active = !vk_active;
    if (vk_active) {
        vk_cursorX = 0;
        vk_cursorY = 0;
        vk_shifted = qfalse;  // reset shift when opening
        
        // Make sure console is open
        if (!(Key_GetCatcher() & KEYCATCH_CONSOLE)) {
            // Send console key to open it
            Com_QueueEvent(0, SE_KEY, K_CONSOLE, qtrue, 0, NULL);
            Com_QueueEvent(0, SE_KEY, K_CONSOLE, qfalse, 0, NULL);
        }
    } else {
        // Close console too
        if (Key_GetCatcher() & KEYCATCH_CONSOLE) {
            Com_QueueEvent(0, SE_KEY, K_CONSOLE, qtrue, 0, NULL);
            Com_QueueEvent(0, SE_KEY, K_CONSOLE, qfalse, 0, NULL);
        }
    }
}

void Con_VirtualKeyboardMove(int dx, int dy) {
    if (!vk_active) return;
    
    vk_cursorX += dx;
    vk_cursorY += dy;
    
    // Wrap around vertically
    if (vk_cursorY < 0) vk_cursorY = VK_ROWS - 1;
    if (vk_cursorY >= VK_ROWS) vk_cursorY = 0;
    
    // Wrap around horizontally
    if (vk_cursorX < 0) vk_cursorX = VK_COLS - 1;
    if (vk_cursorX >= VK_COLS) vk_cursorX = 0;
    
    // Handle keys that span multiple columns
    // If we land on a 0-width key, find the start of the multi-column key
    while (vk_keyWidths[vk_cursorY][vk_cursorX] == 0 && vk_cursorX > 0) {
        vk_cursorX--;
    }
    
    // Make sure we're on a valid key
    if (vk_keyWidths[vk_cursorY][vk_cursorX] == 0) {
        // If still invalid, move to first valid key in row
        for (vk_cursorX = 0; vk_cursorX < VK_COLS; vk_cursorX++) {
            if (vk_keyWidths[vk_cursorY][vk_cursorX] > 0) break;
        }
    }
}

void Con_VirtualKeyboardSelect(void) {
    if (!vk_active) return;
    
    const char *key = Con_GetKeyAtPosition(vk_cursorY, vk_cursorX);
    
    // Handle SHIFT toggle
    if (!strcmp(key, "SHIFT")) {
        vk_shifted = !vk_shifted;
        return;  // Don't process further and don't reset shift
    }
    
    // Handle all instances of SPACE
    if (!strcmp(key, "SPACE")) {
        Com_QueueEvent(0, SE_CHAR, ' ', 0, 0, NULL);
    }
    else if (!strcmp(key, "ENTER")) {
        Com_QueueEvent(0, SE_KEY, K_ENTER, qtrue, 0, NULL);
        Com_QueueEvent(0, SE_KEY, K_ENTER, qfalse, 0, NULL);
    }
    else if (!strcmp(key, "DEL") || !strcmp(key, "DELETE")) {
        // Make sure we're sending backspace events
        Com_QueueEvent(0, SE_CHAR, 8, 0, 0, NULL);  // ASCII backspace
    }
    else if (!strcmp(key, "CLEAR")) {
        // Clear line
        int i;
        for (i = 0; i < 50; i++) {
            Com_QueueEvent(0, SE_CHAR, 8, 0, 0, NULL);  // ASCII backspace
        }
    }
    else if (!strcmp(key, "UP")) {
        Com_QueueEvent(0, SE_KEY, K_UPARROW, qtrue, 0, NULL);
        Com_QueueEvent(0, SE_KEY, K_UPARROW, qfalse, 0, NULL);
    }
    else if (!strcmp(key, "DOWN")) {
        Com_QueueEvent(0, SE_KEY, K_DOWNARROW, qtrue, 0, NULL);
        Com_QueueEvent(0, SE_KEY, K_DOWNARROW, qfalse, 0, NULL);
    }
    else if (!strcmp(key, "LEFT")) {
        Com_QueueEvent(0, SE_KEY, K_LEFTARROW, qtrue, 0, NULL);
        Com_QueueEvent(0, SE_KEY, K_LEFTARROW, qfalse, 0, NULL);
    }
    else if (!strcmp(key, "RIGHT")) {
        Com_QueueEvent(0, SE_KEY, K_RIGHTARROW, qtrue, 0, NULL);
        Com_QueueEvent(0, SE_KEY, K_RIGHTARROW, qfalse, 0, NULL);
    }
    else if (key[0] && strlen(key) == 1) {
        // Single character - including v
        Com_QueueEvent(0, SE_CHAR, key[0], 0, 0, NULL);
    }
    
    // Reset shift after pressing ANY key except SHIFT
    // (SHIFT already returned above, so this won't execute for SHIFT)
    if (vk_shifted) {
        vk_shifted = qfalse;
    }
}

void Con_VirtualKeyboardBackspace(void) {
    if (!vk_active) return;
    
    // Send backspace key event
    Com_QueueEvent(0, SE_KEY, K_BACKSPACE, qtrue, 0, NULL);
    Com_QueueEvent(0, SE_KEY, K_BACKSPACE, qfalse, 0, NULL);
    
    // Also try sending it as a character event
    Com_QueueEvent(0, SE_CHAR, 8, 0, 0, NULL);  // ASCII backspace
}

qboolean Con_VirtualKeyboardActive(void) {
    return vk_active;
}

void Con_VirtualKeyboardSetShift(qboolean shift) {
    vk_shifted = shift;
}

qboolean Con_VirtualKeyboardIsShifted(void) {
    return vk_shifted;
}

// For external access to draw the keyboard
void Con_GetVirtualKeyboardInfo(int *cursorX, int *cursorY, const char ***layout) {
    if (cursorX) *cursorX = vk_cursorX;
    if (cursorY) *cursorY = vk_cursorY;
    if (layout) {
        if (vk_shifted) {
            *layout = (const char **)vk_layout_shifted;
        } else {
            *layout = (const char **)vk_layout_normal;
        }
    }
}

// add touch input to virtual keyboard
void Con_VirtualKeyboardTouch(float x, float y, float consoleHeight) {
    if (!vk_active) return;
    
    // These calculations MUST match Con_DrawVirtualKeyboard exactly
    int availableHeight = cls.glconfig.vidHeight - consoleHeight - 20;
    int availableWidth = cls.glconfig.vidWidth - 40;
    
    int keyHeight = (availableHeight - (VK_ROWS - 1) * VK_KEY_SPACING) / VK_ROWS;
    int baseKeyWidth = availableWidth / 14;
    
    // Ensure minimum sizes (match drawing code exactly)
    if (keyHeight < 40) keyHeight = 40;
    if (baseKeyWidth < 50) baseKeyWidth = 50;
    
    int startY = consoleHeight + 10;

    // Check which row was touched
    int row = (int)((y - startY) / (keyHeight + VK_KEY_SPACING));
    if (row < 0 || row >= VK_ROWS) return;

    // ADD THIS: Find the maximum row width across all rows
    int maxRowWidth = 0;
    int r;
    for (r = 0; r < VK_ROWS; r++) {
        int tempRowWidth = 0;
        int tempKeyCount = 0;
        int c;
        for (c = 0; c < VK_COLS; c++) {
            if (vk_keyWidths[r][c] > 0) {
                tempRowWidth += (baseKeyWidth * vk_keyWidths[r][c]) + VK_KEY_SPACING;
                tempKeyCount++;
            }
        }
        if (tempKeyCount > 0) {
            tempRowWidth -= VK_KEY_SPACING;
        }
        if (tempRowWidth > maxRowWidth) {
            maxRowWidth = tempRowWidth;
        }
    }

    // NOW use maxRowWidth for this specific row's calculation
    int startX = (cls.glconfig.vidWidth - maxRowWidth) / 2;
    int currentX = startX;

    // Find which key was touched
    int c;  // ADD THIS LINE - declare c here
    for (c = 0; c < VK_COLS; c++) {
        const char *key = Con_GetKeyAtPosition(row, c);
        int keyWidth = vk_keyWidths[row][c];
        
        if (!key || !key[0] || keyWidth == 0) continue;
        
        int thisKeyWidth = (baseKeyWidth * keyWidth) + (VK_KEY_SPACING * (keyWidth - 1));
        
        if (x >= currentX && x < currentX + thisKeyWidth) {
            // Found the key
            vk_cursorX = c;
            vk_cursorY = row;
            Con_VirtualKeyboardSelect();
            return;
        }
        
        currentX += thisKeyWidth + VK_KEY_SPACING;
    }
}
