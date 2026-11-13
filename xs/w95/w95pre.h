#define INCLUDE_XSPLATFORM 1
#define XSPLATFORM "w95_xs.h"

#define mxFallThrough (void)0
/* fdlibm: 'huge' is a Watcom keyword; no FLT_EVAL_METHOD in Watcom */
#define huge fd_huge
#define FLT_EVAL_METHOD 0
#define fabsl(x) fabs(x)
