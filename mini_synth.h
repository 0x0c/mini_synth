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
		int _maxValue = INT_MAX;
		int _minValue = INT_MIN;
		int _stepUnit = 2;

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

		void setMaxValue(int maxValue)
		{
			this->_maxValue = maxValue;
		}

		void setMinValue(int minValue)
		{
			this->_minValue = minValue;
		}

		void update()
		{
			if (digitalRead(this->_swPin) == HIGH) {
				// Reference https://jumbleat.com/2016/12/17/encoder_1/
				byte currentValue = (!digitalRead(this->_phaseB) << 1) + !digitalRead(this->_phaseA);
				byte previousValue = this->_previousEncoderState & B00000011;
				byte direction = (this->_previousEncoderState & B00110000) >> 4;

				if (currentValue == 3) {
					currentValue = 2;
				}
				else if (currentValue == 2) {
					currentValue = 3;
				}
				if (currentValue != previousValue) {
					if (direction == 0) {
						if (currentValue == 1 || currentValue == 3) {
							direction = currentValue;
						}
					}
					else {
						if (currentValue == 0) {
							if (direction == 1 && previousValue == 3) {
								this->_position -= this->_stepUnit;
								this->_position = std::min(this->_position, this->_maxValue * this->_stepUnit);
								this->_valueChanged = true;
							}
							else if (direction == 3 && previousValue == 1) {
								this->_position += this->_stepUnit;
								this->_position = std::max(this->_position, this->_minValue * this->_stepUnit);
								this->_valueChanged = true;
							}
							direction = 0;
						}
					}
					this->_previousEncoderState = (direction << 4) + (previousValue << 2) + currentValue;
				}
			}
		}

		bool isValueChanged()
		{
			return this->_valueChanged;
		}

		bool isPressed()
		{
			return digitalRead(this->_swPin) == LOW;
		}

		int setPosition(int position)
		{
			this->_position = position * this->_stepUnit;
			this->_valueChanged = true;
		}

		int position()
		{
			this->_valueChanged = false;
			return this->_position / this->_stepUnit;
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
		const uint8_t _modeSwitchPin = PC13;

		SAA1099 _saa = SAA1099(PA3, PA5, PA7, PA0, PA1, PA2);
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

		bool isFunctionKeyPressed()
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
			this->_saa.setNote(channel, note);
		}

		void volume(uint8_t volume = 0, AudioChannel side = AudioChannel::both, uint8_t channel = 0)
		{
			this->_saa.setVolume(channel, volume, side);
		}

		void mute(uint8_t channel = 0)
		{
			this->_saa.setVolume(channel, 0, 0);
			this->_saa.setVolume(channel, 0, 1);
		}

		void reset(uint8_t channel = 0)
		{
			this->_saa.reset();
			this->_saa.setNoiseEnable(0);
			this->_saa.soundEnable();
			this->_saa.setVolume(channel, 4, 0);
			this->_saa.setVolume(channel, 4, 1);
		}
	};
}
}