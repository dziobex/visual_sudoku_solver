# Visual Sudoku Solver
Solve your sudoku from the given graphical file or camera!<br/>
Written in C++ with OpenCV and Tesseract libraries.

# Usage
1. Use the main camera:
```
./visual_sudoku_solver.exe
```
2. Use your image
```
./visual_sudoku_solver.exe <path_to_the_file>
```
3. Use one of the sample images
```
./visual_sudoku_solver.exe samples "01, 02, ..., n"
```

# Preview

# Algorithm
1. Apply a graying filter.
2. Apply a Gaussian filter.
3. Apply adaptive thresholding (does better work than the normal one).
4. Find the largest rectangle, which is a candidate to be the Sudoku grid.
5. Find the outer barriers of the detected shape.
6. Split that shape into 9x9 squares.
7. Transform each cell to be more OCR-friendly (remove borders, noise, etc...)
8. If no numbers are found inside those squares, go back to step 4. however, exclude that rectangle from the search set.
9. Otherwise, send results to the solving functions and check for any solution.
10. If a solution exists, print it onto the original frame with the appropriate transformation.