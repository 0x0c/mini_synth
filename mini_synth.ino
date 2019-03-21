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

	if (synth.encoder.available()) {
		auto value = MIDI::safe_octave(synth.encoder.position() - 1);
		synth.logger.print("octave: ");
		synth.logger.print(value);
		synth.logger.print("\n");
	}

	bool silent = true;
	for (int row = 0; row < synth.numberOfRows; row++) {
		for (int column = 0; column < synth.numberOfColumns; column++) {
			int index = row * 4 + column;
			if (synth.isPressed(row, column)) {
				int note = MIDI::shift(index, synth.encoder.position());
				synth.play(note);
				if (synth.isLongPress(row, column) == false) {
					synth.logger.print(MIDI::to_string(note).c_str());
					synth.logger.print("\n");
				}
				silent = false;
			}
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
