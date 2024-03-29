#ifndef _XCB_WRAP_H_
#define _XCB_WRAP_H_

#include <xcb/xcb.h>

// xcb_wrap_ctx_t provides everything that is needed to access an XCB-based surface

struct xcb_wrap_ctx {
    xcb_connection_t* conn; // Connection to the X server
    xcb_screen_t* screen; // The screen
    xcb_window_t window; // Created window
};
typedef struct xcb_wrap_ctx xcb_wrap_ctx_t;

void xcb_wrap_create_ctx(xcb_wrap_ctx_t* ctx, const char* const title);
void xcb_wrap_destroy_ctx(xcb_wrap_ctx_t* ctx);

#endif
