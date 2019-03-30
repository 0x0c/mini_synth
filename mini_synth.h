#pragma once
#include <Arduino_STM32_PSG.h>
#include <U8g2lib.h>
#include <string>
#include <vector>

namespace bongorian
{
namespace mini_synth
{
	class Encoder
	{
	private:
		uint8_t _previousEncoderState = 0;
		int _position = 0;
		bool _valueChanged = false;
		int _phaseA;
		int _phaseB;
		int _swPin;

	public:
		Encoder(int phaseA, int phaseB, int swPin)
		{
			this->_phaseA = phaseA;
			this->_phaseB = phaseB;
			this->_swPin = swPin;
		}

		void setup()
		{
			pinMode(this->_phaseA, INPUT);
			pinMode(this->_phaseB, INPUT);
			pinMode(this->_swPin, INPUT);
		}

		void update()
		{
			if (digitalRead(this->_swPin) == HIGH) {
				uint8_t a = digitalRead(this->_phaseA);
				uint8_t b = digitalRead(this->_phaseB);

				uint8_t ab = (a << 1) | b;
				uint8_t encoded = (this->_previousEncoderState << 2) | ab;

				if (encoded == 0b1101 || encoded == 0b0100 || encoded == 0b0010 || encoded == 0b1011) {
					this->_valueChanged = true;
					this->_position--;
				}
				else if (encoded == 0b1110 || encoded == 0b0111 || encoded == 0b0001 || encoded == 0b1000) {
					this->_valueChanged = true;
					this->_position++;
				}

				this->_previousEncoderState = ab;
			}
		}

		bool valueChanged()
		{
			return this->_valueChanged;
		}

		bool isPressed()
		{
			return digitalRead(this->_swPin) == LOW;
		}

		int setPosition(int position)
		{
			this->_position = position * 2;
		}

		int position()
		{
			this->_valueChanged = false;
			return this->_position / 2;
		}
	};

	class Key
	{
	private:
		int _row;
		int _column;
		bool _pressedState = false;
		bool _previousPressedState = false;
		bool _holdState = false;

	public:
		int identifier = 0;

		typedef enum : int
		{
			invalid = -1,
			release = 0,
			pressed = (1 << 0),
			holding = (1 << 1)
		} State;

		Key(int row, int column)
		{
			this->identifier = row + column;
			this->_row = row;
			this->_column = column;
		}

		void setup()
		{
			pinMode(this->_row, OUTPUT);
			digitalWrite(this->_row, HIGH);
			pinMode(this->_column, INPUT_PULLUP);
		}

		void update()
		{
			this->_previousPressedState = this->_pressedState;
			digitalWrite(this->_row, LOW);
			this->_pressedState = digitalRead(this->_column) == LOW;
			this->_holdState = (this->_pressedState == true) && (this->_previousPressedState == true);
			digitalWrite(this->_row, HIGH);
		}

		Key::State currentState()
		{
			if (this->isLongPress()) {
				return (Key::State)(Key::State::holding | Key::State::pressed);
			}
			if (this->isPressed()) {
				return Key::State::pressed;
			}

			return Key::State::release;
		}

		bool isPressed()
		{
			return this->_pressedState;
		}

		bool isLongPress()
		{
			return this->_holdState;
		}
	};

	class Device
	{
	private:
		const uint8_t _rowIO[3] = { PB3, PB4, PB5 };
		const uint8_t _columnIO[4] = { PB8, PB9, PB0, PB1 };
		SAA1099 _saa = SAA1099(PA3, PA5, PA7, PA0, PA1, PA2);
		const uint8_t _modeSwitchPin = PC13;

		uint8_t *_u8log_buffer;

	public:
		typedef enum : int
		{
			left = 0,
			right = 1,
			both = 2
		} AudioChannel;

		const uint8_t numberOfRows = sizeof(this->_rowIO) / sizeof(this->_rowIO[0]);
		const uint8_t numberOfColumns = sizeof(this->_columnIO) / sizeof(this->_columnIO[0]);
		Encoder encoder = Encoder(PA6, PA8, PA4);
		U8G2_SSD1306_128X64_NONAME_F_HW_I2C display = U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
		U8G2LOG logger;
		std::vector<mini_synth::Key *> keys;

		Device()
		{
			for (int i = 0; i < this->numberOfRows; i++) {
				for (int j = 0; j < this->numberOfColumns; j++) {
					auto key = new Key(this->_rowIO[i], this->_columnIO[j]);
					this->keys.push_back(key);
				}
			}
		}

		void begin()
		{
			disableDebugPorts();
			pinMode(this->_modeSwitchPin, INPUT);

			for (int i = 0; i < this->numberOfRows; i++) {
				for (int j = 0; j < this->numberOfColumns; j++) {
					auto key = this->keys[i * this->numberOfColumns + j];
					key->setup();
				}
			}
			this->encoder.setup();

			int width = 32;
			int height = 6;
			this->_u8log_buffer = new uint8_t[width * height];
			this->display.begin();
			this->display.setFont(u8g2_font_ncenB08_tr);
			this->logger.begin(this->display, width, height, this->_u8log_buffer);
			this->logger.setLineHeightOffset(0);
			this->logger.setRedrawMode(0);

			this->reset();
		}

		void update()
		{
			this->encoder.update();

			for (int i = 0; i < this->keys.size(); i++) {
				auto key = this->keys[i];
				key->update();
			}
		}

		bool isModeButtonPressed()
		{
			return digitalRead(this->_modeSwitchPin) == LOW;
		}

		int numberOfKeys()
		{
			return this->keys.size();
		}

		bool isKeyPressed(int keyIndex)
		{
			if (keyIndex > this->keys.size()) {
				return Key::State::invalid;
			}
			auto key = this->keys[keyIndex];
			return key->isPressed();
		}

		bool isKeyLongPressed(int keyIndex)
		{
			if (keyIndex > this->keys.size()) {
				return Key::State::invalid;
			}
			auto key = this->keys[keyIndex];
			return key->isLongPress();
		}

		Key::State keyState(int keyIndex)
		{
			if (keyIndex > this->keys.size()) {
				return Key::State::invalid;
			}
			auto key = this->keys[keyIndex];
			return key->currentState();
		}

		void play(uint8_t note, uint8_t channel = 0)
		{
			this->_saa.SetNote(channel, note);
		}

		void sideVolume(uint8_t volume = 0, uint8_t channel = 0, AudioChannel side = AudioChannel::both)
		{
			this->_saa.SetVolume(channel, volume, side);
		}

		void volume(uint8_t volume = 0, uint8_t channel = 0)
		{
			this->sideVolume(volume, channel);
		}

		void mute(uint8_t channel = 0)
		{
			this->_saa.SetVolume(channel, 0, 0);
			this->_saa.SetVolume(channel, 0, 1);
		}

		void reset(int channel = 0)
		{
			this->_saa.Reset();
			this->_saa.SetNoiseEnable(0);
			this->_saa.SoundEnable();
			this->_saa.SetVolume(channel, 4, 0);
			this->_saa.SetVolume(channel, 4, 1);
		}
	};
}
}