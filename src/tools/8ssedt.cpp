#include <math.h>
#include <string.h>
#include <tools/8ssedt.h>

struct Point {
    int dx, dy;

    int DistSq() const { return dx * dx + dy * dy; }
};

static Point inside = { 0, 0 };
static Point empty = { 9999, 9999 };
//static Point grid1[128 * 128];
//static Point grid2[128 * 128];

Point Get(Point* grid, int x, int y, int width, int height) {
    if (x >= 0 && y >= 0 && x < width && y < height) return grid[(y * width) + x];
    return empty;
}

void Put(Point* grid, int x, int y, const Point &p, int width, int height) {
    grid[(y * width) + x] = p;
}

void Compare(Point* grid, Point &p, int x, int y, int offsetx, int offsety, int width, int height) {
    Point other = Get(grid, x + offsetx, y + offsety, width, height);
    other.dx += offsetx;
    other.dy += offsety;

    if (other.DistSq() < p.DistSq()) p = other;
}

void GenerateSDF(Point* grid, int width, int height) {
    // Pass 0
    for (int y = 0;y < height;y++) {
        for (int x = 0;x < width;x++) {
            Point p = Get(grid, x, y, width, height);
            Compare(grid, p, x, y, -1,  0, width, height);
            Compare(grid, p, x, y,  0, -1, width, height);
            Compare(grid, p, x, y, -1, -1, width, height);
            Compare(grid, p, x, y,  1, -1, width, height);
            Put(grid, x, y, p, width, height);
        }

        for (int x = width - 1;x >= 0;x--) {
            Point p = Get(grid, x, y, width, height);
            Compare(grid, p, x, y, 1, 0, width, height);
            Put(grid, x, y, p, width, height);
        }
    }

    // Pass 1
    for (int y = height - 1;y >= 0;y--) {
        for (int x = width - 1;x >= 0;x--) {
            Point p = Get(grid, x, y, width, height);
            Compare(grid, p, x, y,  1,  0, width, height);
            Compare(grid, p, x, y,  0,  1, width, height);
            Compare(grid, p, x, y, -1,  1, width, height);
            Compare(grid, p, x, y,  1,  1, width, height);
            Put(grid, x, y, p, width, height);
        }

        for (int x = 0;x < width;x++) {
            Point p = Get(grid, x, y, width, height);
            Compare(grid, p, x, y, -1, 0, width, height);
            Put(grid, x, y, p, width, height);
        }
    }
}

void GenerateSDF_Bitmap(unsigned char* bitmap, int width, int height) {
    Point* grid1 = new Point[width * height];
    Point* grid2 = new Point[width * height];
    memset(grid1, 0, sizeof(Point) * width * height);
    memset(grid2, 0, sizeof(Point) * width * height);
    
    for(int y = 0;y < width;y++) {
        for (int x = 0;x < height;x++) {
            // Points inside get marked with a dx/dy of zero.
            // Points outside get marked with an infinitely large distance.
            if (bitmap[(y * width) + x] < 128) {
                Put(grid1, x, y, inside, width, height);
                Put(grid2, x, y, empty, width, height);
            } else {
                Put(grid2, x, y, inside, width, height);
                Put(grid1, x, y, empty, width, height);
            }
        }
    }

    // Generate the SDF.
    GenerateSDF(grid1, width, height);
    GenerateSDF(grid2, width, height);
    
    // Render out the results.
    for(int y = 0;y < height;y++) {
        for (int x = 0;x < width;x++ ) {
            // Calculate the actual distance from the dx/dy
            int dist1 = (int)(sqrt((double)Get( grid1, x, y, width, height).DistSq()));
            int dist2 = (int)(sqrt((double)Get( grid2, x, y, width, height).DistSq()));
            int dist = dist1 - dist2;

            int c = dist + 128;
            if(c < 0) c = 0;
            if(c > 255) c = 255;

            bitmap[(y * width) + x] = (unsigned char)c;
        }
    }
      
    delete [] grid1;
    delete [] grid2;
}
