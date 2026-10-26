// Select the longstanding glibc math ABI so the Pi bundles also load on older
// MODEP/Patchbox images. This uses glibc's supported compatibility symbols;
// it does not ship or replace the target's system libraries.
#if defined(__aarch64__)
#define ATOMIC_GLIBC_BASE "GLIBC_2.17"
#elif defined(__arm__)
#define ATOMIC_GLIBC_BASE "GLIBC_2.4"
#endif
#ifdef ATOMIC_GLIBC_BASE
extern "C" {
double atomic_compat_exp(double);
double atomic_compat_log(double);
double atomic_compat_pow(double,double);
__asm__(".symver atomic_compat_exp,exp@" ATOMIC_GLIBC_BASE);
__asm__(".symver atomic_compat_log,log@" ATOMIC_GLIBC_BASE);
__asm__(".symver atomic_compat_pow,pow@" ATOMIC_GLIBC_BASE);
double __wrap_exp(double x) { return atomic_compat_exp(x); }
double __wrap_log(double x) { return atomic_compat_log(x); }
double __wrap_pow(double x,double y) { return atomic_compat_pow(x,y); }
}
#endif
