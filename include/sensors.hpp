#pragma once

#include "globals.hpp"


// BNO085 removed: the DPS310 is the only sensor now.
// void readBNO08x();
void readDPS310();
void writeSD();
