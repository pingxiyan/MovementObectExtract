/**
* Common function: digital image process
*/

#ifndef _EXTRACT_OBJ_PRIVATE_H_
#define _EXTRACT_OBJ_PRIVATE_H_

#ifndef MIN
#define MIN(a, b)   ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#endif
#ifndef ABS
#define ABS(x) ((x) >= 0 ? (x) : (-(x)))
#endif

#ifndef RANGE_UCHAR
#define RANGE_UCHAR(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))
#endif /* RANGE_UCHAR */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */



#endif /* _EXTRACT_OBJ_PRIVATE_H_ */