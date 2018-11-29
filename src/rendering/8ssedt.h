#pragma once

// Modifies bitmap directly
// Expects bitmap to be a 1 channel bitmap, 8 bit pixels
void GenerateSDF_Bitmap(unsigned char* bitmap, int width, int height);
