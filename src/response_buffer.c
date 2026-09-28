#include "response_buffer.h"
#include <errno.h>
#include <string.h>
int response_buffer_append(struct response_buffer *buffer, const char *data, size_t length)
{
    if (buffer->error) {
        return buffer->error;
    }
    if ((!data && length) || length > sizeof(buffer->data) - 1 - buffer->used) {
        buffer->error = -EMSGSIZE;
        return buffer->error;
    }
    if (length) {
        memcpy(buffer->data + buffer->used, data, length);
    }
    buffer->used += length;
    buffer->data[buffer->used] = 0;
    return 0;
}
