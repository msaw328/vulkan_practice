#include <stdio.h>

#include "xcb_wrap.h"
#include "vk_wrap.h"

int main() {
    xcb_wrap_ctx_t xcb_ctx;
    xcb_wrap_create_ctx(&xcb_ctx, "vulkan-playground");

    vk_wrap_main(&xcb_ctx);
}
