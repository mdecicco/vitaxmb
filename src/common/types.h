#include <stdio.h>
#include <glm/glm.hpp>
#include <string>

namespace v {
    typedef void*               Ptr;

    typedef int8_t              i8;
    typedef int16_t             i16;
    typedef int                 i32;
    typedef int64_t             i64;

    typedef uint8_t             u8;
    typedef uint16_t            u16;
    typedef unsigned int        u32;
    typedef uint64_t            u64;

    typedef signed char         s8;
    typedef signed short        s16;
    typedef signed int          s32;
    typedef signed long long    s64;

    typedef float               f32;
    typedef double              f64;

    typedef char                Byte;
    typedef unsigned char       uByte;

    typedef const char*         Literal;
    typedef char*               CString;
    typedef f32                 Scalar;
    typedef bool                Flag;
    
    typedef glm::vec2           vec2;
    typedef glm::vec3           vec3;
    typedef glm::vec4           vec4;
    
    std::string format (const char* text, ...);
};
