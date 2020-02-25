#define CBUFFER_TRANSFORM( reg ) \
cbuffer TransformBuffer_t : reg \
{ \
	matrix cModelMatrix; \
	matrix cViewMatrix; \
	matrix cProjMatrix; \
	matrix cModelViewProjMatrix; \
};