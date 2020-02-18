#pragma once
#pragma warning(disable:26812)

/* Created by LCBHSStudent */
/*       2020 Feb 3rd      */

#include <cstdio>
#include <memory>
#include <cstdlib>
#include <iomanip>
#include <cstdarg>
#include <iostream>
#include <vector>
#include <fstream>

//constexpr auto PROJECT_NAME = "Vulkan Demo";
#define PROJECT_NAME "Vulkan Demo"

#define     LOG_AND_RETURN(X) return Log(FormatLogMessage(__FILE__, __FUNCTION__, __LINE__)), X
extern inline void LOG_AND_EXIT(const int val);
extern void        EnableLogging();
extern void        DisableLogging();


#define OFFSET_OF(TYPE, MEMBER) ((uintptr_t)(&((TYPE *)0)->MEMBER))
#define GET_MEMBER(TYPE, STRUCTURE, MEMBER) (*((typeof(((TYPE *)0)->MEMBER)*) \
            ((void*)(OFFSET_OF(TYPE, MEMBER) + (uintptr_t)&STRUCTURE))))
#define PRINT_MEMBER(TYPE, STRUCTURE, MEMBER) std::cout << std::setw(15) << std::left \
            << "["#MEMBER "] of [" #STRUCTURE "]: " << GET_MEMBER(TYPE, STRUCTURE, MEMBER) << std::endl

void* alignedMalloc(
    size_t size, size_t alignment);
void  alignedFree(
    void* aligned);
bool  isAligned(
    void* data, unsigned alignment);
void* alignedRealloc(
    void* data, size_t size, size_t alignment);

void Log(
    const char* fmt, ...);
const char* FormatLogMessage(
    const char* file, const char* function, int line);

extern std::vector<char>
    readFile(const std::string & fileNmae);

template<typename T>
void quickSort(
    std::vector<T>&     _list,
    bool                (*_cmpFun)(T,T) // ascending or descending
) {
    static std::vector<T> list = _list;
    static bool(*cmpFun)(T,T)  = _cmpFun;

    struct _swap {
        void operator()(int a, int b) {
            T temp  = list[a];
            list[a] = list[b];
            list[b] = temp;
        }
    } static swap;
    struct _split {
        uint32_t operator()(uint32_t low, uint32_t high) {

            T key       = list[low];
            uint32_t  i = low,
                      j = high;

            while (i < j) {
                while (i < j &&
                       cmpFun(list[j], key)
                ) j--;
                while (i < j &&
                      !cmpFun(list[i], key)
                ) i++;
                swap(i, (i==j? low: j));
            }
            return i;
        }
    } static splict;
    struct _qsort {
        void operator()(uint32_t low, uint32_t heigh) {
            if(low < heigh) {
                uint32_t key = splict(low, heigh);
                this->operator()(low, key - 1);
                this->operator()(key+1, heigh);
            }
        }
    } static qsort;

    qsort(0, static_cast<uint32_t>(list.size()-1));
    _list = list;
}

constexpr auto UNDEFINED_ERROR              = 100;
constexpr auto UNHANDLED_ERROR              = 101;
constexpr auto CREATE_WINDOW_FAILED         = 102;
constexpr auto CREATE_VK_INSTANCE_FAILED    = 103;
constexpr auto CREATE_LOGICAL_DEVICE_FAILED = 104;

class Error : public std::exception {
public:
    explicit Error(const char* errMsg) : msg(new char[128]) {
        try {
            memcpy_s(msg, 128, errMsg, strlen(errMsg));
        }
        catch (std::bad_alloc& err) {
            std::cout << err.what() << std::endl;
        }
    }
    const char* what() const noexcept override { return msg; }
    virtual ~Error() {
        delete[] msg;
        msg = nullptr;
    }
private:
    char* msg = nullptr;
//   const size_t bufferSize = 128;
};

//enum class ERR_CODE {
//    UNDEFINED_ERROR,
//    UNHANDLED_ERROR,
//};