#!/usr/bin/env python3
"""Prove a 4-element sorting network is correct.

This is the "base implementation" formal-support example: any ordering
primitive (radix sort, network, std::sort wrapper) should have an equivalent
model proving (a) output is sorted and (b) output is a permutation of input.
"""
import sys

from z3 import And, If, Implies, Int, Ints, Not, Or, Solver, Sum, unsat


def swap(x, y):
    return If(x <= y, x, y), If(x <= y, y, x)


def check(name: str, formula) -> bool:
    solver = Solver()
    solver.add(Not(formula))
    result = solver.check()
    if result == unsat:
        print(f"[PASS] {name}")
        return True
    print(f"[FAIL] {name}")
    return False


def main() -> int:
    a, b, c, d = Ints("a b c d")
    # 4-element sorting network: (0,1) (2,3) (0,2) (1,3) (1,2)
    a, b = swap(a, b)
    c, d = swap(c, d)
    a, c = swap(a, c)
    b, d = swap(b, d)
    b, c = swap(b, c)

    sorted_output = And(a <= b, b <= c, c <= d)

    inputs = [Int(f"in{i}") for i in range(4)]
    # Relate symbolic inputs to the network by re-running it symbolically.
    ia, ib, ic, id_ = inputs
    ia, ib = swap(ia, ib)
    ic, id_ = swap(ic, id_)
    ia, ic = swap(ia, ic)
    ib, id_ = swap(ib, id_)
    ib, ic = swap(ib, ic)
    outputs = [ia, ib, ic, id_]

    bounded = And(*[And(v >= 0, v <= 3) for v in inputs])
    multiset = And(*[
        Sum([If(v == value, 1, 0) for v in inputs]) ==
        Sum([If(v == value, 1, 0) for v in outputs])
        for value in range(4)
    ])

    ok = True
    ok &= check("network output is sorted", And(a <= b, b <= c, c <= d))
    ok &= check("bounded inputs: output is a permutation", Implies(bounded, multiset))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
