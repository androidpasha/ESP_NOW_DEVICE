#pragma once
#include "constants.h"
#include "CONTROLLER.h"
#include "SLAVE.h"
#include "HANDSHAKE.h"

#define Sleep_Minutes(n) (ESP.deepSleep(n * 60 * 1'000'000, RF_NO_CAL))

