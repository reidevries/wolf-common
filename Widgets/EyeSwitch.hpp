#ifndef WOLF_EYE_SWITCH_HPP_INCLUDED
#define WOLF_EYE_SWITCH_HPP_INCLUDED

#include "NanoSwitch.hpp"
#include "Animation.hpp"
#include "Window.hpp"

START_NAMESPACE_DISTRHO

class EyeSwitch :      public NanoSwitch,
                       public IdleCallback
{
  public:
    explicit EyeSwitch(NanoWidget *widget, Size<uint> size) noexcept;

  protected:
    void draw() override;
    void idleCallback() override;
    void onStateChanged() override;

  private:
    DISTRHO_LEAK_DETECTOR(EyeSwitch)
};

END_NAMESPACE_DISTRHO

#endif
