from numpy.random import rand
from scipy.optimize import linear_sum_assignment
import numpy as np
import random
import sys
from collections import defaultdict
import time
from scipy.sparse import csr_array, lil_array
from scipy.sparse.csgraph import (
    min_weight_full_bipartite_matching,
    maximum_bipartite_matching,
)


def hungarian():
    f = open(sys.argv[1])
    max_weight = 0
    while True:

        linewords = f.readline().split()
        if not linewords[0][0] == "%":
            break

    matrixlines = int(linewords[0])
    matrixcols = int(linewords[1])
    num_of_edges = int(linewords[2])

    if sys.argv[2] == "hungarian":
        matrix = np.zeros((matrixlines, matrixcols), dtype=int)

        while True:
            nextline = f.readline().split()
            if not nextline:
                break

            if len(nextline) == 2:

                matrix[int(nextline[0]) - 1][int(nextline[1]) - 1] = 1
                continue

            matrix[int(nextline[0]) - 1][int(nextline[1]) - 1] = int(nextline[2])

        row_ind, col_ind = linear_sum_assignment(matrix, maximize=True)
        max_weight = matrix[row_ind, col_ind].sum()

    elif sys.argv[2] == "sparse":

        start = time.perf_counter()
        matrix = lil_array((matrixlines, matrixcols))
        processed_lines = 0

        update_interval_read = int(max(1, num_of_edges / 500))
        while True:
            nextline = f.readline().split()
            if not nextline:
                break

            if len(nextline) == 2:

                matrix[int(nextline[0]) - 1, int(nextline[1]) - 1] = -1
                continue

            matrix[int(nextline[0]) - 1, int(nextline[1]) - 1] = -int(nextline[2])
            processed_lines = processed_lines + 1
            if (
                processed_lines
            ) % update_interval_read == 0 or processed_lines == num_of_edges:
                print(
                    f"\rImporting graph file... //  {processed_lines:,} out of {num_of_edges:,} edges read // ({(processed_lines/ num_of_edges) * 100:.0f}%)",
                    end="",
                    flush=True,
                )

        print("created lil array")

        matrix = matrix.tocsr()
        print("created csr array")

        row_ind, col_ind = min_weight_full_bipartite_matching(matrix)
        print("min weight calc done, started sum calc")
        max_weight = int(-matrix[row_ind, col_ind].sum())

        end = time.perf_counter()
        elapsed_time = end - start  # Calculate the difference
        print(f"Time elapsed: {elapsed_time:.6f} seconds \n")

    elif sys.argv[2] == "random2":
        start = time.perf_counter()
        prosumer_edges = defaultdict(list)
        pair_edge_weight = defaultdict(int)
        processed_lines = 0

        update_interval_read = int(max(1, num_of_edges / 500))

        while True:
            nextline = [int(item) for item in f.readline().split()]
            # nextline = f.readline().split()
            if not nextline:
                break

            producer = nextline[0]
            consumer = nextline[1]
            weight = nextline[2]
            prosumer_edges[(producer)].append((consumer))

            if len(nextline) == 2:

                pair_edge_weight[((producer), (consumer))] = 1
                continue

            pair_edge_weight[((producer), (consumer))] = weight

            processed_lines = processed_lines + 1
            if (
                processed_lines
            ) % update_interval_read == 0 or processed_lines == num_of_edges:
                print(
                    f"\rImporting graph file... //  {processed_lines:,} out of {num_of_edges:,} edges read // ({(processed_lines/ num_of_edges) * 100:.0f}%)",
                    end="",
                    flush=True,
                )

        print("\n\n** FINISHED READING FILE **  \n")

        assignments = []
        used_cons = set()
        dupes = 0
        update_interval = max(1, matrixlines / 100)

        for p in range(matrixlines):
            while True:
                if len(prosumer_edges[p]) != 0:
                    rand = random.choice(prosumer_edges[p])
                else:

                    print("No edges left\n")
                    break
                if rand not in used_cons:
                    used_cons.add(rand)
                    assignments.append((p, rand, pair_edge_weight[(p, rand)]))
                    break
                else:
                    dupes = dupes + 1
                    prosumer_edges[p].remove(rand)

            if (p + 1) % update_interval == 0 or p == matrixlines:
                print(
                    f"\rProcessed {p+1:,} out of {matrixlines:,} elements ({(p/ matrixlines) * 100:.0f}%)",
                    end="",
                    flush=True,
                )
                dupes = 0

        print("\n")

        max_weight = sum(val for _, _, val in assignments)

        end = time.perf_counter()
        elapsed_time = end - start  # Calculate the difference
        print(f"Time elapsed: {elapsed_time:.6f} seconds \n")

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
                        f"Processed {processed:,} out of {total_elements:,} elements ({(processed / total_elements) * 100:.0f}%)\r",
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
    print("Total weight is : " + str(max_weight))


np.set_printoptions(threshold=sys.maxsize)
np.set_printoptions(linewidth=10000000)
np.set_printoptions(suppress=True)

hungarian()
