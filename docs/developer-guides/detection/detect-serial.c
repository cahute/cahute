/* Compile using: gcc detect-serial.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

int my_callback(void *cookie, cahute_serial_detection_entry const *entry) {
    printf("New entry data:\n");
    printf("- %s\n", entry->cahute_serial_detection_entry_name);

    return 0;
}

int main(void) {
    cahute_context *context;
    int err;

    err = cahute_create_context(&context);
    if (err) {
        fprintf(
            stderr,
            "cahute_create_context() has returned error %s.\n",
            cahute_get_error_name(err)
        );
        return 1;
    }

    err = cahute_detect_serial(context, &my_callback, NULL);
    if (err)
        fprintf(stderr, "Cahute has returned error 0x%04X.\n", err);

    cahute_destroy_context(context);
    return 0;
}
