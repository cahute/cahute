/* Compile using: gcc open-usb-link.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

int main(void) {
    cahute_context *context;
    cahute_link *link = NULL;
    int err, ret = 1;

    err = cahute_create_context(&context);
    if (err) {
        fprintf(
            stderr,
            "cahute_create_context() has returned error %s.\n",
            cahute_get_error_name(err)
        );
        return 1;
    }

    err = cahute_open_simple_usb_link(context, &link, 0);
    if (err) {
        fprintf(
            stderr,
            "cahute_open_simple_usb_link() has returned error 0x%04X.\n",
            err
        );
        goto fail;
    }

    /* Profit! */
    printf("Link successfully opened!\n");

    ret = 0;
fail:
    if (link)
        cahute_close_link(link);

    cahute_destroy_context(context);
    return ret;
}
