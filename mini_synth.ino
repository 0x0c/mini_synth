#include <MIDICode.h>

#include "mini_synth.h"

using namespace m2d;
using namespace bongorian;
mini_synth::Device synth;

void setup()
{
	Serial.begin(9600);
	Serial.println("Start");
	synth.begin();
	synth.encoder.setPosition(3);
	synth.logger.print("Hello mini_synth");
	synth.logger.print("\n");
}

void loop()
{
	synth.update();

	if (synth.encoder.changed()) {
		auto value = MIDI::safe_octave(synth.encoder.position());
		synth.logger.print("octave: ");
		synth.logger.print(value);
		synth.logger.print("\n");
	}

	bool silent = true;
	for (int i = 0; i < synth.numberOfKeys(); i++) {
		auto state = synth.keyState(i);
		if (state & mini_synth::Key::State::pressed) {
			// if (synth.isKeyPressed(i)) {
			int note = MIDI::shift_octave(i, synth.encoder.position());
			synth.play(note);
			silent = false;
		}
	}

	if (silent) {
		synth.mute();
	}

	if (synth.isModeButtonPressed()) {
		synth.reset();
		synth.logger.print("Reset");
		synth.logger.print("\n");
	}
}
