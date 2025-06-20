//
//  sys_ios.m
//  Quake3-iOS
//
//  Created by Tom Kidd on 11/11/19.
//  Copyright © 2019 Tom Kidd. All rights reserved.
//  Some portions originally Seth Kingsley, January 2008.

#import <Foundation/Foundation.h>
#include "sys_local.h"
#include "qcommon.h"
#include "sys_ios.h"  // Add this line

#ifdef USE_IOS_GAMECONTROLLER
#include "ios_gamecontroller_bridge.h"
#endif

#if TARGET_OS_TV
#import "Quake3_tvOS-Swift.h"
#else
#import "Quake3_iOS-Swift.h"
#endif

#include <SDL_syswm.h>


qboolean Sys_LowPhysicalMemory(void) {
    return qtrue;
}

void Sys_UnloadGame() {
}

void Sys_Error(const char *error, ...) {
    extern void Sys_Exit(int ex);
    
    NSString *errorString;
    va_list ap;
    
    va_start(ap, error);
    errorString = [[NSString alloc] initWithFormat:[NSString stringWithCString:error encoding:NSUTF8StringEncoding]
                                          arguments:ap];
    va_end(ap);

    Sys_UnloadGame();
    
    exit(1);
}

void Sys_Warn(const char *warning, ...) {
    NSString *warningString;
    va_list ap;
    
    va_start(ap, warning);
    warningString = [[NSString alloc] initWithFormat:[NSString stringWithCString:warning encoding:NSUTF8StringEncoding]
                                            arguments:ap];
    va_end(ap);
}

UIViewController* GetSDLViewController(SDL_Window *sdlWindow) {
    SDL_SysWMinfo systemWindowInfo;
    SDL_VERSION(&systemWindowInfo.version);
    if ( ! SDL_GetWindowWMInfo(sdlWindow, &systemWindowInfo)) {
        // error handle?
        return nil;
    }
    UIWindow *appWindow = systemWindowInfo.info.uikit.window;
    UIViewController *rootVC = appWindow.rootViewController;
    return rootVC;
}

void Sys_AddControls(SDL_Window *sdlWindow) {
    #if !TARGET_OS_TV
        #ifdef USE_IOS_GAMECONTROLLER
        // Don't add controls if a controller is connected
        if (iOS_IsControllerConnected()) {
            return;
        }
        #endif
        
        // adding on-screen controls -tkidd
        SDL_uikitviewcontroller *rootVC = (SDL_uikitviewcontroller *)GetSDLViewController(sdlWindow);

        // Fire button - moved more to the left to make room for right joystick
        UIView *fireButton = [rootVC fireButtonWithRect:[rootVC.view frame]];
        // Adjust the fire button position here if needed
        [rootVC.view addSubview:fireButton];
        
        // Jump button - commented out
        // [rootVC.view addSubview:[rootVC jumpButtonWithRect:[rootVC.view frame]]];
        
        // Left joystick for movement
        [rootVC.view addSubview:[rootVC joyStickWithRect:[rootVC.view frame]]];
        
        // Right joystick for aiming - new addition
        UIView *rightJoyStick = [rootVC rightJoyStickWithRect:[rootVC.view frame]];
        [rootVC.view addSubview:rightJoyStick];
        
        [rootVC.view addSubview:[rootVC buttonStackWithRect:[rootVC.view frame]]];
        [rootVC.view addSubview:[rootVC f1ButtonWithRect:[rootVC.view frame]]];
        [rootVC.view addSubview:[rootVC prevWeaponButtonWithRect:[rootVC.view frame]]];
        [rootVC.view addSubview:[rootVC nextWeaponButtonWithRect:[rootVC.view frame]]];
        [rootVC.view addSubview:[rootVC quitButtonWithRect:[rootVC.view frame]]];
    #endif
}

