
/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */

#include "lcbhss_space.h"

void* alignedMalloc(size_t size, size_t alignment) {
    if (alignment & (alignment - 1)) {
        return nullptr;
    }
    //维护FreeBlock(void)指针占用的内存大小-----用sizeof获取
    const int pointerSize = sizeof(void*);
    // alignment - 1 + pointerSize是让求得的FreeBlock
    // 内存对齐所需要的内存大小
    // eg：sizeof（T）=20， __alignof(T)=16,
    // g_MaxNumberOfObjectsInPool = 1000
    // 调用：aligned_malloc(1000*20, 16)
    // alignment - 1 + pointSize = 19
    const unsigned requestedSize = size + alignment - 1 + pointerSize;
    // 分配的大小
    void* rawMemory = malloc(requestedSize);
    // 这里是实际分配的内存地址（差不多就是指针运算适应平台的数字形式）
    uintptr_t start = (uintptr_t)rawMemory + pointerSize;

    // 向上舍入
    // ALIGN - 1 是实际内存对齐的粒度
    // ~(ALIGN - 1)是舍入的粒度
    // bytes + (ALIGN-1)为先进行进位，然后截断0
    // 保证了向上的舍入
    // eg: byte = 100, ALIGN = 8
    // ~(ALIGN - 1) = (1000)b
    // bytes + ALIGN-1 = (1101001)b

    // 此处的 & 能高位保留，是由于~导致其他位无关位都变成1
    // ((bytes + ALIGN-1)& ~(ALIGN-1)) = (1101000)b = (104)d
    // 104/8 = 13; 实现了向上的舍入
    // ！当bytes刚好对齐时，保持大小不变
    void* aligned = (void*)((start + alignment - 1) & ~(alignment - 1));

    // 维护一个指向malloc()真正分配的内存的指针
    *(void**)((uintptr_t)aligned - pointerSize) = rawMemory;

    return aligned;     //因为做了向上的舍入，所以不需要多余的清理操作

    // 这样分配出的内存，对齐填充位于最前，后面紧跟维护指针，再后就是对象的chain
}

void alignedFree(void* aligned) {
    void* rawMemory = *(void**)((uintptr_t)aligned - sizeof(void*));
    free(rawMemory);
}

// 或者直接在这放一个uintptr_t ?
bool isAligned(void* data, unsigned alignment) {
    return ((uintptr_t)data & (alignment - 1)) == 0;
}

void* alignedRealloc(void* data, size_t size, size_t alignment) {
    if (isAligned(data,
        static_cast<int> (alignment)
    )) {
        alignedFree(data);
        return alignedMalloc(size, alignment);
    }
    else {
        return nullptr;
    }
}
//-----------------------------------------------------------------------------------------

//=============================================================================
static bool s_log_enabled = true;
void EnableLogging() { s_log_enabled = true; }
void DisableLogging() { s_log_enabled = false; }

inline void LOG_AND_EXIT(const int val) {
    if(s_log_enabled)
        std::cerr << "Error at: "
            << FormatLogMessage(__FILE__, __FUNCTION__, __LINE__)
            << std::endl;
    std::exit(val);
}

const char* FormatLogMessage(const char* file, const char* function, int line) {
    static char buffer[1024];
    sprintf_s(buffer, "In file <%s> function[%s], line: %d\n", file, function, line);
    return buffer;
}

void Log(const char* fmt, ...) {
    if (!s_log_enabled) return;

    char msg[1024];
    va_list myArgs;
    int count = sprintf_s(msg, 1024, "<" PROJECT_NAME "> ");

    va_start(myArgs, fmt);
    count += vsprintf_s(&msg[count], 1024-(size_t)count, fmt, myArgs);
    va_end(myArgs);

    sprintf_s(&msg[count], 1024-(size_t)count, "\n");

#if defined(NO_CONSOLE) && defined(_WIN32)
    OutputDebugStringA(msg);
#else
    printf(msg);
#endif

}

std::vector<char> readFile(const std::string& fileName) {
    // 以二进制方式读入字节码文件
    std::ifstream file(fileName, std::ios::ate
        | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}