<img align="left" width="100" height="100" src="https://raw.githubusercontent.com/tomkidd/Quake3-iOS/master/icon_quake3.png">  

#  Quake III: Arena for iOS

&nbsp;

Full-featured Quake III Arena port with multiplayer, modern controller support, mod support, console management with virtual keyboard, easy menu navigation with controller, touchscreen controls, and iOS Shortcuts support (to launch a q3 mod directly). 

## Installation

Checkout [releases](https://github.com/rebelancap/Quake3-iOS/releases/latest) to sideload Quake 3 via [Xcode](https://github.com/rebelancap/Quake3-iOS/wiki/Sideloading-Quake-3.ipa), Sideloadly, Altstore, etc. Or [build](https://github.com/rebelancap/Quake3-iOS/wiki/Building-Quake-3-with-Xcode) it yourself. Then use Files app or macOS Finder to transfer baseq3 folder or other mods.

## Important Info

- Use macOS Finder or Files app to transfer baseq3 or mod folders.

- Set up your `.cfg` files (containing your settings and bindings) and place in baseq3 or other mod folders. [Here are my examples](https://github.com/rebelancap/Quake3-iOS/wiki/Configuration-Files-Examples) for baseq3 and q3ut4 (Urban Terror).

- Tap the π symbol in the bottom right corner to launch full version of Quake 3. (A slower way is use the custom menu to create any game, then press START, Exit Arena.) Or create an [iOS Shortcut](https://github.com/rebelancap/Quake3-iOS/wiki/iOS-Shortcuts) to launch directly or your mod and map of choice!

- Menu Navigation. You can navigate the Quake 3 menus by using DPAD, A (Enter), B or START (Esc). Or use left joystick to move mouse, Right Trigger to mouse click. These would have to be changed in Xcode, but you can generally change any in-game bindings/preferences with the `.cfg` files. 

- **Console Management**. Press both thumbsticks down to triggle console + virtual keyboard. (or C button in top right if no controller)
Use `con_scale` to increase text size and con_margin to increase margin padding (if you have a notch). e.g. `con_scale 2.0` & `con_margin 130` is what I use on an iPhone 16 Pro Max.

- **Note!** If your controller gets disconnected, you need to restart the game to reconnect the controller. Had to disable SDL controller and enable Native GameController Framework to get MFi controllers to work. Claude and I could not figure out a way to solve this.

---

Background: I could not get @tomkidd's port to work with any of my controllers: Backbone Pro, Gamesir G8, any bluetooth controllers, etc. I know very little about coding, so Claude is to thank for most of this. Slowly vibe coded in features I wanted.

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
