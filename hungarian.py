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
        assignments = []
        available_rows = set(range(matrixlines))
        available_cols = set(range(matrixcols))

        total_elements = matrixlines * matrixcols
        candidates = []
        processed = 0
        update_interval = max(
            1, total_elements // 100
        )  # Print every ~1% of total elements

        for r in range(matrixlines):
            for c in range(matrixcols):
                if matrix[r][c] != 0:
                    candidates.append((r, c, matrix[r][c]))

                    # Update progress every X iterations
                processed += 1
                if processed % update_interval == 0 or processed == total_elements:
                    print(
                        f"Processed {processed} out of {total_elements} elements ({(processed / total_elements) * 100:.0f}%)\r",
                        end="",
                        flush=True,
                    )
        print("\n")

        # Shuffle for randomness
        np.random.shuffle(candidates)
        for r, c, val in candidates:
            if r in available_rows and c in available_cols:
                assignments.append((r, c, val))
                available_rows.remove(r)
                available_cols.remove(c)

            if not available_rows or not available_cols:
                break

        max_weight = sum(val for _, _, val in assignments)

    # print(matrix)
    print("Max weight is : " + str(max_weight))


np.set_printoptions(threshold=sys.maxsize)
np.set_printoptions(linewidth=10000000)
np.set_printoptions(suppress=True)

hungarian()
