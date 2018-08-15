#include "firmware_storage.h"

namespace fk {

constexpr const char LogName[] = "Firmware";

using Logger = SimpleLog<LogName>;

FirmwareStorage::FirmwareStorage(FlashState<PersistedState> &flashState, SerialFlashFileSystem &fs): flashState_(&flashState), fs_(&fs) {
}

lws::Writer *FirmwareStorage::write() {
    opened_ = fs_->files().open({ }, phylum::OpenMode::Write);

    if (!opened_.format()) {
        // NOTE: Just return a /dev/null writer?
        fk_assert(false);
        return nullptr;
    }

    writer_ = FileWriter{ opened_ };

    return &writer_;
}

lws::Reader *FirmwareStorage::read(FirmwareBank bank) {
    opened_ = fs_->files().open({ }, phylum::OpenMode::Read);

    return nullptr;
}

bool FirmwareStorage::backup() {
    firmware_header_t header;
    if (this->header(FirmwareBank::CoreGood, header)) {
        if (header.version != FIRMWARE_VERSION_INVALID) {
            Logger::info("Have: '%s' (%lu bytes)", header.etag, header.size);
            return true;
        }
    }

    auto writer = write();
    header.version = 1;
    header.time = clock.getTime();
    header.size = 256 * 1024 - 2048;
    header.etag[0] = 0;

    auto headerBytes = writer->write((uint8_t *)&header, sizeof(firmware_header_t));
    if (headerBytes != sizeof(firmware_header_t)) {
        Logger::error("Writing header failed.");
        return false;
    }

    Logger::info("Saving existing firmware (%p)", (void *)FIRMWARE_NVM_PROGRAM_ADDRESS);

    uint32_t bytes = 0;
    uint8_t *ptr = (uint8_t *)FIRMWARE_NVM_PROGRAM_ADDRESS;

    while (bytes < header.size) {
        auto written = writer->write(ptr, 1024);
        if (written == 0) {
            break;
        }
        bytes += written;
    }

    if (bytes == header.size) {
        Logger::info("Done, filling bank.");
        if (!update(FirmwareBank::CoreGood, writer, "")) {
            Logger::error("Error");
            return false;
        }
    }

    return true;
}

bool FirmwareStorage::header(FirmwareBank bank, firmware_header_t &header) {
    header.version = FIRMWARE_VERSION_INVALID;

    auto addr = flashState_->state().firmwares.banks[(int32_t)bank];
    if (!addr.valid()) {
        Logger::info("Bank %d: address is invalid.", bank);
        return false;
    }

    auto file = fs_->files().open(addr, phylum::OpenMode::Read);
    if (!file.exists()) {
        Logger::info("Bank %d: file missing (%lu:%lu).", bank, addr.block, addr.position);
        return false;
    }

    file.seek(UINT64_MAX);

    file.seek(0);

    Logger::info("Bank %d: (%lu:%lu) %lu bytes", bank, addr.block, addr.position, (uint32_t)file.size());

    auto bytes = file.read((uint8_t *)&header, sizeof(firmware_header_t));
    if (bytes != sizeof(firmware_header_t)) {
        Logger::error("Bank %d: Read header failed", bank);
        return false;
    }

    return true;
}

bool FirmwareStorage::update(FirmwareBank bank, lws::Writer *writer, const char *etag) {
    auto beg = opened_.beginning();
    auto head = opened_.head();

    Logger::info("Bank %d: Saving (size=%lu) (beg=%lu:%lu, head=%lu:%lu)", bank,
                 (uint32_t)opened_.size(), beg.block, beg.position, head.block, head.position);

    writer->close();

    opened_.close();

    auto previousAddr = flashState_->state().firmwares.banks[(int32_t)bank];
    flashState_->state().firmwares.banks[(int32_t)bank] = beg;

    if (!flashState_->save()) {
        Logger::error("Error saving block");
    }

    if (previousAddr.valid()) {
        auto previousFile = fs_->files().open(previousAddr, phylum::OpenMode::Write);
        if (previousFile.exists()) {
            previousFile.erase_all_blocks();
        }
    }

    return true;
}

}
