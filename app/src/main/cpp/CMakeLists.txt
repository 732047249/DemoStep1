# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.


INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/noisedetect)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/base/)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/YC_Interface)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/common_audio/)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/common_audio/signal_processing/include/)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/modules/audio_processing/utility/)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/modules/audio_processing/aec/)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/modules/audio_processing/aecm/)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/modules/audio_processing/agc/legacy/)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/webrtc-v57/webrtc/modules/audio_processing/ns)
INCLUDE_DIRECTORIES(/Users/yeeapp/Desktop/develop/DemoStep1/app/src/main/cpp/jni/opus-1.1.3/include/)


add_library( # Sets the name of the library.
        native-lib
        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        #Trap2FindProblem.cpp
        AListener.cpp
        native-lib.cpp
        YCRecorder.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

find_library( # Sets the name of the path variable.
        OpenSLES-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        OpenSLES)


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        native-lib
        ${OpenSLES-lib}
        #opus
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})

#add_library(
#        opus SHARED IMPORTED
#)
#
#target_link_libraries( # Specifies the target library.
#        native-lib
#        opus
#        # Links the target library to the log library
#        # included in the NDK.
#        ${log-lib})
#
#set_target_properties(
#        opus
#        PROPERTIES IMPORTED_LOCATION
#        ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/libs/${ANDROID_ABI}/libopus.so
#)

