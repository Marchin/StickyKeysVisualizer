#ifndef WIN32_MARMOT_H

struct Win32OffscreenBuffer {
    // NOTE(Marchin): pixels are little endinan, 32-bit wide (RGB + padding)
    BITMAPINFO info;
    void* pMemory;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
};

struct Win32WindowDimension {
    int width;
    int height;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

struct Win32State {
    void* pGameMemoryBlock;
    u64 totalGameMemorySize;
    
    char exePathName[WIN32_STATE_FILE_NAME_COUNT];
    char* pathLastSlash;
};

#define WIN32_MARMOT_H
#endif //WIN32_MARMOT_H