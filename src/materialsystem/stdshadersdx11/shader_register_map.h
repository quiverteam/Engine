#ifndef SHADER_REGISTER_MAP_H_
#define SHADER_REGISTER_MAP_H_

#ifdef __cplusplus

// Constant buffer registers delegated to engine
#define INTERNAL_CBUFFER_REG_0 0
#define INTERNAL_CBUFFER_REG_1 1
#define INTERNAL_CBUFFER_REG_2 2
#define INTERNAL_CBUFFER_REG_3 3

// Constant buffer registers delegated to user shaders
#define USER_CBUFFER_REG_0 4
#define USER_CBUFFER_REG_1 5
#define USER_CBUFFER_REG_2 6
#define USER_CBUFFER_REG_3 7
#define USER_CBUFFER_REG_4 8
#define USER_CBUFFER_REG_5 9
#define USER_CBUFFER_REG_6 10
#define USER_CBUFFER_REG_7 11
#define USER_CBUFFER_REG_8 12

#else // Not C++, HLSL case

// Constant buffer registers delegated to engine
#define INTERNAL_CBUFFER_REG_0 register( b0 )
#define INTERNAL_CBUFFER_REG_1 register( b1 )
#define INTERNAL_CBUFFER_REG_2 register( b2 )
#define INTERNAL_CBUFFER_REG_3 register( b3 )

// Constant buffer registers delegated to user shaders
#define USER_CBUFFER_REG_0 register( b4 )
#define USER_CBUFFER_REG_1 register( b5 )
#define USER_CBUFFER_REG_2 register( b6 )
#define USER_CBUFFER_REG_3 register( b7 )
#define USER_CBUFFER_REG_4 register( b8 )
#define USER_CBUFFER_REG_5 register( b9 )
#define USER_CBUFFER_REG_6 register( b10 )
#define USER_CBUFFER_REG_7 register( b11 )
#define USER_CBUFFER_REG_8 register( b12 )

#endif

#endif // SHADER_REGISTER_MAP_H_
