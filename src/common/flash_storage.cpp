#include "debug.h"
#include "flash_storage.h"

namespace fk {

bool SerialFlashFileSystem::initialize(uint8_t cs, phylum::sector_index_t sector_size) {
    if (!storage_.initialize(cs, sector_size)) {
        FlashLog::error("Initialize failed (%d)", cs);
        return false;
    }

    auto g = storage_.geometry();
    FlashLog::info("Flash Geometry: (%lu x %lu)", g.number_of_blocks, g.block_size());

    if (!storage_.open()) {
        FlashLog::error("Open failed");
        return false;
    }

    if (!allocator_.initialize()) {
        FlashLog::error("Initialize failed");
        return false;
    }

    return true;
}

bool SerialFlashFileSystem::preallocate() {
    return allocator_.preallocate(0);
}

bool SerialFlashFileSystem::erase() {
    FlashLog::info("Erasing");

    if (!storage_.erase()) {
        return false;
    }

    return true;
}

bool SerialFlashFileSystem::reclaim(FlashStateService &manager) {
    phylum::UnusedBlockReclaimer reclaimer(files_, manager.manager());
    reclaim(reclaimer, manager.minimum());
    return reclaimer.reclaim();
}

bool SerialFlashFileSystem::reclaim(phylum::UnusedBlockReclaimer &reclaimer, MinimumFlashState &state) {
    for (auto i = 0; i < (int32_t)FirmwareBank::NumberOfBanks; ++i) {
        auto addr = state.firmwares.banks[i];
        if (storage_.geometry().valid(addr)) {
            FlashLog::info("Walk (%lu:%lu)", addr.block, addr.position);
            reclaimer.walk(addr);
        }
        else {
            // NOTE: This fixes up some gibberish addresses I introduced in testing.
            state.firmwares.banks[i] = { };
        }
    }

    return true;
}

bool SerialFlashFileSystem::busy(uint32_t elapsed) {
    watchdog_->task();

    return true;
}

}
