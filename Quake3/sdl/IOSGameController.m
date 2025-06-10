//
//  sys_ios.h
//  Quake3-iOS
//
//  Created by rebelancap on 6/6/25.
//  Copyright © 2025 Tom Kidd. All rights reserved.
//


#import "IOSGameController.h"
#import <CoreHaptics/CoreHaptics.h>
#include "sys_ios.h"  // Add this line

// Remove the forward declarations since they're now in the header
// void Sys_HideControls(SDL_Window *sdlWindow);
// void Sys_ShowControls(SDL_Window *sdlWindow);

@interface IOSGameController ()
@property (nonatomic, strong) GCController *activeController;
@property (nonatomic, strong) id connectObserver;
@property (nonatomic, strong) id disconnectObserver;
@property (nonatomic, strong) CHHapticEngine *hapticEngine API_AVAILABLE(ios(13.0));
@property (nonatomic, strong) id<CHHapticPatternPlayer> hapticPlayer API_AVAILABLE(ios(13.0));
@end

@implementation IOSGameController

+ (instancetype)sharedController {
    static IOSGameController *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[IOSGameController alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        [self setupControllerObservers];
        [self findController];
        [self startControllerDiscovery];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self.connectObserver];
    [[NSNotificationCenter defaultCenter] removeObserver:self.disconnectObserver];
    if (@available(iOS 13.0, *)) {
        [self.hapticEngine stopWithCompletionHandler:nil];
    }
}

- (void)setSDLWindow:(SDL_Window *)window {
    _sdlWindow = window;
}

- (void)setupControllerObservers {
    __weak typeof(self) weakSelf = self;
    
    self.connectObserver = [[NSNotificationCenter defaultCenter] 
        addObserverForName:GCControllerDidConnectNotification
        object:nil
        queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification *note) {
            GCController *controller = note.object;
            NSLog(@"iOS GameController: Controller connected: %@", controller.vendorName ?: @"Unknown");
            
            if (!weakSelf.activeController) {
                weakSelf.activeController = controller;
                [weakSelf setupHaptics];
                
                // Hide on-screen controls when controller connects
                if (weakSelf.sdlWindow) {
                    Sys_HideControls(weakSelf.sdlWindow);
                }
            }
        }];
    
    self.disconnectObserver = [[NSNotificationCenter defaultCenter]
        addObserverForName:GCControllerDidDisconnectNotification
        object:nil
        queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification *note) {
            GCController *controller = note.object;
            NSLog(@"iOS GameController: Controller disconnected: %@", controller.vendorName ?: @"Unknown");
            
            if (weakSelf.activeController == controller) {
                weakSelf.activeController = nil;
                
                if (@available(iOS 13.0, *)) {
                    if (weakSelf.hapticEngine) {
                        [weakSelf.hapticEngine stopWithCompletionHandler:nil];
                        weakSelf.hapticEngine = nil;
                    }
                }
                
                // Try to find another controller
                [weakSelf findController];
                
                // Show on-screen controls if no controller is connected
                if (!weakSelf.activeController && weakSelf.sdlWindow) {
                    Sys_ShowControls(weakSelf.sdlWindow);
                }
            }
        }];
}

- (void)findController {
    NSArray *controllers = [GCController controllers];
    
    if (!self.activeController && controllers.count > 0) {
        for (GCController *controller in controllers) {
            if (controller.extendedGamepad) {
                self.activeController = controller;
                NSLog(@"iOS GameController: Connected to %@", 
                      controller.vendorName ?: @"Unknown Controller");
                [self setupHaptics];
                
                // Hide on-screen controls when controller is found
                if (self.sdlWindow) {
                    Sys_HideControls(self.sdlWindow);
                }
                
                break;
            }
        }
    }
}

- (void)setupHaptics {
    if (@available(iOS 13.0, *)) {
        if (!self.activeController) {
            return;
        }
        
        if (self.activeController.haptics) {
            NSError *error = nil;
            self.hapticEngine = [self.activeController.haptics createEngineWithLocality:GCHapticsLocalityDefault];
            
            if (!error) {
                [self.hapticEngine startWithCompletionHandler:^(NSError *error) {
                    if (error) {
                        NSLog(@"Error starting haptic engine: %@", error);
                    }
                }];
            }
        }
    }
}

