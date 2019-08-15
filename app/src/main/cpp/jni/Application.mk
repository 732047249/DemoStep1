APP_PLATFORM := android-16

APP_MODULES += zayhu

APP_OPTIM := release
APP_CFLAGS += -O3 -fvisibility=hidden -fstack-protector-all
APP_ABI :=  armeabi-v7a  x86
APP_STL := c++_static
