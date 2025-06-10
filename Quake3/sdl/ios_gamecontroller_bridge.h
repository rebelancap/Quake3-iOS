//
//  sys_ios.h
//  Quake3-iOS
//
//  Created by rebelancap on 6/6/25.
//  Copyright © 2025 Tom Kidd. All rights reserved.
//

#ifndef IOS_GAMECONTROLLER_BRIDGE_H
#define IOS_GAMECONTROLLER_BRIDGE_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the controller system
void iOS_InitGameController(void);

// Set SDL window reference
void iOS_SetSDLWindow(SDL_Window *window);

// Check if a controller is connected
int iOS_IsControllerConnected(void);

// Get analog stick values (-1.0 to 1.0)
float iOS_GetLeftStickX(void);
float iOS_GetLeftStickY(void);
float iOS_GetRightStickX(void);
float iOS_GetRightStickY(void);

// Get trigger values (0.0 to 1.0)
float iOS_GetLeftTrigger(void);
float iOS_GetRightTrigger(void);

// Get button states (0 or 1)
int iOS_GetButtonA(void);
int iOS_GetButtonB(void);
int iOS_GetButtonX(void);
int iOS_GetButtonY(void);
int iOS_GetLeftShoulder(void);
int iOS_GetRightShoulder(void);
int iOS_GetDpadUp(void);
int iOS_GetDpadDown(void);
int iOS_GetDpadLeft(void);
int iOS_GetDpadRight(void);
int iOS_GetButtonMenu(void);
int iOS_GetButtonOptions(void);
int iOS_GetLeftThumbstickButton(void);
int iOS_GetRightThumbstickButton(void);

// Haptic feedback
void iOS_TriggerHaptic(float intensity, int durationMs);

#ifdef __cplusplus
}
#endif

#endif
