#pragma once

#include "../Device.h"

namespace bongorian
{
namespace mini_synth
{
	namespace Mode
	{
		class ModeImpl
		{
		protected:
			Device *_device = nullptr;

		public:
			ModeImpl(Device *device)
			{
				this->_device = device;
			}

			virtual void update() = 0;
			virtual void display() = 0;
		};
	}
}
}
