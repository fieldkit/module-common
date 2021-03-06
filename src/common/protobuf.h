#ifndef FK_PROTOBUF_H_INCLUDED
#define FK_PROTOBUF_H_INCLUDED

#include <pb_decode.h>
#include <pb_encode.h>

#include "pool.h"

namespace fk {

constexpr size_t ProtoBufEncodeOverhead = 16;

struct pb_array_t;

typedef bool (*pb_decode_array_item)(pb_istream_t *stream, pb_array_t *array);

bool pb_encode_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg);

bool pb_decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg);

typedef struct pb_array_t {
    size_t length;
    size_t itemSize;
    void *buffer;
    const pb_msgdesc_t *fields;
    pb_decode_array_item decode_item_fn;
    Pool *pool;
} pb_array_t;

bool pb_encode_array(pb_ostream_t *stream, const pb_field_t *field, void *const *arg);

bool pb_encode_uint32_array(pb_ostream_t *stream, const pb_field_t *field, void *const *arg);

bool pb_decode_array(pb_istream_t *stream, const pb_field_t *field, void **arg);

typedef struct pb_data_t {
    size_t length;
    const void *buffer;
} pb_data_t;

pb_data_t *pb_data_allocate(Pool *pool, size_t size);

bool pb_encode_data(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

bool pb_decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg);

}

#endif
