#ifndef RESPONSE_BUFFER_H
#define RESPONSE_BUFFER_H
#include <stddef.h>
struct response_buffer {
    char data[4096];
    size_t used;
    int error;
};
int response_buffer_append(struct response_buffer *buffer, const char *data, size_t length);
#endif
