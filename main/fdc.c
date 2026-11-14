#include <stdio.h>
#include <math.h>
#include <fdc.h>

static float Mortar_x = 0;
static float Mortar_y = 0;

void setLocation(float x, float y){
Mortar_x = x;
Mortar_y = y;
}

float getAzimuth(float x,float y){
float Azimuth = (atan2f(x-Mortar_x,y-Mortar_y)/(2*3.14159265358979323846) * 360);
return Azimuth;
}




float getRange(float x,float y){
return 0;
}