#ifndef WOLF_BIPOLAR_MODE_SWITCH_HPP_INCLUDED
#define WOLF_BIPOLAR_MODE_SWITCH_HPP_INCLUDED

#include "NanoSwitch.hpp"

START_NAMESPACE_DISTRHO

class BipolarModeSwitch : public NanoSwitch
{
public:
    explicit BipolarModeSwitch(NanoWidget *widget, Size<uint> size) noexcept;

protected:
    void draw();
    void drawHandle();
    void drawSocket();

    void drawUp() override;
    void drawDown() override;
    
private:
    DISTRHO_LEAK_DETECTOR(BipolarModeSwitch)
};

END_NAMESPACE_DISTRHO

#endif