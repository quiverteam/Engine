#ifndef COMMON_CBUFFERS_DEF_H_
#define COMMON_CBUFFERS_DEF_H_

// Convenience include if you need all the internal cbuffers

#include "common_cbuffers_fxc.h"

CBUFFER_PERMODEL( register( b0 ) )
CBUFFER_PERFRAME( register( b1 ) )
CBUFFER_PERSCENE( register( b2 ) )

#endif // COMMON_CBUFFERS_DEF_H_