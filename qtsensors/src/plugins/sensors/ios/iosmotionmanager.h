// Copyright (C) 2016 Lorn Potter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef IOSMOTIONMANAGER_H
#define IOSMOTIONMANAGER_H

#import <Foundation/Foundation.h>

@class CMMotionManager;

@interface QIOSMotionManager : NSObject {
}

+ (CMMotionManager *)sharedManager;
@end

#endif //IOSMOTIONMANAGER_H

