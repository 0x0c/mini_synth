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
		uint8_t previousEncoderState = 0;
		int value = 0;
		bool _valueChanged = false;
		int phaseA;
		int phaseB;
		int swPin;

	public:
		Encoder(int phaseA, int phaseB, int swPin)
		{
			this->phaseA = phaseA;
			this->phaseB = phaseB;
			this->swPin = swPin;
		}

		void setup()
		{
			pinMode(this->phaseA, INPUT);
			pinMode(this->phaseB, INPUT);
			pinMode(this->swPin, INPUT);
		}

		void update()
		{
			if (digitalRead(this->swPin) == HIGH) {
				uint8_t a = digitalRead(this->phaseA);
				uint8_t b = digitalRead(this->phaseB);

				uint8_t ab = (a << 1) | b;
				uint8_t encoded = (this->previousEncoderState << 2) | ab;

				if (encoded == 0b1101 || encoded == 0b0100 || encoded == 0b0010 || encoded == 0b1011) {
					this->_valueChanged = true;
					this->value--;
				}
				else if (encoded == 0b1110 || encoded == 0b0111 || encoded == 0b0001 || encoded == 0b1000) {
					this->_valueChanged = true;
					this->value++;
				}

				this->previousEncoderState = ab;
			}
		}

		bool valueChanged()
		{
			return this->_valueChanged;
		}

		bool isPressed()
		{
			return digitalRead(this->swPin) == LOW;
		}

		int setPosition(int position)
		{
			this->value = position * 2;
		}

		int position()
		{
			this->_valueChanged = false;
			return this->value / 2;
		}
	};

	class Key
	{
	private:
		int row;
		int column;
		bool pressedState = false;
		bool previousPressedState = false;
		bool holdState = false;

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
			int identifier = row + column;
			this->row = row;
			this->column = column;
		}

		void setup()
		{
			pinMode(this->row, OUTPUT);
			digitalWrite(this->row, HIGH);
			pinMode(this->column, INPUT_PULLUP);
		}

		void update()
		{
			this->previousPressedState = this->pressedState;
			digitalWrite(this->row, LOW);
			this->pressedState = digitalRead(this->column) == LOW;
			this->holdState = (this->pressedState == true) && (this->previousPressedState == true);
			digitalWrite(this->row, HIGH);
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
			return this->pressedState;
		}

		bool isLongPress()
		{
			return this->holdState;
		}
	};

	class Device
	{
	private:
		const uint8_t rowIO[3] = { PB3, PB4, PB5 };
		const uint8_t columnIO[4] = { PB8, PB9, PB0, PB1 };
		SAA1099 saa = SAA1099(PA3, PA5, PA7, PA0, PA1, PA2);
		const uint8_t modeSwitchPin = PC13;

		uint8_t *u8log_buffer;

	public:
		const uint8_t numberOfRows = sizeof(this->rowIO) / sizeof(this->rowIO[0]);
		const uint8_t numberOfColumns = sizeof(this->columnIO) / sizeof(this->columnIO[0]);
		Encoder encoder = Encoder(PA6, PA8, PA4);
		U8G2_SSD1306_128X64_NONAME_F_HW_I2C display = U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
		U8G2LOG logger;
		std::vector<mini_synth::Key *> keys;

		Device()
		{
			for (int i = 0; i < this->numberOfRows; i++) {
				for (int j = 0; j < this->numberOfColumns; j++) {
					auto key = new Key(this->rowIO[i], this->columnIO[j]);
					this->keys.push_back(key);
				}
			}
		}

		void begin()
		{
			disableDebugPorts();
			pinMode(this->modeSwitchPin, INPUT);

			for (int i = 0; i < this->numberOfRows; i++) {
				for (int j = 0; j < this->numberOfColumns; j++) {
					auto key = this->keys[i * this->numberOfColumns + j];
					key->setup();
				}
			}
			this->encoder.setup();

			int width = 32;
			int height = 6;
			this->u8log_buffer = new uint8_t[width * height];
			this->display.begin();
			this->display.setFont(u8g2_font_ncenB08_tr);
			this->logger.begin(this->display, width, height, this->u8log_buffer);
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
			return digitalRead(this->modeSwitchPin) == LOW;
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

		void play(uint8_t note, int channel = 0)
		{
			this->saa.SetNote(channel, note);
		}

		void sideVolume(uint8_t channel = 0, uint8_t side = 0, uint8_t volume = 0)
		{
			this->saa.SetVolume(channel, volume, side);
		}

		void volume(uint8_t channel = 0, uint8_t volume = 0)
		{
			this->sideVolume(channel, 0, volume);
			this->sideVolume(channel, 1, volume);
		}

		void mute(uint8_t channel = 0)
		{
			this->saa.SetVolume(channel, 0, 0);
			this->saa.SetVolume(channel, 0, 1);
		}

		void reset(int channel = 0)
		{
			this->saa.Reset();
			this->saa.SetNoiseEnable(0);
			this->saa.SoundEnable();
			this->saa.SetVolume(channel, 4, 0);
			this->saa.SetVolume(channel, 4, 1);
		}
	};
}
}