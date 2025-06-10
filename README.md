<img align="left" width="100" height="100" src="https://raw.githubusercontent.com/tomkidd/Quake3-iOS/master/icon_quake3.png">  

#  Quake III: Arena for iOS and tvOS for Apple TV

&nbsp;

Background: I could not get @tomkidd's port to work with any of my controllers: Backbone Pro, Gamesir G8, any bluetooth controllers, etc. I know very little about coding, so Claude is to thank for most of this. Slowly vibe coded in MFi Controller support, mod support, file management support (for `.cfg` files, mods, etc), background handling (stop rendering and flush for more stability/battery life), virtual keyboard & mouse support, hiding on-screen controls when controller is connected, etc. More ideas of mine to come, like iOS shortcuts to launch mods directly, maybe a virtual right joystick for those without controllers, etc.

# Building Quake3-iOS

## Prerequisites
- Xcode (latest version recommended)
- iOS Developer Account (free or paid)
- Quake 3 Arena game files (baseq3 folder)

## Step-by-Step Instructions

**1. Clone the Project**
- Open Xcode
- File → Clone Repository
- Enter: `https://github.com/rebelancap/Quake3-iOS`
- Choose a location and clone

**2. Add Game Files**
- In Xcode, select the **Quake3-iOS** target
- Go to **Build Phases** tab
- Expand **Copy Bundle Resources**
- Click the **+** button at the bottom
- Select **Add Other...**
- Browse to your `baseq3` folder (contains game files like pak0.pk3)
- Select the folder and click **Add**
- Choose **Create folder references** (important!)
- Click **Finish**

**3. Configure Signing**
- Select the **Quake3-iOS** target
- Go to **Signing & Capabilities** tab
- Change **Team** to your Apple Developer account
- Change **Bundle Identifier** to something unique (e.g., `com.yourname.quake3ios`)

**4. Build and Run**
- Connect your iOS device
- Select your device from the scheme dropdown (top left)
- Click the **Play** button or press **Cmd+R**

## Notes
- You'll need legitimate Quake 3 Arena game files
- First build may take several minutes
- Make sure your device is trusted for development (Settings → General → VPN & Device Management)

## Important Info

Set up your `.cfg` files (containing your settings and bindings) and place in baseq3 or other mod folders. [Here are my examples](https://github.com/rebelancap/Quake3-iOS/wiki/Configuration-Files-Examples) for baseq3 and q3ut4 (Urban Terror).

Use macOS Finder or Files app to transfer files. Copy mod folders to Quake III Arena folder. Put `autoexec.cfg` in the mod folder.

Tap the π symbol in the bottom right corner to launch full version of Quake 3. (A slower way is use the custom menu to create any game, then press START, Exit Arena.) 

You can navigate the Q3 menus by using DPAD, A (Enter), B or START (Esc). Or use left joystick to move mouse, Right Trigger to left mouse click. These would have to be changed in Xcode, but you can generally change any in-game bindings/preferences with the `.cfg` files. 
Press both thumbsticks down to triggle console + virtual keyboard.

**Note!** If your controller gets disconnected, you need to restart the game to reconnect the controller. Had to disable SDL controller and enable Native GameController Framework to get MFi controllers to work. Claude and I could not figure out a way to solve this.

---

Original README:

This is my port of Quake III: Arena for iOS, running in modern resolutions including the full width of the iPhone X. I have also made a target and version for tvOS to run on Apple TV.

![screenshot](https://raw.githubusercontent.com/tomkidd/Quake3-iOS/master/ss_quake3.png)

Features

- Tested and builds without warnings on Xcode 9.4.1.
- Runs single player campaigns at full screen and full speed on iOS
- Multiplayer support, including server browser
- MFi controller support (reccomended) and on-screen control options
- Second project target for tvOS that takes advantage of focus model and removes on-screen controls.
- Limited support for native menus of original game

This commit does not need any placeholder resources as it is not an update of an existing id Software port. 

You will need to provide your own copies of the `baseq3` directory from an existing instalation of Quake III: Arena. You can grab the whole thing with expansions on [GOG](https://www.gog.com/game/quake_iii_gold) or [Steam](https://store.steampowered.com/app/2200/Quake_III_Arena/).

You will need to drag your directories into the project and select "Create Folder References" and add them to both the Quake3-iOS and Quake-tvOS targets. The folders will be blue if you've done it right:

![folders](https://raw.githubusercontent.com/tomkidd/Quake3-iOS/master/folders.png)

You can read a lengthy blog article on how I did all this [here](http://schnapple.com/quake-3-for-ios-and-tvos-for-apple-tv/).

This repo was based on the code from *[Beben III](https://itunes.apple.com/us/app/beben-iii/id771105890?mt=8)* by Ronny Stiftel (more info [here](http://www.mac-and-i.net/2013/12/beben-iii-openarenaquake-3-for-ios.html)), ultimately derived from [ioquake3](https://ioquake3.org/).  On-screen joystick code came from [this repo](https://github.com/bradhowes/Joystick) by Brad Howe. Quake font DpQuake by Dead Pete available [here](https://www.dafont.com/quake.font). Code to query servers came from [Q3ServerBrowser](https://github.com/andreagiavatto/Q3ServerBrowser) on GitHub by Andrea Giavatto.

I have now modified the code to use [SDL for iOS](https://www.libsdl.org/), employing [OpenGL ES work](https://github.com/zturtleman/ioq3/tree/opengles1) from Zack Middleton. The original, GLKit-based version is available on the [legacy](https://github.com/tomkidd/Quake3-iOS/tree/legacy) branch. 

[Video of Quake III: Arena running on an iPhone X](https://www.youtube.com/watch?v=4Fu1fmXtcvo)

[Video of Quake III: Arena running on an Apple TV](https://www.youtube.com/watch?v=ade-J3RYpsQ)

I have also made apps for [*Wolfenstein 3-D*](https://github.com/tomkidd/Wolf3D-iOS), [*DOOM*, *DOOM II* and *Final DOOM*](https://github.com/tomkidd/DOOM-iOS), [*Quake*](https://github.com/tomkidd/Quake-iOS), [*Quake II*](https://github.com/tomkidd/Quake2-iOS), [*Return to Castle Wolfenstein*](https://github.com/tomkidd/RTCW-iOS) and [*DOOM 3*](https://github.com/tomkidd/DOOM3-iOS).

Have fun. For any questions I can be reached at tomkidd@gmail.com
