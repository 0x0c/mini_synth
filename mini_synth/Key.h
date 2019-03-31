#pragma once

#include <Arduino.h>

namespace bongorian
{
namespace mini_synth
{
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
}
}