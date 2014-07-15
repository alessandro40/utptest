# Force the use of a modern toolchain, for C++11 support
#NDK_TOOLCHAIN_VERSION := 4.8
NDK_TOOLCHAIN_VERSION := clang

# Specify platform to remove warning
APP_PLATFORM          := android-14

# Target architectures
APP_ABI               := armeabi
#APP_ABI               := all

# Use an STL library
APP_STL               := c++_static
#APP_STL               := stlport_shared

# Enable exceptions and RTTI
APP_CPPFLAGS          += -fexceptions -frtti
