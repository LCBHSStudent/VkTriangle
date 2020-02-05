
/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */

#include "lcbhss_space.h"

void* alignedMalloc(size_t size, size_t alignment) {
    if (alignment & (alignment - 1)) {
        return nullptr;
    }
    //ά��FreeBlock(void)ָ��ռ�õ��ڴ��С-----��sizeof��ȡ
    const int pointerSize = sizeof(void*);
    // alignment - 1 + pointerSize������õ�FreeBlock
    // �ڴ��������Ҫ���ڴ��С
    // eg��sizeof��T��=20�� __alignof(T)=16,
    // g_MaxNumberOfObjectsInPool = 1000
    // ���ã�aligned_malloc(1000*20, 16)
    // alignment - 1 + pointSize = 19
    const unsigned requestedSize = size + alignment - 1 + pointerSize;
    // ����Ĵ�С
    void* rawMemory = malloc(requestedSize);
    // ������ʵ�ʷ�����ڴ��ַ��������ָ��������Ӧƽ̨��������ʽ��
    uintptr_t start = (uintptr_t)rawMemory + pointerSize;

    // ��������
    // ALIGN - 1 ��ʵ���ڴ���������
    // ~(ALIGN - 1)�����������
    // bytes + (ALIGN-1)Ϊ�Ƚ��н�λ��Ȼ��ض�0
    // ��֤�����ϵ�����
    // eg: byte = 100, ALIGN = 8
    // ~(ALIGN - 1) = (1000)b
    // bytes + ALIGN-1 = (1101001)b

    // �˴��� & �ܸ�λ������������~��������λ�޹�λ�����1
    // ((bytes + ALIGN-1)& ~(ALIGN-1)) = (1101000)b = (104)d
    // 104/8 = 13; ʵ�������ϵ�����
    // ����bytes�պö���ʱ�����ִ�С����
    void* aligned = (void*)((start + alignment - 1) & ~(alignment - 1));

    // ά��һ��ָ��malloc()����������ڴ��ָ��
    *(void**)((uintptr_t)aligned - pointerSize) = rawMemory;

    return aligned;     //��Ϊ�������ϵ����룬���Բ���Ҫ������������

    // ������������ڴ棬�������λ����ǰ���������ά��ָ�룬�ٺ���Ƕ����chain
}

void alignedFree(void* aligned) {
    void* rawMemory = *(void**)((uintptr_t)aligned - sizeof(void*));
    free(rawMemory);
}

// ����ֱ�������һ��uintptr_t ?
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
    // �Զ����Ʒ�ʽ�����ֽ����ļ�
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