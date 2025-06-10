//
//  sys_ios.h
//  Quake3-iOS
//
//  Created by rebelancap on 6/6/25.
//  Copyright © 2025 Tom Kidd. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <GameController/GameController.h>
#include <SDL.h>

@interface IOSGameController : NSObject

+ (instancetype)sharedController;
- (void)startControllerDiscovery;
- (void)stopControllerDiscovery;
- (BOOL)isControllerConnected;

// Add SDL window reference
@property (nonatomic, assign) SDL_Window *sdlWindow;
- (void)setSDLWindow:(SDL_Window *)window;

// Controller state
@property (nonatomic, readonly) float leftStickX;
@property (nonatomic, readonly) float leftStickY;
@property (nonatomic, readonly) float rightStickX;
@property (nonatomic, readonly) float rightStickY;
@property (nonatomic, readonly) float leftTrigger;
@property (nonatomic, readonly) float rightTrigger;

// Button states
@property (nonatomic, readonly) BOOL buttonA;
@property (nonatomic, readonly) BOOL buttonB;
@property (nonatomic, readonly) BOOL buttonX;
@property (nonatomic, readonly) BOOL buttonY;
@property (nonatomic, readonly) BOOL leftShoulder;
@property (nonatomic, readonly) BOOL rightShoulder;
@property (nonatomic, readonly) BOOL dpadUp;
@property (nonatomic, readonly) BOOL dpadDown;
@property (nonatomic, readonly) BOOL dpadLeft;
@property (nonatomic, readonly) BOOL dpadRight;
@property (nonatomic, readonly) BOOL buttonMenu;
@property (nonatomic, readonly) BOOL buttonOptions;
@property (nonatomic, readonly) BOOL leftThumbstickButton;
@property (nonatomic, readonly) BOOL rightThumbstickButton;

// Haptic feedback
- (void)triggerHaptic:(float)intensity duration:(int)durationMs;

@end