- (void)triggerHaptic:(float)intensity duration:(int)durationMs {
    if (@available(iOS 13.0, *)) {
        if (!self.hapticEngine || self.hapticEngine.currentTime < 0) {
            return;
        }
        
        intensity = MAX(0.0f, MIN(1.0f, intensity));
        
        CHHapticEventParameter *intensityParam = [[CHHapticEventParameter alloc] 
            initWithParameterID:CHHapticEventParameterIDHapticIntensity 
            value:intensity];
        
        CHHapticEventParameter *sharpnessParam = [[CHHapticEventParameter alloc] 
            initWithParameterID:CHHapticEventParameterIDHapticSharpness 
            value:0.5f];
        
        CHHapticEvent *event = [[CHHapticEvent alloc] 
            initWithEventType:CHHapticEventTypeHapticContinuous 
            parameters:@[intensityParam, sharpnessParam] 
            relativeTime:0 
            duration:durationMs / 1000.0];
        
        NSError *error = nil;
        CHHapticPattern *pattern = [[CHHapticPattern alloc] 
            initWithEvents:@[event] 
            parameters:@[] 
            error:&error];
        
        if (!error) {
            self.hapticPlayer = [self.hapticEngine createPlayerWithPattern:pattern error:&error];
            
            if (!error) {
                [self.hapticPlayer startAtTime:CHHapticTimeImmediate error:&error];
            }
        }
    }
}

- (BOOL)isControllerConnected {
    return self.activeController != nil;
}

- (void)startControllerDiscovery {
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        NSLog(@"iOS GameController: Wireless discovery completed");
        [self findController];
    }];
}

- (void)stopControllerDiscovery {
    [GCController stopWirelessControllerDiscovery];
}

// Analog stick getters
- (float)leftStickX {
    if (!self.activeController || !self.activeController.extendedGamepad) return 0.0f;
    return self.activeController.extendedGamepad.leftThumbstick.xAxis.value;
}

- (float)leftStickY {
    if (!self.activeController || !self.activeController.extendedGamepad) return 0.0f;
    return self.activeController.extendedGamepad.leftThumbstick.yAxis.value;
}

- (float)rightStickX {
    if (!self.activeController || !self.activeController.extendedGamepad) return 0.0f;
    return self.activeController.extendedGamepad.rightThumbstick.xAxis.value;
}

- (float)rightStickY {
    if (!self.activeController || !self.activeController.extendedGamepad) return 0.0f;
    return self.activeController.extendedGamepad.rightThumbstick.yAxis.value;
}

- (float)leftTrigger {
    if (!self.activeController || !self.activeController.extendedGamepad) return 0.0f;
    return self.activeController.extendedGamepad.leftTrigger.value;
}

- (float)rightTrigger {
    if (!self.activeController || !self.activeController.extendedGamepad) return 0.0f;
    return self.activeController.extendedGamepad.rightTrigger.value;
}

// Button getters
- (BOOL)buttonA {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.buttonA.pressed;
}

- (BOOL)buttonB {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.buttonB.pressed;
}

- (BOOL)buttonX {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.buttonX.pressed;
}

- (BOOL)buttonY {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.buttonY.pressed;
}

- (BOOL)leftShoulder {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.leftShoulder.pressed;
}

- (BOOL)rightShoulder {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.rightShoulder.pressed;
}

- (BOOL)dpadUp {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.dpad.up.pressed;
}

- (BOOL)dpadDown {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.dpad.down.pressed;
}

- (BOOL)dpadLeft {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.dpad.left.pressed;
}

- (BOOL)dpadRight {
    if (!self.activeController || !self.activeController.extendedGamepad) return NO;
    return self.activeController.extendedGamepad.dpad.right.pressed;
}

- (BOOL)buttonMenu {
    if (@available(iOS 13.0, tvOS 13.0, *)) {
        if (!self.activeController || !self.activeController.extendedGamepad) return NO;
        return self.activeController.extendedGamepad.buttonMenu.pressed;
    }
    return NO;
}

- (BOOL)buttonOptions {
    if (@available(iOS 13.0, tvOS 13.0, *)) {
        if (!self.activeController || !self.activeController.extendedGamepad) return NO;
        return self.activeController.extendedGamepad.buttonOptions.pressed;
    }
    return NO;
}

- (BOOL)leftThumbstickButton {
    if (@available(iOS 12.1, tvOS 12.1, *)) {
        if (!self.activeController || !self.activeController.extendedGamepad) return NO;
        return self.activeController.extendedGamepad.leftThumbstickButton.pressed;
    }
    return NO;
}

- (BOOL)rightThumbstickButton {
    if (@available(iOS 12.1, tvOS 12.1, *)) {
        if (!self.activeController || !self.activeController.extendedGamepad) return NO;
        return self.activeController.extendedGamepad.rightThumbstickButton.pressed;
    }
    return NO;
}

@end
