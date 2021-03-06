//
// Created by ebassetti on 21/08/15.
//

#include "../../generic.h"
#include "../../Log.h"
#include "DummyAccelerometer.h"


void vendor_init(int argc, char** argv) {
	if(argv != NULL) {
		Log::d("vendor_init called with %i arguments", argc);
	}
}

Accelerometer* getAccelerometer() {
	return new DummyAccelerometer();
}

std::string getPlatformName() {
	return "Linux Generic";
}

void platformReboot() {
	exit(0);
}

void platformUpgrade(std::string path) {
	Log::d("Received upgrade hint for %s", path.c_str());
}
