// fdc.h - fire direction calculations (azimuth / range from the mortar position)
#pragma once

// Set the mortar's position.
void setLocation(float x, float y);

// Azimuth in degrees from the mortar to the target at (x, y).
float getAzimuth(float x, float y);

// Range from the mortar to the target at (x, y). Not implemented yet (returns 0).
float getRange(float x, float y);
