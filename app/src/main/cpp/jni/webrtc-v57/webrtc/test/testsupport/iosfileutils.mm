/*
 *  Copyright 2015 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#if defined(WEBRTC_IOS)

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import <Foundation/Foundation.h>
#include <string.h>

#include "webrtc/base/checks.h"
#include "webrtc/typedefs.h"
#include "webrtc/sdk/objc/Framework/Classes/helpers.h"

namespace webrtc {
namespace test {

using webrtc::ios::NSStringFromStdString;
using webrtc::ios::StdStringFromNSString;

// For iOS, resource files are added to the application bundle in the root
// and not in separate folders as is the case for other platforms. This method
// therefore removes any prepended folders and uses only the actual file name.
std::string IOSResourcePath(std::string name, std::string extension) {
  @autoreleasepool {
    NSString* path = NSStringFromStdString(name);
    NSString* fileName = path.lastPathComponent;
    NSString* fileType = NSStringFromStdString(extension);
    // Get full pathname for the resource identified by the name and extension.
    NSString* pathString = [[NSBundle mainBundle] pathForResource:fileName
                                                           ofType:fileType];
    return StdStringFromNSString(pathString);
  }
}

std::string IOSRootPath() {
  @autoreleasepool {
    NSBundle* mainBundle = [NSBundle mainBundle];
    return StdStringFromNSString(mainBundle.bundlePath) + "/";
  }
}

// For iOS, we don't have access to the output directory. Return the path to the
// temporary directory instead. This is mostly used by tests that need to write
// output files to disk.
std::string IOSOutputPath()  {
  @autoreleasepool {
    NSString* tempDir = NSTemporaryDirectory();
    if (tempDir == nil)
        tempDir = @"/tmp";
    return StdStringFromNSString(tempDir);
  }
}

}  // namespace test
}  // namespace webrtc

#endif  // defined(WEBRTC_IOS)
