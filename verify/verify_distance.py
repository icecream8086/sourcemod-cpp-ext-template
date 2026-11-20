#!/usr/bin/env python3
"""Spec-level proofs for the distance primitive.

Models exact real arithmetic (not float rounding).  The scalar loop and the
vector formulation must compute the same value and be non-negative.

For bit-exact float equivalence, extend this file with IEEE-754 modeling or
use a translation-validation tool; the contract here is the mathematical core.
"""
import sys

from z3 import Not, Reals, Solver, unsat


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
    dx, dy, dz = Reals("dx dy dz")

    scalar = dx * dx + dy * dy + dz * dz
    # Vector lanes compute the same polynomial with a different association.
    vector = dz * dz + (dy * dy + dx * dx)

    ok = True
    ok &= check("scalar == vector formulation", scalar == vector)
    ok &= check("squared distance is non-negative", scalar >= 0)
    ok &= check("symmetry: d(dx) == d(-dx)",
                (dx * dx) == ((-dx) * (-dx)))
    print("note: sqrt and float rounding are outside this model")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
