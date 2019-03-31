#pragma once

#include <Arduino.h>

namespace bongorian
{
namespace mini_synth
{
	class RoterlyEncoder
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
		RoterlyEncoder(int phaseA, int phaseB, int swPin)
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
								// Decriment
								this->_position -= this->_stepUnit;
								this->_position = std::max(this->_position, this->_minValue * this->_stepUnit);
								this->_valueChanged = true;
							}
							else if (direction == 3 && previousValue == 1) {
								// Increment
								this->_position += this->_stepUnit;
								this->_position = std::min(this->_position, this->_maxValue * this->_stepUnit);
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

		bool isSwitchPressed()
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
}
}