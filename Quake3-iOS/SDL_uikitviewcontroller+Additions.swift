//
//  SDL2ViewController+Additions.swift
//  Quake2-iOS
//
//  Created by Tom Kidd on 1/28/19.
//

import UIKit

// Give access to Virtual Keyboard
@_silgen_name("Con_ToggleVirtualKeyboard")
func Con_ToggleVirtualKeyboard()

@_silgen_name("Cbuf_AddText")
func Cbuf_AddText(_ text: UnsafePointer<CChar>)

extension SDL_uikitviewcontroller {
    
    // A method of getting around the fact that Swift extensions cannot have stored properties
    // https://medium.com/@valv0/computed-properties-and-extensions-a-pure-swift-approach-64733768112c
    struct Holder {
        static var _fireButton = UIButton()
        static var _jumpButton = UIButton()
        static var _joystickView = JoyStickView(frame: .zero)
        static var _tildeButton = UIButton()
        static var _expandButton = UIButton()
        static var _escapeButton = UIButton()
        static var _buttonStack = UIStackView(frame: .zero)
        static var _buttonStackExpanded = false
        static var _f1Button = UIButton()
        static var _prevWeaponButton = UIButton()
        static var _nextWeaponButton = UIButton()
        static var _rightJoystickView = JoyStickView(frame: .zero)
        static var _quitButton = UIButton()
    }
    
    var fireButton:UIButton {
        get {
            return Holder._fireButton
        }
        set(newValue) {
            Holder._fireButton = newValue
        }
    }
    
    var jumpButton:UIButton {
        get {
            return Holder._jumpButton
        }
        set(newValue) {
            Holder._jumpButton = newValue
        }
    }
    
    var joystickView:JoyStickView {
        get {
            return Holder._joystickView
        }
        set(newValue) {
            Holder._joystickView = newValue
        }
    }

    var tildeButton:UIButton {
        get {
            return Holder._tildeButton
        }
        set(newValue) {
            Holder._tildeButton = newValue
        }
    }

    var escapeButton:UIButton {
        get {
            return Holder._escapeButton
        }
        set(newValue) {
            Holder._escapeButton = newValue
        }
    }

    var expandButton:UIButton {
        get {
            return Holder._expandButton
        }
        set(newValue) {
            Holder._expandButton = newValue
        }
    }
    
    var buttonStack:UIStackView {
        get {
            return Holder._buttonStack
        }
        set(newValue) {
            Holder._buttonStack = newValue
        }
    }

    var buttonStackExpanded:Bool {
        get {
            return Holder._buttonStackExpanded
        }
        set(newValue) {
            Holder._buttonStackExpanded = newValue
        }
    }
    
    var f1Button:UIButton {
        get {
            return Holder._f1Button
        }
        set(newValue) {
            Holder._f1Button = newValue
        }
    }
    
    var prevWeaponButton:UIButton {
        get {
            return Holder._prevWeaponButton
        }
        set(newValue) {
            Holder._prevWeaponButton = newValue
        }
    }

    var nextWeaponButton:UIButton {
        get {
            return Holder._nextWeaponButton
        }
        set(newValue) {
            Holder._nextWeaponButton = newValue
        }
    }
    
    var rightJoystickView:JoyStickView {
        get {
            return Holder._rightJoystickView
        }
        set(newValue) {
            Holder._rightJoystickView = newValue
        }
    }
    
    var quitButton:UIButton {
        get {
            return Holder._quitButton
        }
        set(newValue) {
            Holder._quitButton = newValue
        }
    }

