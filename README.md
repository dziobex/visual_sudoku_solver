# Visual Sudoku Solver
Solve your sudoku from the given graphical file or camera!<br/>
Written in C++ with OpenCV and Tesseract libraries.

# Algorithm
1. Apply graying filter
2. Apply gaussian filter
3. Apply adaptive threshold (do better work than the normal one)
4. Find the outside barriers of the sudoku
5. Transform the perspective to the given sudoku's grid