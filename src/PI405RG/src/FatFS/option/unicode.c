#include <FatFS/ff.h>

#if _USE_LFN != 0

#if   _CODE_PAGE == 932	/* Japanese Shift_JIS */
#include <FatFS/option/cc932.c>
#elif _CODE_PAGE == 936	/* Simplified Chinese GBK */
#include <FatFS/option/cc936.c>
#elif _CODE_PAGE == 949	/* Korean */
#include <FatFS/option/cc949.c>
#elif _CODE_PAGE == 950	/* Traditional Chinese Big5 */
#include <FatFS/option/cc950.c>
#else					/* Single Byte Character-Set */
#include <FatFS/option/ccsbcs.c>
#endif

#endif
