#include "message_buffer.h"
#include "i2c.h"

namespace fk {

bool MessageBuffer::send(uint8_t address) {
    return i2c_device_send(address, buffer, pos);
}

bool MessageBuffer::receive(uint8_t address) {
    pos = i2c_device_receive(address, buffer, sizeof(buffer));
    return pos > 0;
}

bool MessageBuffer::read(size_t bytes) {
    pos = i2c_device_read(buffer, sizeof(buffer), bytes);
    return pos > 0;
}

bool MessageBuffer::write(ModuleQueryMessage &message) {
    return write(fk_module_WireMessageQuery_fields, message.forEncode());
}

bool MessageBuffer::write(ModuleReplyMessage &message) {
    return write(fk_module_WireMessageReply_fields, message.forEncode());
}

bool MessageBuffer::read(ModuleQueryMessage &message) {
    return read(fk_module_WireMessageQuery_fields, message.forDecode());
}

bool MessageBuffer::read(ModuleReplyMessage &message) {
    return read(fk_module_WireMessageReply_fields, message.forDecode());
}

bool MessageBuffer::write(const pb_field_t *fields, void *src) {
    auto stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode_delimited(&stream, fields, src)) {
        return false;
    }
    pos = stream.bytes_written;
    return true;
}

bool MessageBuffer::read(const pb_field_t *fields, void *src) {
    auto stream = pb_istream_from_buffer(buffer, pos);
    if (!pb_decode_delimited(&stream, fields, src)) {
        return false;
    }
    return true;
}

}