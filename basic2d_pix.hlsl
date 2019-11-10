
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

Texture2D shaderTexture;
SamplerState sampler1;

float4 main(VOUT coord) : SV_TARGET
{
	float4 retval = shaderTexture.Sample(sampler1, coord.tex);
	if (retval.a < 0.05) discard;
	return retval;
}