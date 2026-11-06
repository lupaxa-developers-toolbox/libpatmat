#ifndef PATMAT_H
#define PATMAT_H

#define PATMAT_VERSION_STRING "0.1.0"

/* PATMAT_OK is success, and *matched is then 1 or 0. PATMAT_INVAL means a
   text, pattern, or out-pointer was missing. PATMAT_BADPAT means the
   pattern is not well formed. This function does not allocate. On
   PATMAT_INVAL and PATMAT_BADPAT, *matched is left unchanged when the
   out-pointer is not NULL. */
#define PATMAT_OK 0
#define PATMAT_INVAL (-2)
#define PATMAT_BADPAT (-3)

#ifdef __cplusplus
extern "C" {
#endif

/* Compare the whole text with pattern. Letters A–Z match either case.
   *matched is 1 on a match and 0 on a miss. */
int patmat_match(const char *text, const char *pattern, int *matched);

#ifdef __cplusplus
}
#endif

#endif /* PATMAT_H */
