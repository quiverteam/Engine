#pragma once

// NOTE: VS2015 changed from __iob_func to __acrt_iob_func
#if ( _MSC_VER < 1900 )
#define stdin  (&__iob_func()[0])
#define stdout (&__iob_func()[1])
#define stderr (&__iob_func()[2])
#endif