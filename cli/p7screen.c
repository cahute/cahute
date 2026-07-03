/* ****************************************************************************
 * Copyright (C) 2016-2017, 2024 Thomas Touhey <thomas@touhey.fr>
 *
 * This software is governed by the CeCILL 2.1 license under French law and
 * abiding by the rules of distribution of free software. You can use, modify
 * and/or redistribute the software under the terms of the CeCILL 2.1 license
 * as circulated by CEA, CNRS and INRIA at the following
 * URL: https://cecill.info
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL 2.1 license and that you accept its terms.
 * ************************************************************************* */

#include "p7screen.h"
#include <string.h>
#include <SDL3/SDL.h>

#define FRAME_TIMEOUT_MS 400 /* Timeout for a frame to be received. */

/**
 * Display cookie.
 *
 * @property context Context in which to run the loop.
 * @property window Window that contains the surface.
 * @property renderer Renderer for the window.
 * @property texture Texture that covers the window.
 * @property saved_width Saved width from when the window was first opened.
 * @property saved_height Saved height from when the window was first opened.
 * @property zoom Zoom with which to draw the window.
 */
struct display_cookie {
    cahute_context *context;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int saved_width;
    int saved_height;
    int zoom;
};

static char const error_notfound[] =
    "Could not connect to the calculator.\n"
    "- Is it plugged in and in PROJ mode?\n"
    "- Have you tried unplugging, plugging and selecting Projector on "
    "pop-up?\n"
    "- Have you tried changing the cable?\n";

static char const error_toomany[] =
    "Too many calculators connected by USB, please only have one connected.\n";

static char const error_noaccess[] =
    "Could not get access to the calculator.\n"
    "Install the appropriate udev rule, or run as root.\n";

static char const error_unplanned[] =
    "The calculator didn't act as planned.\n"
    "Stop receive mode on calculator and start it again before re-running "
    "p7screen.\n";

/**
 * Scale up a picture from the original size using a zoom.
 *
 * This function is made so as the products are not made in the loop itself,
 * but in the initialisation.
 *
 * @param pixels Pixels in which the source data is, and the scaled up image
 *        should be written to.
 * @param width Width of the original image.
 * @param height Height of the original image.
 * @param zoom Zoom to apply to the original image.
 */
static void scale_up_picture(Uint32 *pixels, int width, int height, int zoom) {
    int new_row_size = width * zoom;
    int new_line_size = new_row_size * zoom;
    int ozx, ozy, zx, zy;
    int src_offset = width * height - 1;

    for (ozy = height * new_line_size; ozy;) {
        ozy -= new_line_size;

        for (ozx = ozy + new_row_size; ozx > ozy;) {
            ozx -= zoom;

            for (zx = ozx + zoom; zx > ozx;)
                pixels[--zx] = pixels[src_offset];

            src_offset--;
        }

        /* NOTE: memcpy() takes amounts of bytes, not Uint32, so we need to
         * multiply the line size by 4 (by shifting it 2 bytes to the left). */
        for (zy = ozy + new_line_size - new_row_size; zy > ozy;
             zy -= new_row_size)
            memcpy(&pixels[zy], &pixels[ozy], new_row_size << 2);
    }
}

/**
 * Callback to display the screen frame.
 *
 * @param cookie Display cookie.
 * @param frame Frame to display.
 * @return Whether we want to interrupt the flow.
 */
