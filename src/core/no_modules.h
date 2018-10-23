#ifndef FK_NO_MODULES_H_INCLUDED
#define FK_NO_MODULES_H_INCLUDED

#include "state_services.h"

namespace fk {

class NoModules : public MainServicesState {
private:
    uint32_t entered_{ 0 };
    uint32_t scanned_at_{ 0 };

public:
    const char *name() const override {
        return "NoModules";
    }

public:
    void task() override;
};

}

#endif