    @objc func fireButton(rect: CGRect) -> UIButton {
        fireButton = UIButton(frame: CGRect(x: rect.width - 250, y: rect.height - 90, width: 75, height: 75))
        fireButton.setTitle("FIRE", for: .normal)
        fireButton.setBackgroundImage(UIImage(named: "JoyStickBase")!, for: .normal)
        fireButton.addTarget(self, action: #selector(self.firePressed), for: .touchDown)
        fireButton.addTarget(self, action: #selector(self.fireReleased), for: .touchUpInside)
        fireButton.alpha = 0.5
        return fireButton
    }
    
    @objc func jumpButton(rect: CGRect) -> UIButton {
        jumpButton = UIButton(frame: CGRect(x: rect.width - 90, y: rect.height - 135, width: 75, height: 75))
        jumpButton.setTitle("JUMP", for: .normal)
        jumpButton.setBackgroundImage(UIImage(named: "JoyStickBase")!, for: .normal)
        jumpButton.addTarget(self, action: #selector(self.jumpPressed), for: .touchDown)
        jumpButton.addTarget(self, action: #selector(self.jumpReleased), for: .touchUpInside)
        jumpButton.alpha = 0.5
        return jumpButton
    }
    
    @objc func joyStick(rect: CGRect) -> JoyStickView {
        let size = CGSize(width: 100.0, height: 100.0)
        let joystick1Frame = CGRect(origin: CGPoint(x: 50.0,
                                                    y: (rect.height - size.height - 50.0)),
                                    size: size)
        joystickView = JoyStickView(frame: joystick1Frame)
        joystickView.delegate = self
        
        joystickView.movable = false
        joystickView.alpha = 0.5
        joystickView.baseAlpha = 0.5 // let the background bleed thru the base
        joystickView.handleTintColor = UIColor.darkGray // Colorize the handle
        return joystickView
    }
    
    @objc func buttonStack(rect: CGRect) -> UIStackView {
        
        
        expandButton = UIButton(type: .custom)
        expandButton.setTitle(" > ", for: .normal)
        expandButton.addTarget(self, action: #selector(self.expand), for: .touchUpInside)
        expandButton.sizeToFit()
        expandButton.alpha = 0.5
        expandButton.frame.size.width = 50

        tildeButton = UIButton(type: .custom)
        tildeButton.setTitle(" ~ ", for: .normal)
        tildeButton.addTarget(self, action: #selector(self.tildePressed), for: .touchDown)
        tildeButton.addTarget(self, action: #selector(self.tildeReleased), for: .touchUpInside)
        tildeButton.alpha = 0
        tildeButton.isHidden = true

        escapeButton = UIButton(type: .custom)
        escapeButton.setTitle(" ESC ", for: .normal)
        escapeButton.addTarget(self, action: #selector(self.escapePressed), for: .touchDown)
        escapeButton.addTarget(self, action: #selector(self.escapeReleased), for: .touchUpInside)
        escapeButton.layer.borderColor = UIColor.white.cgColor
        escapeButton.layer.borderWidth = CGFloat(1)
        escapeButton.alpha = 0
        escapeButton.isHidden = true

        buttonStack = UIStackView(frame: .zero)
        buttonStack.frame.origin = CGPoint(x: 50, y: 50)
        buttonStack.translatesAutoresizingMaskIntoConstraints = false
        buttonStack.axis = .horizontal
        buttonStack.spacing = 8.0
        buttonStack.alignment = .leading
        buttonStack.addArrangedSubview(expandButton)
//        buttonStack.addArrangedSubview(tildeButton)
        buttonStack.addArrangedSubview(escapeButton)

        return buttonStack
        
    }
    
    @objc func f1Button(rect: CGRect) -> UIButton {
        f1Button = UIButton(frame: CGRect(x: rect.width - 40, y: 10, width: 30, height: 30))
        f1Button.setTitle(" C ", for: .normal)  // Changed from " F1 " to " C "
        f1Button.addTarget(self, action: #selector(self.consoleTogglePressed), for: .touchDown)
        f1Button.layer.borderColor = UIColor.white.cgColor
        f1Button.layer.borderWidth = CGFloat(1)
        f1Button.alpha = 0.5
        return f1Button
    }
    
    @objc func quitButton(rect: CGRect) -> UIButton {
        quitButton = UIButton(frame: CGRect(x: 10, y: 10, width: 30, height: 30))  // Top left
        quitButton.setTitle(" Q ", for: .normal)
        quitButton.addTarget(self, action: #selector(self.quitPressed), for: .touchDown)
        quitButton.layer.borderColor = UIColor.white.cgColor
        quitButton.layer.borderWidth = CGFloat(1)
        quitButton.alpha = 0.5
        return quitButton
    }
    
    @objc func prevWeaponButton(rect: CGRect) -> UIButton {
        prevWeaponButton = UIButton(frame: CGRect(x: (rect.width / 3), y: rect.height/2, width: (rect.width / 3), height: rect.height/2))
        prevWeaponButton.addTarget(self, action: #selector(self.prevWeaponPressed), for: .touchDown)
        prevWeaponButton.addTarget(self, action: #selector(self.prevWeaponReleased), for: .touchUpInside)
        return prevWeaponButton
    }
    
    @objc func nextWeaponButton(rect: CGRect) -> UIButton {
        nextWeaponButton = UIButton(frame: CGRect(x: (rect.width / 3), y: 0, width: (rect.width / 3), height: rect.height/2))
        nextWeaponButton.addTarget(self, action: #selector(self.nextWeaponPressed), for: .touchDown)
        nextWeaponButton.addTarget(self, action: #selector(self.nextWeaponReleased), for: .touchUpInside)
        return nextWeaponButton
    }
    
    @objc func rightJoyStick(rect: CGRect) -> JoyStickView {
        let size = CGSize(width: 100.0, height: 100.0)
        let rightJoystickFrame = CGRect(origin: CGPoint(x: rect.width - size.width - 50.0,
                                                         y: (rect.height - size.height - 50.0)),
                                        size: size)
        rightJoystickView = JoyStickView(frame: rightJoystickFrame)
        rightJoystickView.delegate = self
        rightJoystickView.tag = 2  // Different tag from left joystick
        
        rightJoystickView.movable = false
        rightJoystickView.alpha = 0.5
        rightJoystickView.baseAlpha = 0.5
        rightJoystickView.handleTintColor = UIColor.darkGray
        return rightJoystickView
    }

    
    @objc func firePressed(sender: UIButton!) {
        // Try sending the command directly instead of a key
        Cbuf_AddText("+attack\n")
    }

    @objc func fireReleased(sender: UIButton!) {
        Cbuf_AddText("-attack\n")
    }
    
    @objc func jumpPressed(sender: UIButton!) {
        Key_Event(32, qboolean(1), qboolean(1))
    }
    
    @objc func jumpReleased(sender: UIButton!) {
        Key_Event(32, qboolean(0), qboolean(1))
    }
    
    @objc func tildePressed(sender: UIButton!) {
//        Key_Event(32, qboolean(1), qboolean(1))
    }
    
    @objc func tildeReleased(sender: UIButton!) {
//        Key_Event(32, qboolean(0), qboolean(1))
    }
    
    @objc func escapePressed(sender: UIButton!) {
        Key_Event(27, qboolean(1), qboolean(1))
    }
    
    @objc func escapeReleased(sender: UIButton!) {
        Key_Event(27, qboolean(0), qboolean(1))
    }
        
    @objc func consoleTogglePressed(sender: UIButton!) {
        // Match what IN_ToggleiOSKeyboard does in sdl_input.c
        Con_ToggleVirtualKeyboard()  // This opens console AND virtual keyboard
    }
    
    @objc func prevWeaponPressed(sender: UIButton!) {
        Key_Event(183, qboolean(1), qboolean(1))
    }
    
    @objc func prevWeaponReleased(sender: UIButton!) {
        Key_Event(183, qboolean(0), qboolean(1))
    }
    
    @objc func nextWeaponPressed(sender: UIButton!) {
        Key_Event(184, qboolean(1), qboolean(1))
    }
    
    @objc func nextWeaponReleased(sender: UIButton!) {
        Key_Event(184, qboolean(0), qboolean(1))
    }

    
    func Key_Event(_ key: Int32, _ down: qboolean, _ special: qboolean) {
        CL_KeyEvent(key, down, UInt32(Sys_Milliseconds()))
    }

    @objc func expand(_ sender: Any) {
        buttonStackExpanded = !buttonStackExpanded
        
        UIView.animate(withDuration: 0.5) {
            self.expandButton.setTitle(self.buttonStackExpanded ? " < " : " > ", for: .normal)
            self.expandButton.alpha = self.buttonStackExpanded ? 1 : 0.5
            self.escapeButton.isHidden = !self.buttonStackExpanded
            self.escapeButton.alpha = self.buttonStackExpanded ? 1 : 0
            self.tildeButton.isHidden = !self.buttonStackExpanded
            self.tildeButton.alpha = self.buttonStackExpanded ? 1 : 0
        }
    }
    
    @objc func toggleControls(_ hide: Bool) {
        // Check if console is active
        let consoleActive = (Key_GetCatcher() & KEYCATCH_CONSOLE) != 0
        
        // Check if we're in game
        let inGame = (Key_GetCatcher() & (KEYCATCH_UI | KEYCATCH_CONSOLE)) == 0
        
        // Hide these controls when UI is hidden OR when console is active
        let shouldHide = hide || consoleActive
        
        self.fireButton.isHidden = shouldHide
        // self.jumpButton.isHidden = shouldHide  // Already commented out
        self.joystickView.isHidden = shouldHide
        self.rightJoystickView.isHidden = shouldHide
        self.prevWeaponButton.isHidden = shouldHide
        self.nextWeaponButton.isHidden = shouldHide
        
        // Quit button only visible when in game and no controller
        #if USE_IOS_GAMECONTROLLER
        self.quitButton.isHidden = !inGame || iOS_IsControllerConnected()
        #else
        self.quitButton.isHidden = !inGame
        #endif
        
        // Always hide these when requested (but not affected by console)
        self.buttonStack.isHidden = hide
        
        // Never hide the F1/C button
        // self.f1Button.isHidden = hide
    }
    
    @objc func quitPressed(sender: UIButton!) {
        // Execute killserver command
        Cbuf_AddText("disconnect\n")
    }
    
    open override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    open override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
    }
    
    
    
}

extension SDL_uikitviewcontroller: JoystickDelegate {
    
    func handleJoyStickPosition(sender: JoyStickView, x: CGFloat, y: CGFloat) {
        // Deadzone configuration
        let deadzone: CGFloat = 0.02  // Adjust this (0.0 to 1.0)
        
        // Apply deadzone
        var adjustedX = x
        var adjustedY = y
        let magnitude = sqrt(x * x + y * y)
        
        if magnitude < deadzone {
            adjustedX = 0
            adjustedY = 0
        } else {
            // Scale from deadzone to 1.0
            let scaledMagnitude = (magnitude - deadzone) / (1.0 - deadzone)
            adjustedX = (x / magnitude) * scaledMagnitude
            adjustedY = (y / magnitude) * scaledMagnitude
        }
        
        if sender.tag == 2 {
            // Right joystick - for aiming/looking
            let sensitivity: CGFloat = 0.6
            let acceleration: CGFloat = 1.2  // Power curve for acceleration (1.0 = linear, 2.0 = quadratic)
            // Typically want 1:2 sensitivity:acceleration ratio
            // Most console FPS games use values between 1.5 and 2.5 for aiming acceleration
            
            // Apply acceleration curve
            let accelX = adjustedX * pow(abs(adjustedX), acceleration - 1)
            let accelY = adjustedY * pow(abs(adjustedY), acceleration - 1)
            
            // Scale to mouse pixel movement (try different values here)
            let mouseScale: CGFloat = 50  // This is your main tuning parameter | make it higher for older phones
            
            let deltaX = Int32(accelX * sensitivity * mouseScale)
            let deltaY = Int32(-accelY * sensitivity * mouseScale) // inverted Y
            
            // Send mouse motion events
            if deltaX != 0 || deltaY != 0 {
                CL_MouseEvent(deltaX, deltaY, Int32(Sys_Milliseconds()), qtrue)
            }
        } else {
            // Left joystick - proportional movement
            if adjustedY != 0 {
                if adjustedY > 0 {
                    cl_joyscale_y.0 = Int32(abs(adjustedY) * 60)
                    Key_Event(132, qboolean(1), qboolean(1))
                    Key_Event(133, qboolean(0), qboolean(1))
                } else {
                    cl_joyscale_y.1 = Int32(abs(adjustedY) * 60)
                    Key_Event(132, qboolean(0), qboolean(1))
                    Key_Event(133, qboolean(1), qboolean(1))
                }
            } else {
                cl_joyscale_y.0 = 0
                cl_joyscale_y.1 = 0
                Key_Event(132, qboolean(0), qboolean(1))
                Key_Event(133, qboolean(0), qboolean(1))
            }

            // Use strafe keys (A=97, D=100) instead of turn keys & Remove the 0.25 threshold for X axis
            if adjustedX != 0 {
                if adjustedX > 0 {
                    cl_joyscale_x.0 = Int32(abs(adjustedX) * 60)
                    Key_Event(100, qboolean(1), qboolean(1))  // 'D' - strafe right
                    Key_Event(97, qboolean(0), qboolean(1))   // Release 'A'
                } else {
                    cl_joyscale_x.1 = Int32(abs(adjustedX) * 60)
                    Key_Event(97, qboolean(1), qboolean(1))   // 'A' - strafe left
                    Key_Event(100, qboolean(0), qboolean(1))  // Release 'D'
                }
            } else {
                cl_joyscale_x.0 = 0
                cl_joyscale_x.1 = 0
                Key_Event(97, qboolean(0), qboolean(1))
                Key_Event(100, qboolean(0), qboolean(1))
            }
        }
    }
    
    func handleJoyStick(sender: JoyStickView, angle: CGFloat, displacement: CGFloat) {
        // print("angle: \(angle) displacement: \(displacement)")
    }
    
}
