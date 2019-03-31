#include "mini_synth/mini_synth.h"

using namespace m2d;
using namespace bongorian;
mini_synth::Device synth;
mini_synth::Mode::ModeImpl *mode;

void setup()
{
	Serial.begin(9600);
	Serial.println("Start");

	synth.begin();
	synth.encoder.setMaxValue(6);
	synth.encoder.setMinValue(-1);
	synth.encoder.setPosition(3);
	synth.logger.print("Hello mini_synth");
	synth.logger.print("\n");

	mode = new mini_synth::Mode::Keyboard(&synth);
}

void loop()
{
	mode->update();
	mode->display();
}
