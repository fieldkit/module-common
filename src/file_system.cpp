#include "debug.h"
#include "hardware.h"
#include "file_system.h"
#include "file_cursors.h"
#include "rtc.h"

using namespace phylum;

namespace fk {

static Files *global_files{ nullptr };

constexpr const char Log[] = "FileSystem";

extern "C" {

static uint32_t log_uptime() {
    return clock.getTime();
}

static size_t debug_write_log(const LogMessage *m, const char *formatted, void *arg) {
    if (m->level == (uint8_t)LogLevels::TRACE) {
        return 0;
    }

    EmptyPool empty;
    DataLogMessage dlm{ m, empty };
    uint8_t buffer[dlm.calculateSize()];
    auto stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode_delimited(&stream, fk_data_DataRecord_fields, dlm.forEncode())) {
        log_uart_get()->println("Unable to encode log message");
        return 0;
    }

    int32_t bytes = stream.bytes_written;
    if (global_files->log().write(buffer, bytes, true) != bytes) {
        log_uart_get()->println("Unable to append log");
        return 0;
    }

    return 0;
}

}

FileSystem::FileSystem(TwoWireBus &bus, Pool &pool) : data_{ bus, files_, pool }, replies_{ *this } {
}

bool FileSystem::format() {
    if (!fs_.format(files_.descriptors_)) {
        logf(LogLevels::ERROR, Log, "Format failed!");
        return false;
    }

    if (!fs_.mount(files_.descriptors_)) {
        logf(LogLevels::ERROR, Log, "Mount failed!");
        return false;
    }

    logf(LogLevels::INFO, Log, "Formatted!");

    return true;
}

bool FileSystem::setup() {
    if (!storage_.initialize(g_, Hardware::SD_PIN_CS)) {
        logf(LogLevels::ERROR, Log, "Unable to initialize SD.");
        return false;
    }

    if (!storage_.open()) {
        logf(LogLevels::ERROR, Log, "Unable to open SD.");
        return false;
    }

    if (!fs_.mount(files_.descriptors_)) {
        logf(LogLevels::ERROR, Log, "Mount failed!");

        if (!format()) {
            return false;
        }
    }

    logf(LogLevels::INFO, Log, "Mounted");

    auto startup = fs_.open(files_.file_log_startup_fd, OpenMode::Write);
    if (!startup) {
        return false;
    }

    files_.log_ = startup;

    files_.data_ = fs_.open(files_.file_data_fk, OpenMode::Write);
    if (!files_.data_) {
        return false;
    }

    log_configure_time(fk_uptime, log_uptime);
    log_add_hook(debug_write_log, nullptr);
    log_configure_hook(true);

    return true;
}

bool FileSystem::beginFileCopy(FileCopySettings settings) {
    auto fd = files_.descriptors_[(size_t)settings.file];

    logf(LogLevels::INFO, Log, "Prepare: id=%d name=%s offset=%lu length=%lu",
         (size_t)settings.file, fd->name, settings.offset, settings.length);

    files_.opened_ = fs_.open(*fd, OpenMode::Read);
    if (!files_.opened_) {
        return false;
    }

    auto newReader = FileReader{ &files_.opened_ };
    if (!files_.fileCopy_.prepare(newReader, settings)) {
        return false;
    }

    return true;
}

bool FileSystem::flush() {
    if (!files_.log_.flush()) {
        return false;
    }

    if (!files_.data_.flush()) {
        return false;
    }

    return true;
}

phylum::SimpleFile FileSystem::openSystem(phylum::OpenMode mode) {
    return fs_.open(files_.file_system_area_fd, mode);
}

Files::Files(phylum::FileOpener &files) : files_(&files) {
    global_files = this;
}

phylum::SimpleFile &Files::log() {
    return log_;
}

phylum::SimpleFile &Files::data() {
    return data_;
}

FileCopyOperation &Files::fileCopy() {
    return fileCopy_;
}

FileCopyOperation::FileCopyOperation() {
}

bool FileCopyOperation::prepare(const FileReader &reader, const FileCopySettings &settings) {
    reader_ = reader;

    streamCopier_.restart();

    if (!reader_.open(settings.offset, settings.length)) {
        return false;
    }

    started_ = 0;
    busy_ = true;
    copied_ = 0;
    lastStatus_ = fk_uptime();
    total_ = reader_.size() - reader_.tell();

    return true;
}

bool FileCopyOperation::copy(lws::Writer &writer) {
    if (started_ == 0) {
        started_ = fk_uptime();
    }

    auto started = fk_uptime();
    while (fk_uptime() - started < FileCopyMaximumElapsed) {
        if (reader_.isFinished()) {
            status();
            busy_ = false;
            return true;
        }
        auto bytes = streamCopier_.copy(reader_, writer);
        if (bytes == 0) {
            break;
        }
        if (bytes > 0) {
            copied_ += bytes;
        }
        if (bytes == lws::Stream::EOS) {
            status();
            busy_ = false;
            return false;
        }
        if (fk_uptime() - lastStatus_ > FileCopyStatusInterval) {
            status();
            lastStatus_ = fk_uptime();
        }
    }

    return true;
}

void FileCopyOperation::status() {
    auto elapsed = fk_uptime() - started_;
    auto complete = copied_ > 0 ? ((float)copied_ / total_) * 100.0f : 0.0f;
    auto speed = copied_ > 0 ? copied_ / ((float)elapsed / 1000.0f) : 0.0f;
    logf(LogLevels::TRACE, "Copy", "%lu/%lu %lums %.2f %.2fbps (%lu)",
         copied_, total_, elapsed, complete, speed, fk_uptime() - lastStatus_);
}

}
