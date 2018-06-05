#include "EyeSwitch.hpp"

START_NAMESPACE_DISTRHO

EyeSwitch::EyeSwitch(NanoWidget *widget, Size<uint> size) noexcept : NanoSwitch(widget, size),
{
}

void RemoveDCSwitch::idleCallback()
{

}

void RemoveDCSwitch::onStateChanged()
{

}

void RemoveDCSwitch::draw()
{
	const float width = getWidth();
	const float height = getHeight();

	beginPath();

	fillColor(Color(255,0,0,255));
	rect(0,0, width, height);
	fill();

	closePath();
}

END_NAMESPACE_DISTRHO
