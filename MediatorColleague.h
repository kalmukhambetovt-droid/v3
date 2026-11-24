#pragma once
#include "IMediator.h"
#include <memory>
#include <stdexcept>

class MediatorColleague {
private:
    std::weak_ptr<IMediator> mediator;

protected:
    std::shared_ptr<IMediator> lockMediator() const {
        std::shared_ptr<IMediator> locked = mediator.lock();
        if (!locked) throw std::logic_error("Компонент не подключён к посреднику");
        return locked;
    }

public:
    virtual ~MediatorColleague() = default;

    void setMediator(const std::weak_ptr<IMediator>& gameMediator) { mediator = gameMediator; }
    bool hasMediator() const { return !mediator.expired(); }
};
