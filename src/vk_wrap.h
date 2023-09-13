#ifndef _VK_WRAP_H_
#define _VK_WRAP_H_

#include <vulkan/vulkan.h>

#include "xcb_wrap.h"

struct vk_wrap_ctx {
    VkInstance instance;
};
typedef struct vk_wrap_ctx vk_wrap_ctx_t;

void vk_wrap_main(xcb_wrap_ctx_t* xcb_ctx);

#endif
