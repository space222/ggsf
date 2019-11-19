
cbuffer data : register(b0)
{
	float4x4 persp;
	float4 campos;
	float4 recpos, crop, dim;
};

struct VOUT
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VOUT main(uint id: SV_VertexID)
{
	VOUT retval;
	float angle = -dim.x;
	float cosangle = cos(angle);
	float sinangle = sin(angle);
	
	float4 orp = float4(0.0, 0.0, 0.0, 0.0);
	float4 pos = float4(0.0, 0.0, 0.0, 0.0);

	float2 txcrds = float2(0.0, 0.0);
	float2 ts = float2(dim.b, dim.a);
	float4 cr = crop;
	if (dim.y > 0.5) cr = float4(0, 0, ts.x, ts.y);

	retval.position = float4(0, 0, 0, 1);
	retval.tex = float2(0, 0);

	switch (id)
	{
	case 0: //top left
		retval.tex = float2(cr.x/ts.x, cr.y/ts.y);
		orp = float4(-recpos.b / 2, -recpos.a / 2, 0.0, 1.0);
		break;
	case 4:
	case 1: //top right
		retval.tex = float2((cr.x+cr.b)/ts.x, cr.y/ts.y);
		orp = float4(recpos.b / 2, -recpos.a / 2, 0.0, 1.0);
		break;
	case 3:
	case 2: //bottom left
		retval.tex = float2(cr.x/ts.x, (cr.y+cr.a)/ts.y);
		orp = float4(-recpos.b / 2, recpos.a / 2, 0.0, 1.0);
		break;
	case 5: //bottom right
		retval.tex = float2((cr.x+cr.b)/ts.x, (cr.y+cr.a)/ts.y);
		orp = float4(recpos.b / 2, recpos.a / 2, 0.0, 1.0);
		break;
	}

	float4x4 mTw = {
		cosangle, sinangle, 0, 0,
		-sinangle, cosangle, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// + recpos.ba / 2
	float4 temp = mul(mTw, orp) + float4(recpos.xy, 0.0, 0.0);
	temp -= campos;
	retval.position = mul(persp, temp);

	return retval;
}