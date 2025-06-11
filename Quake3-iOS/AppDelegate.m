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
@property (nonatomic, strong) NSString *launchMode;
@property (nonatomic, strong) NSString *launchMod;
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

// Add these methods to handle URL launching
- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey,id> *)options {
    return [self handleQuakeURL:url];
}

// For iOS 8 compatibility
- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    return [self handleQuakeURL:url];
}

- (BOOL)handleQuakeURL:(NSURL *)url {
    if ([url.scheme isEqualToString:@"quake3"]) {
        NSString *host = url.host;
        
        // Parse URL parameters
        NSURLComponents *components = [NSURLComponents componentsWithURL:url resolvingAgainstBaseURL:NO];
        NSString *mod = nil;
        NSString *map = nil;
        
        for (NSURLQueryItem *item in components.queryItems) {
            if ([item.name isEqualToString:@"mod"]) {
                mod = item.value;
            } else if ([item.name isEqualToString:@"map"]) {
                map = item.value;
            }
        }
        
        // Store map if provided
        if (map) {
            [[NSUserDefaults standardUserDefaults] setObject:map forKey:@"selectedMap"];
        }
        
        if ([host isEqualToString:@"vanilla"]) {
            self.launchMode = @"vanilla";
            self.launchMod = nil;
            [self launchVanillaQuake3];
            return YES;
        } else if ([host isEqualToString:@"mod"] && mod) {
            self.launchMode = @"mod";
            self.launchMod = mod;
            [self launchQuake3WithMod:mod];
            return YES;
        }
    }
    return NO;
}

- (void)launchVanillaQuake3 {
    // Store launch preference
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"launchVanillaOnStart"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    // Post notification that MainMenuViewController can listen for
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LaunchVanillaQuake3" object:nil];
    
    // If the app is already running, we need to navigate to the game
    if (self.rootNavigationController && self.rootNavigationController.topViewController) {
        dispatch_async(dispatch_get_main_queue(), ^{
            // Pop to root first
            [self.rootNavigationController popToRootViewControllerAnimated:NO];
            
            // Get the main menu view controller
            UIViewController *rootVC = self.rootNavigationController.topViewController;
            
            // Try to instantiate GameViewController directly
            UIStoryboard *mainStoryboard = [UIStoryboard storyboardWithName:@"Main" bundle:nil];
            UIViewController *gameVC = [mainStoryboard instantiateViewControllerWithIdentifier:@"GameViewController"];
            if (gameVC) {
                gameVC.modalPresentationStyle = UIModalPresentationFullScreen;
                [rootVC presentViewController:gameVC animated:NO completion:nil];
            }
        });
    }
}

- (void)launchQuake3WithMod:(NSString *)modName {
    // Store the mod name for GameViewController to use - USE CONSISTENT KEY
    [[NSUserDefaults standardUserDefaults] setObject:modName forKey:@"launchMod"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    // Store launch preference for cold start
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"launchVanillaOnStart"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    // Post notification
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LaunchQuake3Mod"
                                                        object:nil
                                                      userInfo:@{@"mod": modName}];
    
    // Navigate to game
    if (self.rootNavigationController && self.rootNavigationController.topViewController) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.rootNavigationController popToRootViewControllerAnimated:NO];
            
            UIViewController *rootVC = self.rootNavigationController.topViewController;
            UIStoryboard *mainStoryboard = [UIStoryboard storyboardWithName:@"Main" bundle:nil];
            UIViewController *gameVC = [mainStoryboard instantiateViewControllerWithIdentifier:@"GameViewController"];
            if (gameVC) {
                gameVC.modalPresentationStyle = UIModalPresentationFullScreen;
                [rootVC presentViewController:gameVC animated:NO completion:nil];
            }
        });
    }
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
