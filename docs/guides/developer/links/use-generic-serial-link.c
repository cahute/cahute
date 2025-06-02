/* Compile using: gcc use-generic-serial-link.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

int main(void) {
    cahute_context *context;
    cahute_link *link = NULL;
    cahute_u8 buf[2];
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

    err = cahute_open_serial_link(
        context,
        &link,
        CAHUTE_SERIAL_PROTOCOL_NONE,
        "/dev/ttyUSB0",
        0
    );
    if (err) {
        fprintf(
            stderr,
            "cahute_open_serial_link() has returned %s.\n",
            cahute_get_error_name(err)
        );
        goto fail;
    }

    buf[0] = 'A';
    buf[1] = 'B';

    err = cahute_send_on_link(link, buf, 2);
    if (err) {
        fprintf(
            stderr,
            "cahute_send_on_link() has returned %s.\n",
            cahute_get_error_name(err)
        );
        goto fail;
    }

    err = cahute_receive_on_link(link, buf, 2, 0, 0);
    if (err) {
        fprintf(
            stderr,
            "cahute_receive_on_link() has returned %s.\n",
            cahute_get_error_name(err)
        );
        goto fail;
    }

    printf("Received characters are the following: %c%c\n", buf[0], buf[1]);

    ret = 0;

fail:
    if (link)
        cahute_close_link(link);

    cahute_destroy_context(context);
    return 0;
}
