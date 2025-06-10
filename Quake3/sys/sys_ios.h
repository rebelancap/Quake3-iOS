//
//  sys_ios.h
//  Quake3-iOS
//
//  Created by rebelancap on 6/8/25.
//  Copyright © 2025 Tom Kidd. All rights reserved.
//

#ifndef SYS_IOS_H
#define SYS_IOS_H

#include <SDL.h>

// Function declarations for iOS-specific system functions
void Sys_AddControls(SDL_Window *sdlWindow);
void Sys_ToggleControls(SDL_Window *sdlWindow);
void Sys_HideControls(SDL_Window *sdlWindow);
void Sys_ShowControls(SDL_Window *sdlWindow);

#endif // SYS_IOS_H
