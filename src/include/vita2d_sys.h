/* Compatibility shim: the original used GrapheneCt's vita2d_sys (the GPU-side
 * variant needed for 1080p output). The VitaSDK port drops 1080p and uses the
 * standard open-source vita2d instead. A few API differences (init, buffer
 * swap) are handled directly in the .c sources. */
#ifndef VMP_VITA2D_SYS_SHIM_H
#define VMP_VITA2D_SYS_SHIM_H
#include <vita2d.h>
#endif
