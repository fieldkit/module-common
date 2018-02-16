#ifndef FK_MODULE_MESSAGES_H_INCLUDED
#define FK_MODULE_MESSAGES_H_INCLUDED

#include <fk-module-protocol.h>

#include "pool.h"
#include "protobuf.h"

namespace fk {

class ModuleQueryMessage {
private:
    fk_module_WireMessageQuery message = fk_module_WireMessageQuery_init_default;
    Pool *pool;

public:
    ModuleQueryMessage(Pool &pool) : pool(&pool) {
    }

    void clear() {
        message = fk_module_WireMessageQuery_init_default;
    }

    fk_module_WireMessageQuery *forDecode() {
        message.custom.message.funcs.decode = pb_decode_data;
        message.custom.message.arg = pool;
        return &message;
    }

    fk_module_WireMessageQuery *forEncode() {
        message.custom.message.funcs.encode = pb_encode_data;
        return &message;
    }

    fk_module_WireMessageQuery &m() {
        return message;
    }

};

class ModuleReplyMessage {
private:
    fk_module_WireMessageReply message = fk_module_WireMessageReply_init_default;
    Pool *pool;

public:
    ModuleReplyMessage(Pool &pool) : pool(&pool) {
    }

    void clear() {
        message = fk_module_WireMessageReply_init_default;
    }

    fk_module_WireMessageReply *forDecode() {
        message.error.message.funcs.decode = pb_decode_string;
        message.error.message.arg = pool;
        message.capabilities.name.funcs.decode = pb_decode_string;
        message.capabilities.name.arg = pool;
        message.sensorCapabilities.name.funcs.decode = pb_decode_string;
        message.sensorCapabilities.name.arg = pool;
        message.sensorCapabilities.unitOfMeasure.funcs.decode = pb_decode_string;
        message.sensorCapabilities.unitOfMeasure.arg = pool;
        message.custom.message.funcs.decode = pb_decode_data;
        message.custom.message.arg = pool;
        return &message;
    }

    fk_module_WireMessageReply *forEncode() {
        message.error.message.funcs.encode = pb_encode_string;
        message.capabilities.name.funcs.encode = pb_encode_string;
        message.sensorCapabilities.name.funcs.encode = pb_encode_string;
        message.sensorCapabilities.unitOfMeasure.funcs.encode = pb_encode_string;
        message.custom.message.funcs.encode = pb_encode_data;
        return &message;
    }

    fk_module_WireMessageReply &m() {
        return message;
    }

    bool isError() {
        return message.type == fk_module_ReplyType_REPLY_ERROR;
    }

};

}

#endif
