#pragma once

#include <vector>
#include "Screen.h"
#include "../TFT/Display.h"

class NavigationStack
{
  private:
    Display *m_tft;
  public:
    std::vector<Screen *> stack;
    NavigationStack(Display *tft) : m_tft(tft) {}
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
    void updateKey(SpecKeys key, uint8_t state) {
      Screen *top = getTop();
      if (top) {
        top->updateKey(key, state);
      }
    };
    void pressKey(SpecKeys key) {
      #ifdef ENABLE_FRAMEBUFFER
      if (key == SPECKEY_MENU) {
        m_tft->saveScreenshot();
        return;
      }
      #endif
      Screen *top = getTop();
      if (top) {
        top->pressKey(key);
      }
    };
};
