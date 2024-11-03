#pragma once

#include <iostream>
#include <functional>

class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> func) : func_(std::move(func)), active_(true) {}

    // Disable copy to prevent multiple executions.
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    // Allow move semantics if necessary.
    ScopeGuard(ScopeGuard&& other) noexcept : func_(std::move(other.func_)), active_(other.active_) {
        other.active_ = false;
    }

    ~ScopeGuard() {
        if (active_) func_();  // Execute stored function at end of scope
    }

    void dismiss() { active_ = false; }  // Optionally disable guard before scope end

private:
    std::function<void()> func_;
    bool active_;
};
