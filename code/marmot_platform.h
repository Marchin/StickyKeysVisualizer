#ifndef MARMOT_PLATFORM_H
#include <stdint.h>
#include <limits.h>
#include <float.h>

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif
#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#endif

#define internal static
#define local_persist static
#define global static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef s8       b8;
typedef s32      b32;
typedef float    f32;
typedef double   f64;
typedef size_t MemoryIndex;

#define F32_MAX FLT_MAX

#define PI32 3.14159265359f

// NOTE(Marchin): if expression is false, write into 0 (invalid memory) for a
//plaftorm independent break
#if MARMOT_SLOW
#define ASSERT(expression) if(!(expression)) {*(int*)0 = 0;}
#else
#define ASSERT(expression)
#endif

#define INVALID_CODE_PATH ASSERT(!"INVALID_CODE_PATH");
#define INVALID_DEFAULT_CASE default: { INVALID_CODE_PATH; } break;

#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value)*1024LL)
#define GIGABYTES(value) (MEGABYTES(value)*1024LL)
#define TERABYTES(value) (GIGABYTES(value)*1024LL)
#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))

inline u32
safeTruncateU64(u64 value) {
    ASSERT(value <= 0xFFFFFFFF);
    u32 result  = (u32)value;
    return result;
}

typedef struct ThreadContext {
    int placeHolder;
} ThreadContext;


#if MARMOT_INTERNAL
typedef struct DebugReadFileResult {
    u32 contentsSize;
    void* pContents;
} DebugReadFileResult;

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(\
ThreadContext* pThread, void* pMemory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(DebugPlatformFreeFileMemory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) DebugReadFileResult name(\
ThreadContext* pThread, char* pFileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(DebugPlatformReadEntireFile);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(\
ThreadContext* pThread, char* pFileName, u32 memorySize, void* pMemory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DebugPlatformWriteEntireFile);
#endif

const s32 gcBMPBytesPerPixel = 4;

typedef struct GameOffScreenBuffer {
    void* pMemory;
    int width;
    int height;
    int pitch;
} GameOffScreenBuffer;

typedef struct GameSoundOutputBuffer {
    int samplesPerSecond;
    int sampleCount;
    s16* pSamples;
} GameSoundOutputBuffer;

typedef struct GameButtonState {
    int halfTransitionCount;
    b32 endedDown;
} GameButtonState;

typedef struct GameControllerInput {
    b32 isConnected;
    b32 isAnalog;
    f32 stickAverageX;
    f32 stickAverageY;
    
    union {
        GameButtonState buttons[12];
        struct {
            GameButtonState moveUp;
            GameButtonState moveDown;
            GameButtonState moveLeft;
            GameButtonState moveRight;
            
            GameButtonState actionUp;
            GameButtonState actionDown;
            GameButtonState actionLeft;
            GameButtonState actionRight;
            
            GameButtonState leftShoulder;
            GameButtonState rightShoulder;
            
            GameButtonState back;
            GameButtonState start;
            
            GameButtonState terminator;
        };
    };
} GameControllerInput;


typedef struct GameInput {
    GameButtonState mouseButtons[5];
    s32 mouseX, mouseY, mouseZ;
    
    b32 executableReloaded;
    f32 deltaTime;
    
    GameControllerInput controllers[5];
} GameInput;

extern "C" inline GameControllerInput*
getController(GameInput* pInput, int iController) {
    ASSERT(iController < ARRAY_COUNT(pInput->controllers));
    GameControllerInput* pResult = &pInput->controllers[iController];
    return pResult;
}

#define MARMOT_PLATFORM_H
#endif