static int
display_frame(struct display_cookie *cookie, cahute_frame const *frame) {
    int width, height, format, zoom, err;

    width = frame->cahute_frame_width;
    height = frame->cahute_frame_height;
    format = frame->cahute_frame_format;
    zoom = cookie->zoom;

    if (format != CAHUTE_PICTURE_FORMAT_1BIT_MONO
        && format != CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50
        && format != CAHUTE_PICTURE_FORMAT_1BIT_DUAL
        && format != CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50
        && format != CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED
        && format != CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5) {
        fprintf(stderr, "Unsupported format %d.\n", format);
        return 1;
    }

    if (!cookie->window) {
        /* We haven't got a window, our objective is to create one,
         * with a renderer and a texture. */
        cookie->window =
            SDL_CreateWindow("p7screen", width * zoom, height * zoom, 0);
        if (!cookie->window) {
            fprintf(
                stderr,
                "Couldn't create the window: %s\n",
                SDL_GetError()
            );
            return 1;
        }

        cookie->renderer = SDL_CreateRenderer(cookie->window, NULL);
        if (!cookie->renderer) {
            fprintf(
                stderr,
                "Couldn't create the renderer: %s\n",
                SDL_GetError()
            );
            return 1;
        }

        /* The texture is created as a classic ARGB pixel matrix (8 bits per
         * component). */
        cookie->texture = SDL_CreateTexture(
            cookie->renderer,
            SDL_PIXELFORMAT_XRGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            width * zoom,
            height * zoom
        );
        if (!cookie->texture) {
            fprintf(
                stderr,
                "Couldn't create the texture: %s\n",
                SDL_GetError()
            );
            return 1;
        }

        cookie->saved_width = width;
        cookie->saved_height = height;
    } else if (cookie->saved_width != width || cookie->saved_height != height) {
        /* The dimensions have changed somehow, we don't support this. */
        fprintf(stderr, "Unmanaged dimensions changed.\n");
        return 1;
    }

    /* Copy the data. */
    {
        Uint32 *texture_pixels;
        int pitch;

        if (!SDL_LockTexture(
                cookie->texture,
                NULL,
                (void **)&texture_pixels,
                &pitch
            )) {
            fprintf(stderr, "Couldn't lock the texture: %s\n", SDL_GetError());
            return 1;
        }

        err = cahute_convert_picture_from_frame(
            cookie->context,
            texture_pixels,
            CAHUTE_PICTURE_FORMAT_32BIT_ARGB_HOST,
            frame
        );
        if (err) {
            fprintf(
                stderr,
                "Couldn't convert the picture from frame (%s).\n",
                cahute_get_error_name(err)
            );
            return 1;
        }

        if (zoom > 1)
            scale_up_picture(
                texture_pixels,
                frame->cahute_frame_width,
                frame->cahute_frame_height,
                cookie->zoom
            );

        SDL_UnlockTexture(cookie->texture);
    }

    if (!SDL_RenderTexture(cookie->renderer, cookie->texture, NULL, NULL)) {
        fprintf(
            stderr,
            "Couldn't render the texture on the renderer: %s\n",
            SDL_GetError()
        );
        return 1;
    }

    if (!SDL_RenderPresent(cookie->renderer)) {
        fprintf(
            stderr,
            "Couldn't render using the current renderer: %s\n",
            SDL_GetError()
        );
        return 1;
    }

    return 0;
}

/**
 * Main entry point of the program.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @return Exit status code.
 */
int main(int ac, char **av) {
    cahute_context *context = NULL;
    cahute_link *link = NULL;
    struct args args;
    struct display_cookie cookie;
    int err, ret = 1;
    int sdl_initialized = 0;

    if (!parse_args(ac, av, &args))
        return 0;

    cookie.context = NULL;
    cookie.window = NULL;
    cookie.renderer = NULL;
    cookie.texture = NULL;
    cookie.saved_width = -1;
    cookie.saved_height = -1;
    cookie.zoom = args.zoom;

    err = cahute_create_context(&context);
    if (err)
        goto end;

    cookie.context = context;

    if (args.loglevel)
        set_log_level(context, args.loglevel);

    if (args.serial_name)
        /* The user has selected a serial link!
         * On serial links, we can either use automatic protocol detection
         * with the risk that we have a data protocol, or we can choose a
         * specific protocol but not support both CASIOLINK screen captures
         * and Protocol 7.00 Screenstreaming. We choose the former. */
        err = cahute_open_serial_link(
            context,
            &link,
            args.serial_flags | CAHUTE_SERIAL_RECEIVER,
            args.serial_name,
            args.serial_speed
        );
    else
        err = cahute_open_simple_usb_link(
            context,
            &link,
            CAHUTE_USB_RECEIVER | CAHUTE_USB_SEVEN | CAHUTE_USB_OHP
        );

    if (err) {
        switch (err) {
        case CAHUTE_ERROR_NOT_FOUND:
            fprintf(stderr, error_notfound);
            break;

        case CAHUTE_ERROR_TOO_MANY:
            fprintf(stderr, error_toomany);
            break;

        case CAHUTE_ERROR_PRIV:
            fprintf(stderr, error_noaccess);
            break;

        default:
            fprintf(stderr, error_unplanned);
            break;
        }

        return 1;
    }

    /* Initialize the SDL. */
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        cahute_close_link(link);
        return 3;
    }
    sdl_initialized = 1;

    while (1) {
        SDL_Event event;
        cahute_frame *frame;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                ret = 0;
                goto end;
            }
        }

        err = cahute_receive_screen(link, &frame, FRAME_TIMEOUT_MS);
        switch (err) {
        case 0:
            /* A frame was received! */
            display_frame(&cookie, frame);
            break;

        case CAHUTE_ERROR_TIMEOUT_START:
            /* No frame was received in the given time. */
            continue;

        case CAHUTE_ERROR_ABORT:
        case CAHUTE_ERROR_GONE:
        case CAHUTE_ERROR_TERMINATED:
            ret = 0;
            goto end;

        default:
            fprintf(stderr, error_unplanned);
            goto end;
        }
    }

end:
    cahute_close_link(link);
    cahute_destroy_context(context);

    if (cookie.texture)
        SDL_DestroyTexture(cookie.texture);
    if (cookie.renderer)
        SDL_DestroyRenderer(cookie.renderer);
    if (cookie.window)
        SDL_DestroyWindow(cookie.window);
    if (sdl_initialized)
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    return ret;
}
