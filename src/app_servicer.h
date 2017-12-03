#ifndef FK_APP_SERVICER_H_INCLUDED
#define FK_APP_SERVICER_H_INCLUDED

#include "active_object.h"
#include "module_controller.h"
#include "app_messages.h"

namespace fk {

class AppServicer : public Task {
private:
    AppQueryMessage query;
    ModuleController *modules;
    Pool *pool;

public:
    AppServicer(ModuleController &modules, Pool &pool);

public:
    TaskEval &task() override;

    bool read(MessageBuffer &buffer);
    void handle(AppQueryMessage &query);

};

}

#endif
