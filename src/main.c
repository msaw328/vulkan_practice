#include <stdio.h>

#include "xcb_wrap.h"

int main() {
    xcb_wrap_ctx_t xcb_ctx;
    xcb_wrap_create_ctx(&xcb_ctx, "vulkan-playground");

    xcb_wrap_event_loop(&xcb_ctx);

    xcb_wrap_destroy_ctx(&xcb_ctx);
}
