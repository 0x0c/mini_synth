#pragma once

#include <Arduino.h>
#include <Arduino_STM32_PSG.h>
#include <U8g2lib.h>
#include <string>
#include <vector>

#include "Key.h"
#include "RoterlyEncoder.h"

namespace bongorian
{
namespace mini_synth
{
	class Device
	{
	private:
		const uint8_t _rowIO[3] = { PB3, PB4, PB5 };
		const uint8_t _columnIO[4] = { PB8, PB9, PB0, PB1 };
		const uint8_t _modeSwitchPin = PC13;
		const uint8_t _encoderSwitchPin = PA4;

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

		RoterlyEncoder encoder = RoterlyEncoder(PA6, PA8, this->_encoderSwitchPin);
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
			this->display.begin(this->_encoderSwitchPin, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE);
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