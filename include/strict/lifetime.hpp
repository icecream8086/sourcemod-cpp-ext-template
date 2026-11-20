#pragma once
// Lifetime annotations used by the L4D2 strict profile.
//
// Clang's lifetime analysis only reports on interfaces that carry
// [[clang::lifetimebound]]; without annotations it stays silent.
// Mark functions that return a reference/pointer whose lifetime is tied to an
// argument or to *this. Do NOT mark functions that return owned values.
//
//   const Thing& view(const Ctx& ctx STRICT_LIFETIMEBOUND);
//   struct S { const int* data() const STRICT_LIFETIMEBOUND; };
//
// On non-Clang compilers the annotation is empty; the code still builds.

#if defined(__clang__)
#define STRICT_LIFETIMEBOUND [[clang::lifetimebound]]
#else
#define STRICT_LIFETIMEBOUND
#endif
