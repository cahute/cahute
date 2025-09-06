/* Compile using: gcc detect-usb.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

int my_callback(void *cookie, cahute_usb_detection_entry const *entry) {
    char const *type_name;

    switch (entry->cahute_usb_detection_entry_type) {
    case CAHUTE_USB_DETECTION_ENTRY_TYPE_SERIAL:
        type_name =
            "Serial calculator (fx-9860G, Classpad 300 / 330 (+) or "
            "compatible)";
        break;

    case CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI:
        type_name = "UMS calculator (fx-CG, fx-CP400+, fx-GIII)";
        break;

    default:
        type_name = "(unknown)";
    }

    printf("New entry data:\n");
    printf("- Name or path: %s\n", entry->cahute_usb_detection_entry_name);
    printf("- Type: %s\n", type_name);

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

    err = cahute_detect_usb(context, &my_callback, NULL);
    if (err)
        fprintf(stderr, "Cahute has returned error 0x%04X.\n", err);

    cahute_destroy_context(context);
    return 0;
}
