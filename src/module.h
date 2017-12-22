#ifndef FK_MODULE_H_INCLUDED
#define FK_MODULE_H_INCLUDED

#include "active_object.h"
#include "module_messages.h"
#include "module_servicer.h"

namespace fk {

class Module : public ActiveObject, public ModuleCallbacks {
private:
    Pool replyPool;
    MessageBuffer outgoing;
    MessageBuffer incoming;
    ModuleServicer moduleServicer;
    ModuleInfo *info;

public:
    static Module *active;

public:
    Module(ModuleInfo &info);

public:
    void begin();
    void resume();
    void receive(size_t bytes);
    void reply();

public:
    ModuleReadingStatus beginReading(SensorReading *readings) override;
    ModuleReadingStatus readingStatus(SensorReading *readings) override;

};

}

#endif
