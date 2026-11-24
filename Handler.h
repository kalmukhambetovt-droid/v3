#pragma once
#include <memory>
#include <string>
#include <utility>
#include "IGameFacade.h"

class IGameFacade;

class Handler {
protected:
    std::shared_ptr<Handler> next_;
    IGameFacade& facade_;

    bool passToNext(const std::string& cmd);
    bool reportUsage(const std::string& problem, const std::string& usage);

public:
    explicit Handler(IGameFacade& facade);
    virtual ~Handler() = default;
    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;

    std::shared_ptr<Handler> setNext(std::shared_ptr<Handler> next);
    virtual bool handle(const std::string& cmd) = 0;
};
inline Handler::Handler(IGameFacade& facade) : next_(), facade_(facade) {
}

inline std::shared_ptr<Handler> Handler::setNext(std::shared_ptr<Handler> next) {
    next_ = std::move(next);
    return next_;
}

inline bool Handler::passToNext(const std::string& cmd) {
    if (!next_) return false;
    return next_->handle(cmd);
}

inline bool Handler::reportUsage(const std::string& problem, const std::string& usage) {
    facade_.printMessage("[Ошибка ввода] " + problem + ". Формат: " + usage);
    return true;
}
