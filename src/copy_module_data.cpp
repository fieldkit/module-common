#include "copy_module_data.h"
#include "watchdog.h"
#include "transmissions.h"
#include "performance.h"

namespace fk {

void CopyModuleData::task() {
    FileCopySettings fileCopySettings{ FileNumber::Data };

    PrepareTransmissionData prepareTransmissionData{
        *services().state,
        *services().fileSystem,
        *services().moduleCommunications,
        fileCopySettings,
    };

    prepareTransmissionData.enqueued();

    services().bus->begin(400000);

    while (simple_task_run(prepareTransmissionData)) {
        services().watchdog->task();
        services().moduleCommunications->task();
    }

    back();
}

}
