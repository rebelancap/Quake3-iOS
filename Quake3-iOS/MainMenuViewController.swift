//
//  MainMenuViewController.swift
//  Quake3-iOS
//
//  Created by Tom Kidd on 8/8/18.
//  Copyright © 2018 Tom Kidd. All rights reserved.
//

import UIKit
import ZIPFoundation

class MainMenuViewController: UIViewController {

    let defaults = UserDefaults()
    
    // Check if baseq3 exists in Documents
    var hasBaseq3: Bool {
        let fileManager = FileManager.default
        #if os(tvOS)
        let documentsDir = try! FileManager().url(for: .cachesDirectory, in: .userDomainMask, appropriateFor: nil, create: true).path
        #else
        let documentsDir = try! FileManager().url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: true).path
        #endif
        
        let baseq3Path = documentsDir + "/baseq3"
        let pak0Path = baseq3Path + "/pak0.pk3"
        
        return fileManager.fileExists(atPath: baseq3Path) && fileManager.fileExists(atPath: pak0Path)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Add notification observers for URL launches
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(handleLaunchVanilla),
            name: NSNotification.Name("LaunchVanillaQuake3"),
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(handleLaunchMod(_:)),
            name: NSNotification.Name("LaunchQuake3Mod"),
            object: nil
        )
        
        if defaults.string(forKey: "playerName") == nil {
            defaults.set("iOSPlayer", forKey: "playerName")
        }
        
        // Check if game files are available
        if !hasBaseq3 {
            showMissingFilesAlert()
            return
        }
        
        // Extract files from Documents/baseq3 instead of bundled baseq3
        extractFilesFromDocuments()
    }
    
    // Add these new methods
    @objc func handleLaunchVanilla() {
        // Check if baseq3 exists first
        if !hasBaseq3 {
            showMissingFilesAlert()
            return
        }
        
        // Directly instantiate and present GameViewController
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            self.launchGameDirectly()
        }
    }
    
    @objc func handleLaunchMod(_ notification: Notification) {
        // Check if baseq3 exists first
        if !hasBaseq3 {
            showMissingFilesAlert()
            return
        }
        
        if let modName = notification.userInfo?["mod"] as? String {
            // Store mod for GameViewController - USE SAME KEY AS APPDELEGATE
            UserDefaults.standard.set(modName, forKey: "launchMod")
            UserDefaults.standard.synchronize()
            
            // Launch the game
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                self.launchGameDirectly()
            }
        }
    }
    
    func launchGameDirectly() {
        // Create GameViewController directly
        let storyboard = UIStoryboard(name: "Main", bundle: nil)
        if let gameVC = storyboard.instantiateViewController(withIdentifier: "GameViewController") as? GameViewController {
            // Present it modally to bypass the iOS menu
            gameVC.modalPresentationStyle = .fullScreen
            self.present(gameVC, animated: false, completion: nil)
        }
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        // Check if we should launch vanilla on start
        if UserDefaults.standard.bool(forKey: "launchVanillaOnStart") {
            UserDefaults.standard.set(false, forKey: "launchVanillaOnStart")
            UserDefaults.standard.synchronize()
            
            // Small delay to ensure any mod settings are saved
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                self.launchGameDirectly()
            }
        }
    }
    
    // Add this to clean up observers
    deinit {
        NotificationCenter.default.removeObserver(self)
    }
    
    func showMissingFilesAlert() {
        let alert = UIAlertController(
            title: "Game Files Missing",
            message: "Please copy the 'baseq3' folder containing pak0.pk3 (and other pak files) to the app's Documents folder using iTunes File Sharing or the Files app.\n\nThe game cannot start without these files.",
            preferredStyle: .alert
        )
        
        alert.addAction(UIAlertAction(title: "OK", style: .default) { _ in
            // Optionally check again when user dismisses
            if self.hasBaseq3 {
                self.extractFilesFromDocuments()
            }
        })
        
        present(alert, animated: true)
    }
    
    func extractFilesFromDocuments() {
        #if os(tvOS)
        let documentsDir = try! FileManager().url(for: .cachesDirectory, in: .userDomainMask, appropriateFor: nil, create: true).path
        #else
        let documentsDir = try! FileManager().url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: true).path
        #endif
        
        // Extract from Documents/baseq3 instead of bundled baseq3
        let baseq3Path = documentsDir + "/baseq3"
        
        // Extract levelshots
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3CTF1.jpg", destination: "graphics/Q3CTF1.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3CTF2.jpg", destination: "graphics/Q3CTF2.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3CTF3.jpg", destination: "graphics/Q3CTF3.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3CTF4.jpg", destination: "graphics/Q3CTF4.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM0.jpg", destination: "graphics/Q3DM0.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM1.jpg", destination: "graphics/Q3DM1.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM10.jpg", destination: "graphics/Q3DM10.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM11.jpg", destination: "graphics/Q3DM11.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM12.jpg", destination: "graphics/Q3DM12.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM13.jpg", destination: "graphics/Q3DM13.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM14.jpg", destination: "graphics/Q3DM14.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM15.jpg", destination: "graphics/Q3DM15.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM16.jpg", destination: "graphics/Q3DM16.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM17.jpg", destination: "graphics/Q3DM17.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM18.jpg", destination: "graphics/Q3DM18.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM19.jpg", destination: "graphics/Q3DM19.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM2.jpg", destination: "graphics/Q3DM2.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM3.jpg", destination: "graphics/Q3DM3.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM4.jpg", destination: "graphics/Q3DM4.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM5.jpg", destination: "graphics/Q3DM5.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM6.jpg", destination: "graphics/Q3DM6.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM7.jpg", destination: "graphics/Q3DM7.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM8.jpg", destination: "graphics/Q3DM8.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3DM9.jpg", destination: "graphics/Q3DM9.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3TOURNEY1.jpg", destination: "graphics/Q3TOURNEY1.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3TOURNEY2.jpg", destination: "graphics/Q3TOURNEY2.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3TOURNEY3.jpg", destination: "graphics/Q3TOURNEY3.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3TOURNEY4.jpg", destination: "graphics/Q3TOURNEY4.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3TOURNEY5.jpg", destination: "graphics/Q3TOURNEY5.jpg")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "levelshots/Q3TOURNEY6.jpg", destination: "graphics/Q3TOURNEY6.jpg")
        
        // Extract player icons from pak0
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "models/players/anarki/icon_blue.tga", destination: "graphics/anarki/icon_blue.tga")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "models/players/anarki/icon_default.tga", destination: "graphics/anarki/icon_default.tga")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "models/players/anarki/icon_red.tga", destination: "graphics/anarki/icon_red.tga")
        // ... (all other player icons from pak0.pk3)
        
        // Extract skill icons
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "menu/art/skill1.tga", destination: "graphics/menu/art/skill1.tga")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "menu/art/skill2.tga", destination: "graphics/menu/art/skill2.tga")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "menu/art/skill3.tga", destination: "graphics/menu/art/skill3.tga")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "menu/art/skill4.tga", destination: "graphics/menu/art/skill4.tga")
        extractFile(pk3: baseq3Path + "/pak0.pk3", source: "menu/art/skill5.tga", destination: "graphics/menu/art/skill5.tga")
        
        // Extract from pak2 if it exists
        let pak2Path = baseq3Path + "/pak2.pk3"
        if FileManager.default.fileExists(atPath: pak2Path) {
            extractFile(pk3: pak2Path, source: "models/players/brandon/icon_default.tga", destination: "graphics/brandon/icon_default.tga")
            extractFile(pk3: pak2Path, source: "models/players/carmack/icon_default.tga", destination: "graphics/carmack/icon_default.tga")
            extractFile(pk3: pak2Path, source: "models/players/cash/icon_default.tga", destination: "graphics/cash/icon_default.tga")
            extractFile(pk3: pak2Path, source: "models/players/paulj/icon_default.tga", destination: "graphics/paulj/icon_default.tga")
            extractFile(pk3: pak2Path, source: "models/players/tim/icon_default.tga", destination: "graphics/tim/icon_default.tga")
            extractFile(pk3: pak2Path, source: "models/players/xian/icon_default.tga", destination: "graphics/xian/icon_default.tga")
        }
    }
    
    func extractFile(pk3: String, source: String, destination: String) {
        let fileManager = FileManager()
        #if os(tvOS)
        let documentsDir = try! FileManager().url(for: .cachesDirectory, in: .userDomainMask, appropriateFor: nil, create: true).path
        #else
        let documentsDir = try! FileManager().url(for: .documentDirectory, in: .userDomainMask, appropriateFor: nil, create: true).path
        #endif

        var destinationURL = URL(fileURLWithPath: documentsDir)
        destinationURL.appendPathComponent(destination)
        
        if fileManager.fileExists(atPath: destinationURL.path) {
            return
        }
        
        // Use full path directly instead of relative path
        let archiveURL = URL(fileURLWithPath: pk3)
        guard let archive = Archive(url: archiveURL, accessMode: .read) else  {
            print("Failed to open archive: \(pk3)")
            return
        }
        guard let entry = archive[source] else {
            print("Entry not found in archive: \(source)")
            return
        }
        do {
            let _ = try archive.extract(entry, to: destinationURL)
        } catch {
            print("Extracting entry from archive failed with error:\(error)")
        }
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    @IBAction func exitToMainMenu(segue: UIStoryboardSegue) {
    }

}