void Sys_ToggleControls(SDL_Window *sdlWindow) {
    #if !TARGET_OS_TV
        #ifdef USE_IOS_GAMECONTROLLER
        // Don't show controls if a controller is connected
        if (iOS_IsControllerConnected()) {
            return;
        }
        #endif
        
        SDL_uikitviewcontroller *rootVC = (SDL_uikitviewcontroller *)GetSDLViewController(sdlWindow);
        [rootVC toggleControls:Key_GetCatcher( ) & KEYCATCH_UI];
    #endif
}

void Sys_HideControls(SDL_Window *sdlWindow) {
    #if !TARGET_OS_TV
        SDL_uikitviewcontroller *rootVC = (SDL_uikitviewcontroller *)GetSDLViewController(sdlWindow);
        if (!rootVC) return;
        
        // Hide all control subviews
        for (UIView *subview in [rootVC.view.subviews copy]) {
            if ([subview isKindOfClass:[JoyStickView class]] ||
                [subview isKindOfClass:[UIButton class]]) {
                subview.hidden = YES;
                [subview removeFromSuperview];
            }
        }
    #endif
}

void Sys_ShowControls(SDL_Window *sdlWindow) {
    #if !TARGET_OS_TV
        #ifdef USE_IOS_GAMECONTROLLER
        // Don't show if controller is connected
        if (iOS_IsControllerConnected()) {
            NSLog(@"Sys_ShowControls: Controller connected, not showing controls");
            return;
        }
        #endif
        
        NSLog(@"Sys_ShowControls called");
        SDL_uikitviewcontroller *rootVC = (SDL_uikitviewcontroller *)GetSDLViewController(sdlWindow);
        if (!rootVC) {
            NSLog(@"Sys_ShowControls: rootVC is NULL");
            return;
        }
        
        // Re-add controls
        dispatch_async(dispatch_get_main_queue(), ^{
            // Remove any existing controls first
            for (UIView *subview in [rootVC.view.subviews copy]) {
                if ([subview isKindOfClass:[JoyStickView class]] ||
                    [subview isKindOfClass:[UIButton class]]) {
                    [subview removeFromSuperview];
                }
            }
            
            // Add controls back
            UIView *fireButton = [rootVC fireButtonWithRect:[rootVC.view frame]];
            [rootVC.view addSubview:fireButton];
            
            // Jump button - commented out
            // [rootVC.view addSubview:[rootVC jumpButtonWithRect:[rootVC.view frame]]];
            
            // Left joystick for movement
            [rootVC.view addSubview:[rootVC joyStickWithRect:[rootVC.view frame]]];
            
            // Right joystick for aiming
            UIView *rightJoyStick = [rootVC rightJoyStickWithRect:[rootVC.view frame]];
            [rootVC.view addSubview:rightJoyStick];
            
            [rootVC.view addSubview:[rootVC buttonStackWithRect:[rootVC.view frame]]];
            [rootVC.view addSubview:[rootVC f1ButtonWithRect:[rootVC.view frame]]];
            [rootVC.view addSubview:[rootVC prevWeaponButtonWithRect:[rootVC.view frame]]];
            [rootVC.view addSubview:[rootVC nextWeaponButtonWithRect:[rootVC.view frame]]];
            [rootVC.view addSubview:[rootVC quitButtonWithRect:[rootVC.view frame]]];
            
            [rootVC.view setNeedsLayout];
            [rootVC.view layoutIfNeeded];
        });
    #endif
}

void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
    // unused in iOS
}

/*
 =================
 Sys_StripAppBundle
 
 Discovers if passed dir is suffixed with the directory structure of an iOS
 .app bundle. If it is, the .app directory structure is stripped off the end and
 the result is returned. If not, dir is returned untouched.
 =================
 */
char *Sys_StripAppBundle( char *dir )
{
    static char cwd[MAX_OSPATH];
    
    Q_strncpyz(cwd, dir, sizeof(cwd));
    if(!strstr(Sys_Basename(cwd), ".app"))
        return dir;
    Q_strncpyz(cwd, Sys_Dirname(cwd), sizeof(cwd));
    return cwd;
}

/*
 ==============
 Sys_Dialog
 ==============
 */
dialogResult_t Sys_Dialog( dialogType_t type, const char *message, const char *title ) { return 0; }
