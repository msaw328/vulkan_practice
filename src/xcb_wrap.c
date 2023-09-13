#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include "xcb_wrap.h"

#define WIDTH 640
#define HEIGHT 480

void xcb_wrap_create_ctx(xcb_wrap_ctx_t* ctx, const char* const title) {
    ctx->conn = xcb_connect(NULL, NULL);


    const xcb_setup_t* setup = xcb_get_setup(ctx->conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    ctx->screen = iter.data;

    ctx->window = xcb_generate_id(ctx->conn);

    uint32_t opt_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t opt_values[2] = {
        ctx->screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS
    };

    xcb_create_window(
                ctx->conn,                      /* Connection          */
                XCB_COPY_FROM_PARENT,           /* depth (same as root)*/
                ctx->window,                    /* window Id           */
                ctx->screen->root,              /* parent window       */
                0, 0,                           /* x, y                */
                WIDTH, HEIGHT,                  /* width, height       */
                10,                             /* border_width        */
                XCB_WINDOW_CLASS_INPUT_OUTPUT,  /* class               */
                ctx->screen->root_visual,       /* visual              */
                opt_mask, opt_values);          /* options like events */

    xcb_map_window(ctx->conn, ctx->window);

    xcb_change_property(
                ctx->conn,
                XCB_PROP_MODE_REPLACE,
                ctx->window,
                XCB_ATOM_WM_NAME,
                XCB_ATOM_STRING,
                8,
                strlen(title),
                title);

    // https://stackoverflow.com/a/27771295/5457426
    xcb_size_hints_t hints;
    xcb_icccm_size_hints_set_min_size(&hints, WIDTH, HEIGHT);
    xcb_icccm_size_hints_set_max_size(&hints, WIDTH, HEIGHT);
    xcb_icccm_set_wm_size_hints(ctx->conn, ctx->window, XCB_ATOM_WM_NORMAL_HINTS, &hints);

    xcb_flush(ctx->conn);
}

void xcb_wrap_destroy_ctx(xcb_wrap_ctx_t* ctx) {
    xcb_disconnect(ctx->conn);
}

void xcb_wrap_event_loop(xcb_wrap_ctx_t* ctx) {
    xcb_generic_event_t* ev = NULL;
    while((ev = xcb_wait_for_event(ctx->conn))) {
        switch (ev->response_type & ~0x80) {

            case XCB_EXPOSE: {
                //xcb_expose_event_t* expose_ev = (xcb_expose_event_t*) ev;
                puts("EXPOSE event handler: press ESC to exit");
                break;
            }

            case XCB_KEY_RELEASE: {
                xcb_key_release_event_t* key_ev = (xcb_key_release_event_t*) ev;
                
                if(key_ev->detail == 9) {
                    free(ev);
                    return;
                } // ESC
                
                break;
            }

            case XCB_KEY_PRESS: {
                xcb_key_press_event_t* key_ev = (xcb_key_press_event_t*) ev;
                
                if(key_ev->detail == 9) {
                    free(ev);
                    return;
                } // ESC

                break;
            }

        }

        free(ev);
    };
}
