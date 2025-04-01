from scipy.optimize import linear_sum_assignment
import numpy as np
import sys


def hungarian():
    f = open(sys.argv[1])
    max_weight = 0
    while True:

        linewords = f.readline().split()
        if not linewords[0][0] == "%":
            break

    matrixlines = int(linewords[0])
    matrixcols = int(linewords[1])
    matrix = np.zeros((matrixlines, matrixcols), dtype=int)
    while True:

        nextline = f.readline().split()
        if not nextline:
            break

        if len(nextline) < 3:

            matrix[int(nextline[0]) - 1][int(nextline[1]) - 1] = 1
            continue

        matrix[int(nextline[0]) - 1][int(nextline[1]) - 1] = int(nextline[2])
    if sys.argv[2] == "hungarian":
        row_ind, col_ind = linear_sum_assignment(matrix, maximize=True)
        max_weight = matrix[row_ind, col_ind].sum()
    elif sys.argv[2] == "random":
        random_match = []
        for row in range(matrixlines):
            nonzerovalues = []
            for col in range(matrixcols):
                if matrix[row][col] != 0:
                    nonzerovalues.append(matrix[row][col])
            randomlist = np.random.rand(len(nonzerovalues))

            random_match.append(nonzerovalues[np.argmax(randomlist)])

        max_weight = sum(random_match)

    # print(matrix)
    print("Max weight is : " + str(max_weight))


np.set_printoptions(threshold=sys.maxsize)
np.set_printoptions(linewidth=10000000)
np.set_printoptions(suppress=True)

hungarian()
