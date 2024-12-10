import argparse


def k_best(data: list[int], k: int, epsilon: float) -> list[int]:
    sorted_data = sorted(data)
    best_array: list[int] = sorted_data[0:k]

    assert len(best_array) == k

    if best_array[-1] > (1 + epsilon) * best_array[0]:
        raise Exception("the best array is not epsilon close to the best array")
    return best_array


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-k", help="the length of the best array", type=int, default=10)
    parser.add_argument("-err", help="the relative error", type=float, default=0.02)

    args = parser.parse_args()
    print(args)

    data: list[int] = []
    while True:
        try:
            text = input()
            data += list(map(int, text.split()))
        except EOFError:
            break

    print(k_best(data, args.k, args.err))
