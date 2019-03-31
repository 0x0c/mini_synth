#pragma once

#include "ModeImpl.h"
#include <MIDICode.h>

namespace bongorian
{
namespace mini_synth
{
	namespace Mode
	{
		class Keyboard : public ModeImpl
		{
		public:
			Keyboard(Device *device)
			    : ModeImpl(device)
			{
			}

			void update()
			{
				this->_device->update();

				bool silent = true;
				for (int i = 0; i < this->_device->numberOfKeys(); i++) {
					auto state = this->_device->keyState(i);
					if (state & mini_synth::Key::State::pressed) {
						// if (this->_device->isKeyPressed(i)) {
						auto note = m2d::MIDI::YamahaNote(i, this->_device->encoder.position());
						this->_device->volume(4);
						this->_device->play(note.rawValue());
						silent = false;
					}
				}

				if (silent) {
					this->_device->mute();
				}
			}

			void display()
			{
				if (this->_device->encoder.isValueChanged()) {
					auto value = m2d::MIDI::YamahaNote::safe_octave(this->_device->encoder.position());
					this->_device->logger.print("octave: " + String(value));
					this->_device->logger.print("\n");
				}
			}
		};
	}
}
}