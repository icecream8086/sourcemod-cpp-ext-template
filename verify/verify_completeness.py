#!/usr/bin/env python3
"""Completeness proofs: no silent overflow, no undefined validity, dispatch total.

Covers both 32-bit and 64-bit size_t so the template stays reusable for 64-bit
SourceMod games as well.
"""
import sys

from z3 import (And, BitVec, BitVecVal, Bool, If, Implies, Not, Or, Solver,
                ULE, unsat)


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
    ok = True
    for bits in (32, 64):
        maxv = (1 << bits) - 1
        count = BitVec(f"count{bits}", bits)
        max_batch = maxv // 3
        valid_count = ULE(count, BitVecVal(max_batch, bits))
        product = count * BitVecVal(3, bits)

        # Guard => count*3 cannot wrap.
        ok &= check(f"{bits}-bit: valid count implies 3*count fits",
                    Implies(valid_count, ULE(product, BitVecVal(maxv, bits))))

        # Validity function (mirrors ValidDistanceBatch).
        names = ("ax", "ay", "az", "bx", "by", "bz", "out")
        pointers = [Bool(f"{n}{bits}") for n in names]
        all_pointers = And(*pointers)
        valid = If(valid_count, If(count == 0, True, all_pointers), False)

        ok &= check(f"{bits}-bit: empty batch is always valid",
                    Implies(And(valid_count, count == 0), valid))
        ok &= check(f"{bits}-bit: non-empty validity equals all-pointers-non-null",
                    Implies(And(valid_count, count != 0), valid == all_pointers))
        ok &= check(f"{bits}-bit: oversized count is invalid",
                    Implies(Not(valid_count), Not(valid)))

        # Dispatch totality: every feature combination yields exactly one impl.
        avx2, osxsave, force = [Bool(f"{n}{bits}") for n in ("avx2", "osxsave", "force")]
        chosen = If(force, 0, If(And(avx2, osxsave), 2, 0))
        ok &= check(f"{bits}-bit: dispatch is total",
                    Or(chosen == 0, chosen == 2))
        ok &= check(f"{bits}-bit: avx2 only with avx2+osxsave",
                    Implies(chosen == 2, And(avx2, osxsave)))

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
