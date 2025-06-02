/* Compile using: gcc convert-simple.c `pkg-config cahute --cflags --libs`. */

#include <stdint.h>
#include <cahute.h>

/* Example buffer to convert. */
static uint16_t example[] = {
    '\\',
    '\\',
    'f',
    'l',
    's',
    '0',
    '\\',
    'a',
    'n',
    'g',
    0xCE,
    '.',
    't',
    'x',
    't'
};

int main(void) {
    cahute_context *context;
    char buf[128];
    cahute_u8 *dest = buf;
    size_t dest_size = sizeof(buf);
    void const *source = example;
    size_t source_size = sizeof(example);
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

    err = cahute_convert_text(
        context,
        (void **)&dest,
        &dest_size,
        &source,
        &source_size,
        CAHUTE_TEXT_ENCODING_UTF8,
        CAHUTE_TEXT_ENCODING_9860_16_HOST
    );
    if ((!err || err == CAHUTE_ERROR_TERMINATED) && !dest_size) {
        /* We need enough space to add a terminating zero here. */
        err = CAHUTE_ERROR_SIZE;
    }

    if (err) {
        printf("Conversion has failed: error 0x%04X has occurred.\n", err);
        goto fail;
    }

    *dest = 0;

    printf("Result: %s\n", buf);
    ret = 0;

fail:
    cahute_destroy_context(context);
    return 0;
}
