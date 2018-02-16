#ifndef FK_APP_MESSAGES_H_INCLUDED
#define FK_APP_MESSAGES_H_INCLUDED

#include <fk-app-protocol.h>

#include "debug.h"
#include "pool.h"
#include "protobuf.h"

namespace fk {

class AppQueryMessage {
private:
    fk_app_WireMessageQuery message = fk_app_WireMessageQuery_init_default;
    pb_array_t networksArray;
    Pool *pool;

public:
    AppQueryMessage(Pool *pool) : pool(pool) {
    }

    void clear() {
        message = fk_app_WireMessageQuery_init_default;
    }

    pb_data_t *getDownloadToken() {
        if (message.downloadFile.token.arg != pool) {
            return (pb_data_t *)message.downloadFile.token.arg;
        }
        if (message.downloadDataSet.token.arg != pool) {
            return (pb_data_t *)message.downloadDataSet.token.arg;
        }
        return nullptr;
    }

    fk_app_WireMessageQuery *forDecode() {
        message.downloadFile.token.funcs.decode = pb_decode_data;
        message.downloadFile.token.arg = (void *)pool;

        message.downloadDataSet.token.funcs.decode = pb_decode_data;
        message.downloadDataSet.token.arg = (void *)pool;

        networksArray = {
            .length = 0,
            .itemSize = sizeof(fk_app_NetworkInfo),
            .buffer = nullptr,
            .fields = fk_app_NetworkInfo_fields,
            .pool = pool,
        };

        message.networkSettings.networks.funcs.decode = pb_decode_array;
        message.networkSettings.networks.arg = (void *)&networksArray;

        message.identity.device.funcs.decode = pb_decode_string;
        message.identity.device.arg = (void *)pool;
        message.identity.stream.funcs.decode = pb_decode_string;
        message.identity.stream.arg = (void *)pool;

        message.module.message.funcs.decode = pb_decode_data;
        message.module.message.arg = (void *)pool;

        return &message;
    }

    fk_app_WireMessageQuery *forEncode() {
        return &message;
    }

    fk_app_WireMessageQuery &m() {
        return message;
    }

};

class AppReplyMessage {
private:
    fk_app_WireMessageReply message = fk_app_WireMessageReply_init_default;
    Pool *pool;

public:
    AppReplyMessage(Pool *pool) : pool(pool) {
    }

    void clear() {
        message = fk_app_WireMessageReply_init_default;
    }

    fk_app_WireMessageReply *forDecode() {
        return &message;
    }

    fk_app_WireMessageReply *forEncode() {
        if (message.fileData.token.arg != nullptr) {
            message.fileData.token.funcs.encode = pb_encode_data;
        }
        if (message.dataSetData.token.arg != nullptr) {
            message.dataSetData.token.funcs.encode = pb_encode_data;
        }
        if (message.fileData.data.arg != nullptr) {
            message.fileData.data.funcs.encode = pb_encode_data;
        }
        if (message.identity.device.arg != nullptr) {
            message.identity.device.funcs.encode = pb_encode_string;
        }
        if (message.identity.stream.arg != nullptr) {
            message.identity.stream.funcs.encode = pb_encode_string;
        }
        if (message.module.message.arg != nullptr) {
            message.module.message.funcs.encode = pb_encode_data;
        }
        return &message;
    }

    fk_app_WireMessageReply &m() {
        return message;
    }

public:
    void busy(const char *text);
    void error(const char *text);

};

}

#endif
