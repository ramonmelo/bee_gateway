#include <Arduino.h>

namespace InovaBee
{
	struct Packet
	{
		public:
			String deviceID;
			int externalTemp;
			int internalTemp;
			int humidity;
	};
}