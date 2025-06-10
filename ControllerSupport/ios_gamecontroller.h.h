//
//  ios_gamecontroller.h.h
//  Quake3-iOS
//
//  Created by Austin Archibald on 6/7/25.
//  Copyright © 2025 Tom Kidd. All rights reserved.
//

#ifndef IOS_GAMECONTROLLER_H
#define IOS_GAMECONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

void IOS_InitGameControllers(void);
void IOS_ShutdownGameControllers(void);
void IOS_UpdateGameControllers(void);

#ifdef __cplusplus
}
#endif

#endif // IOS_GAMECONTROLLER_H
