#ifndef _MP4_DEFS_H_
#define _MP4_DEFS_H_

#include <cstdint>
#ifndef MAX
    #define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
    #define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define MP4_UNUSED(val) ((void)val)
#define MP4_UUID_LEN    (16)

using Mp4BoxType = uint32_t;

#endif // _MP4_DEFS_H_