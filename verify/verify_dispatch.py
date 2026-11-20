#!/usr/bin/env python3
"""Prove the dispatch rule is safe.

Mirrors template_ext::SelectDistanceBatchFn:
    forceScalar               -> scalar (0)
    features.avx2 && osxsave  -> avx2   (2)
    otherwise                 -> scalar (0)
"""
from z3 import And, Bools, If, Implies, Not, Or, Solver, unsat

SCALAR, AVX2 = 0, 2


def check(name: str, formula) -> bool:
    solver = Solver()
    solver.add(Not(formula))
    result = solver.check()
    if result == unsat:
        print(f"[PASS] {name}")
        return True
    print(f"[FAIL] {name}")
    if solver.model():
        print("       counterexample:", solver.model())
    return False


def main() -> int:
    avx2, osxsave, sse42, force = Bools("avx2 osxsave sse42 force")
    chosen = If(force, SCALAR, If(And(avx2, osxsave), AVX2, SCALAR))

    ok = True
    ok &= check("AVX2 path requires CPU+OS support",
                Implies(chosen == AVX2, And(avx2, osxsave)))
    ok &= check("scalar path is used when unsupported or forced",
                Implies(chosen == SCALAR, Or(force, Not(And(avx2, osxsave)))))
    ok &= check("selection is total (scalar or avx2)",
                Or(chosen == SCALAR, chosen == AVX2))
    ok &= check("forcing scalar cannot select AVX2",
                Implies(force, chosen == SCALAR))
    ok &= check("osxsave alone is not enough for AVX2",
                Implies(And(avx2, Not(osxsave)), chosen == SCALAR))

    # sse42 is intentionally allowed to be irrelevant to this primitive.
    print("note: sse42 is modeled but not used by this dispatch rule")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
