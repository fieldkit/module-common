#include <cstring>
#include <cstdio>

#include "http_response_parser.h"

namespace fk {

constexpr const char *ContentLength = "Content-Length: ";
constexpr const char *ETag = "ETag: ";

#ifdef __unix__
constexpr size_t ContentLengthLength = strlen(ContentLength);
constexpr size_t ETagLength = strlen(ETag);
#else
const size_t ContentLengthLength = strlen(ContentLength);
const size_t ETagLength = strlen(ETag);
#endif

void HttpResponseParser::begin() {
    reading_header_ = true;
    consecutive_nls_ = 0;
    previous_ = 0;
    position_ = 0;
    spaces_seen_ = 0;
    status_code_ = 0;
    buffer_[0] = 0;
    etag_[0] = 0;
}

void HttpResponseParser::write(uint8_t c) {
    if (status_code_ == 0) {
        if (spaces_seen_ < 2) {
            if (c == ' ') {
                spaces_seen_++;
                if (spaces_seen_ == 2) {
                    status_code_ = atoi(buffer_);
                }
                buffer_[0] = 0;
                position_ = 0;
            } else {
                if (position_ < BufferSize - 1) {
                    buffer_[position_++] = c;
                    buffer_[position_] = 0;
                }
            }
        }
    }
    else {
        if (c == '\n' || c == '\r') {
            if (c == '\n' && (previous_ == '\r' || previous_ == '\n')) {
                consecutive_nls_++;
                if (consecutive_nls_ == 2) {
                    reading_header_ = false;
                }
            }

            if (strncasecmp(buffer_, ContentLength, ContentLengthLength) == 0) {
                content_length_ = atoi(buffer_ + ContentLengthLength);
            }

            if (strncasecmp(buffer_, ETag, ETagLength) == 0) {
                memcpy(etag_, buffer_ + ETagLength, position_ - ETagLength);
                etag_[position_ - ETagLength] = 0;
            }

            buffer_[0] = 0;
            position_ = 0;
        }
        else {
            consecutive_nls_ = 0;

            if (position_ < BufferSize - 1) {
                buffer_[position_++] = c;
                buffer_[position_] = 0;
            }
        }

        previous_ = c;
    }
}

}
