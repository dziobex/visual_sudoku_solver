#include "solve.h"

bool isValueValid(int grid[9][9], int x, int y, int value) {

    for (int i = 0; i < 9; ++i) {

        // check the row
        if (grid[y][i] == value)
            return false;

        // check the column
        if (grid[i][x] == value)
            return false;

        // check the square
        int xx = x - (x % 3) + (i % 3);
        int yy = y - (y % 3) + (i / 3);
        if (grid[yy][xx] == value)
            return false;
    }

    return true;
}

bool solve(int grid[9][9], int x, int y) {

    // finished the whole board and didn't stop on anything :) must be a valid solution
    if (x == 9) {
        x = 0;
        if (++y == 9)
            return true;
    }

    // go to the next cell if this is already filled
    if (grid[y][x] != 0)
        return solve(grid, x + 1, y);

    // check all the options
    for (int v = 1; v <= 9; ++v) {
        if (isValueValid(grid, x, y, v)) {
            grid[y][x] = v; // maybe this?

            if (solve(grid, x + 1, y))
                return true;

            grid[y][x] = 0; // naaah
        }
    }

    return false; // ok! a bad number was chosen before!
}