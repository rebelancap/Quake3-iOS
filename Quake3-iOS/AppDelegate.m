//
//  AppDelegate.m
//  Quake2-iOS
//
//  Created by Tom Kidd on 1/26/19.
//

#import "AppDelegate.h"
#import <GameController/GameController.h>
#if TARGET_OS_TV
#import "Quake3_tvOS-Swift.h"
#else
#import "Quake3_iOS-Swift.h"
#endif

@implementation SDLUIKitDelegate (customDelegate)

// hijack the the SDL_UIKitAppDelegate to use the UIApplicationDelegate we implement here
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-protocol-method-implementation"
+ (NSString *)getAppDelegateClassName {
    return @"AppDelegate";
}
#pragma clang diagnostic pop

@end

@interface AppDelegate ()
@end

@implementation AppDelegate
@synthesize rootNavigationController, uiwindow;

#pragma mark -
#pragma mark AppDelegate methods
- (id)init {
    if ((self = [super init])) {
        rootNavigationController = nil;
        uiwindow = nil;
    }
    return self;
}

- (BOOL)prefersStatusBarHidden {
   return NO;
}

// override the direct execution of SDL_main to allow us to implement our own frontend
- (void)postFinishLaunch
{
    [self performSelector:@selector(hideLaunchScreen) withObject:nil afterDelay:0.0];

    self.uiwindow = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    self.uiwindow.backgroundColor = [UIColor blackColor];
    
    UIStoryboard *mainStoryboard = [UIStoryboard storyboardWithName:@"Main" bundle: nil];

    rootNavigationController = (UINavigationController *)[mainStoryboard instantiateViewControllerWithIdentifier:@"RootNC"];

    self.uiwindow.rootViewController = self.rootNavigationController;
    
    [self.uiwindow makeKeyAndVisible];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
    [super applicationDidReceiveMemoryWarning:application];
}

// true multitasking with SDL works only on 4.2 and above; we close the game to avoid a black screen at return
- (void)applicationWillResignActive:(UIApplication *)application {
    [super applicationWillResignActive:application];
    NSLog(@"App going to background - pausing game rendering");
    
    // Send notification to pause rendering
    [[NSNotificationCenter defaultCenter] postNotificationName:@"PauseGameRendering" object:nil];
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    [super applicationDidEnterBackground:application];
    NSLog(@"App entered background");
    
    // Send notification to pause rendering (backup)
    [[NSNotificationCenter defaultCenter] postNotificationName:@"PauseGameRendering" object:nil];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Initialize GameController framework EARLY
    NSLog(@"AppDelegate: Initializing GameController framework early...");
    
    // Start controller discovery immediately
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        NSLog(@"AppDelegate: Controller discovery completed");
        NSArray *controllers = [GCController controllers];
        NSLog(@"AppDelegate: Found %lu controllers", (unsigned long)controllers.count);
        for (GCController *controller in controllers) {
            NSLog(@"AppDelegate: - %@", controller.vendorName);
        }
    }];
    
    // Check for already connected controllers
    NSArray *controllers = [GCController controllers];
    NSLog(@"AppDelegate: Initial controller check: %lu controllers", (unsigned long)controllers.count);
    
    // Set up controller connect/disconnect notifications
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidConnect:)
                                                 name:GCControllerDidConnectNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDidDisconnect:)
                                                 name:GCControllerDidDisconnectNotification
                                               object:nil];
    // Set up game background/foreground notifications
    [self setupGameNotifications];
    
    return [super application:application didFinishLaunchingWithOptions:launchOptions];
}

- (void)controllerDidConnect:(NSNotification *)notification {
    GCController *controller = notification.object;
    NSLog(@"AppDelegate: Controller connected: %@", controller.vendorName);
}

- (void)controllerDidDisconnect:(NSNotification *)notification {
    GCController *controller = notification.object;
    NSLog(@"AppDelegate: Controller disconnected: %@", controller.vendorName);
}

- (void)applicationDidFinishLaunching:(UIApplication *)application {
    [super applicationDidFinishLaunching:application];
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    [super applicationDidBecomeActive:application];
    NSLog(@"App became active - resuming game rendering");
    
    // Send notification to resume rendering
    [[NSNotificationCenter defaultCenter] postNotificationName:@"ResumeGameRendering" object:nil];
}

- (void)setupGameNotifications {
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePauseRendering:)
                                                 name:@"PauseGameRendering"
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResumeRendering:)
                                                 name:@"ResumeGameRendering"
                                               object:nil];
}

- (void)handlePauseRendering:(NSNotification *)notification {
    NSLog(@"AppDelegate: Handling pause rendering notification");
    
    // Declare the C function here
    extern void CL_PauseRendering(void);
    CL_PauseRendering();
}

- (void)handleResumeRendering:(NSNotification *)notification {
    NSLog(@"AppDelegate: Handling resume rendering notification");
    
    // Declare the C function here
    extern void CL_ResumeRendering(void);
    CL_ResumeRendering();
}

// dummy function to prevent linkage fail
int SDL_main(int argc, char **argv) {
    return 0;
}

@end
