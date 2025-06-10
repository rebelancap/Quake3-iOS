//
//  sys_ios.h
//  Quake3-iOS
//
//  Created by rebelancap on 6/6/25.
//  Copyright © 2025 Tom Kidd. All rights reserved.
//

#import "ios_gamecontroller_bridge.h"
#import "IOSGameController.h"

void iOS_InitGameController(void) {
    [[IOSGameController sharedController] startControllerDiscovery];
}

void iOS_SetSDLWindow(SDL_Window *window) {
    [[IOSGameController sharedController] setSDLWindow:window];
}

int iOS_IsControllerConnected(void) {
    return [[IOSGameController sharedController] isControllerConnected] ? 1 : 0;
}

float iOS_GetLeftStickX(void) {
    return [[IOSGameController sharedController] leftStickX];
}

float iOS_GetLeftStickY(void) {
    return [[IOSGameController sharedController] leftStickY];
}

float iOS_GetRightStickX(void) {
    return [[IOSGameController sharedController] rightStickX];
}

float iOS_GetRightStickY(void) {
    return [[IOSGameController sharedController] rightStickY];
}

float iOS_GetLeftTrigger(void) {
    return [[IOSGameController sharedController] leftTrigger];
}

float iOS_GetRightTrigger(void) {
    return [[IOSGameController sharedController] rightTrigger];
}

int iOS_GetButtonA(void) {
    return [[IOSGameController sharedController] buttonA] ? 1 : 0;
}

int iOS_GetButtonB(void) {
    return [[IOSGameController sharedController] buttonB] ? 1 : 0;
}

int iOS_GetButtonX(void) {
    return [[IOSGameController sharedController] buttonX] ? 1 : 0;
}

int iOS_GetButtonY(void) {
    return [[IOSGameController sharedController] buttonY] ? 1 : 0;
}

int iOS_GetLeftShoulder(void) {
    return [[IOSGameController sharedController] leftShoulder] ? 1 : 0;
}

int iOS_GetRightShoulder(void) {
    return [[IOSGameController sharedController] rightShoulder] ? 1 : 0;
}

int iOS_GetDpadUp(void) {
    return [[IOSGameController sharedController] dpadUp] ? 1 : 0;
}

int iOS_GetDpadDown(void) {
    return [[IOSGameController sharedController] dpadDown] ? 1 : 0;
}

int iOS_GetDpadLeft(void) {
    return [[IOSGameController sharedController] dpadLeft] ? 1 : 0;
}

int iOS_GetDpadRight(void) {
    return [[IOSGameController sharedController] dpadRight] ? 1 : 0;
}

int iOS_GetButtonMenu(void) {
    return [[IOSGameController sharedController] buttonMenu] ? 1 : 0;
}

int iOS_GetButtonOptions(void) {
    return [[IOSGameController sharedController] buttonOptions] ? 1 : 0;
}

int iOS_GetLeftThumbstickButton(void) {
    return [[IOSGameController sharedController] leftThumbstickButton] ? 1 : 0;
}

int iOS_GetRightThumbstickButton(void) {
    return [[IOSGameController sharedController] rightThumbstickButton] ? 1 : 0;
}

void iOS_TriggerHaptic(float intensity, int durationMs) {
    [[IOSGameController sharedController] triggerHaptic:intensity duration:durationMs];
}
