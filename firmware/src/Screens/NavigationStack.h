#pragma once

#include <vector>
#include "Screen.h"

class NavigationStack
{
  public:
    std::vector<Screen *> stack;
    NavigationStack() {}
    ~NavigationStack() {}
    Screen *getTop() {
      if (stack.size() > 0) {
        return stack.back();
      }
      return nullptr;
    }
    void push(Screen *screen) {
      screen->setNavigationStack(this);
      Screen *top = getTop();
      if (top) {
        top->willDisappear();
      }
      stack.push_back(screen);
      screen->didAppear();
    }
    void pop() {
      if (stack.size() <= 1) {
        return;
      }
      Screen *top = getTop();
      if (top) {
        top->willDisappear();
        top->setNavigationStack(nullptr);
        delete top;
        stack.pop_back();
        top = stack.back();
        if (top) {
          top->didAppear();
        }
      }
    }
    void updatekey(SpecKeys key, uint8_t state) {
      Screen *top = getTop();
      if (top) {
        top->updatekey(key, state);
      }
    };
    void pressKey(SpecKeys key) {
      Screen *top = getTop();
      if (top) {
        top->pressKey(key);
      }
    };
};
