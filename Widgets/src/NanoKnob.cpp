#include "NanoKnob.hpp"
#include "Mathf.hpp"

START_NAMESPACE_DISTRHO

NanoKnob::NanoKnob(NanoWidget *widget, Size<uint> size) noexcept
    : WolfWidget(widget),
      fMin(0.0f),
      fMax(1.0f),
      fStep(0.0f),
      fValue(0.5f),
      fUsingLog(false),
      fLeftMouseDown(false),
      fIsHovered(false),
      fColor(Color(255, 0, 0, 255)),
      fCallback(nullptr)
{
    setSize(size);
}

float NanoKnob::getValue() const noexcept
{
    return fValue;
}

void NanoKnob::setRange(float min, float max) noexcept
{
    DISTRHO_SAFE_ASSERT(min < max);

    fMin = min;
    fMax = max;

    fValue = wolf::clamp(fValue, min, max);
}

void NanoKnob::setStep(float step) noexcept
{
    fStep = step;
}

float NanoKnob::getMin() noexcept
{
    return fMin;
}

float NanoKnob::getMax() noexcept
{
    return fMax;
}

// NOTE: value is assumed to be scaled if using log
void NanoKnob::setValue(float value, bool sendCallback) noexcept
{
    value = wolf::clamp(value, fMin, fMax);

    if (d_isEqual(fValue, value))
        return;

    fValue = value;

    if (sendCallback && fCallback != nullptr)
        fCallback->nanoKnobValueChanged(this, fValue);

    repaint();
}

void NanoKnob::setUsingLogScale(bool yesNo) noexcept
{
    fUsingLog = yesNo;
}

void NanoKnob::setCallback(Callback *callback) noexcept
{
    fCallback = callback;
}

void NanoKnob::setColor(Color color) noexcept
{
    fColor = color;
}

Color NanoKnob::getColor() noexcept
{
    return fColor;
}

void NanoKnob::onNanoDisplay()
{
    draw();
}

bool NanoKnob::onMouse(const MouseEvent &ev)
{
    if (ev.button != 1)
        return fLeftMouseDown;

    Window &window = getParentWindow();

    if (!ev.press)
    {
        if (fLeftMouseDown == true)
        {
            fLeftMouseDown = false;
            setFocus(false);

//            window.unclipCursor();
//            window.setCursorPos(this);
//            window.showCursor();
//            getParentWindow().setCursorStyle(Window::CursorStyle::Grab);

            onMouseUp();

            return true;
        }

        return false;
    }

    if (contains(ev.pos))
    {
        fLeftMouseDownLocation = ev.pos;
        fLeftMouseDown = true;
        
        setFocus(true);
//        window.hideCursor();
//        window.clipCursor(Rectangle<int>(getAbsoluteX() + getWidth() / 2.0f, 0, 0, (int)window.getHeight()));

        onMouseDown();

        return true;
    }

    return false;
}

void NanoKnob::onMouseHover()
{
}

void NanoKnob::onMouseLeave()
{
}

void NanoKnob::onMouseUp()
{
}

void NanoKnob::onMouseDown()
{
}

bool NanoKnob::onMotion(const MotionEvent &ev)
{
    if (fLeftMouseDown)
    {
        const float resistance = 1200.0f;
        const float difference = (fLeftMouseDownLocation.getY() - ev.pos.getY()) / resistance * (fMax - fMin);

        Window &window = getParentWindow();
        const int windowHeight = window.getHeight();

        // this doesn't seem right. TODO: investigate mouse cursor manipulation code for off-by-one error
        if (ev.pos.getY() + getAbsoluteY() >= windowHeight - 1)
        {
//            window.setCursorPos(getAbsoluteX(), 2);
            fLeftMouseDownLocation.setY(-getAbsoluteY() + 2);
        }
        else if (ev.pos.getY() + getAbsoluteY() == 0)
        {
//            window.setCursorPos(getAbsoluteX(), windowHeight - 2);
            fLeftMouseDownLocation.setY(windowHeight - getAbsoluteY() - 2);
        }
        else
        {
            fLeftMouseDownLocation.setY(ev.pos.getY());
        }

        setValue(fValue + difference, true);

        return true;
    }

    if (contains(ev.pos))
    {
        if (!fIsHovered)
        {
            fIsHovered = true;
            onMouseHover();
        }
    }
    else
    {
        if (fIsHovered)
        {
            fIsHovered = false;
            onMouseLeave();
        }
    }

    return false;
}

bool NanoKnob::onScroll(const ScrollEvent &ev)
{
    if (!contains(ev.pos))
        return false;

    const float resistance = 80.0f;

    setValue(getValue() + ev.delta.getY() / resistance * (fMax - fMin), true);

    return true;
}

END_NAMESPACE_DISTRHO