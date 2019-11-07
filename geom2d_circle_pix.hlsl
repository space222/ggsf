
cbuffer data : register(b0)
{
	float4x4 persp;
	float4 campos;
	float4 recpos, crop, e3;
};

struct VOUT
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(VOUT coord) : SV_TARGET
{
	float4 retval = crop;
	float2 t = coord.tex - float2(0.5, 0.5);

	if (sqrt(t.x * t.x + t.y * t.y) > 0.5)
		discard;

	return retval;
}