#ifndef SHADER_REGISTER_MAP_H_
#define SHADER_REGISTER_MAP_H_

#ifdef __cplusplus

// Constant buffer registers delegated to engine
#define INTERNAL_CBUFFER_REG_0 0
#define INTERNAL_CBUFFER_REG_1 1
#define INTERNAL_CBUFFER_REG_2 2
#define INTERNAL_CBUFFER_REG_3 3
#define INTERNAL_CBUFFER_REG_4 4

// Constant buffer registers delegated to user shaders
#define USER_CBUFFER_REG_0 5
#define USER_CBUFFER_REG_1 6
#define USER_CBUFFER_REG_2 7
#define USER_CBUFFER_REG_3 8
#define USER_CBUFFER_REG_4 9
#define USER_CBUFFER_REG_5 10
#define USER_CBUFFER_REG_6 11
#define USER_CBUFFER_REG_7 12
#define USER_CBUFFER_REG_8 13

#else // Not C++, HLSL case

// Constant buffer registers delegated to engine
#define INTERNAL_CBUFFER_REG_0 register( b0 )
#define INTERNAL_CBUFFER_REG_1 register( b1 )
#define INTERNAL_CBUFFER_REG_2 register( b2 )
#define INTERNAL_CBUFFER_REG_3 register( b3 )
#define INTERNAL_CBUFFER_REG_4 register( b4 )

// Constant buffer registers delegated to user shaders
#define USER_CBUFFER_REG_0 register( b5 )
#define USER_CBUFFER_REG_1 register( b6 )
#define USER_CBUFFER_REG_2 register( b7 )
#define USER_CBUFFER_REG_3 register( b8 )
#define USER_CBUFFER_REG_4 register( b9 )
#define USER_CBUFFER_REG_5 register( b10 )
#define USER_CBUFFER_REG_6 register( b11 )
#define USER_CBUFFER_REG_7 register( b12 )
#define USER_CBUFFER_REG_8 register( b13 )

#endif

#endif // SHADER_REGISTER_MAP_H_
