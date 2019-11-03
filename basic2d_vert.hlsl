
cbuffer data : register(b0)
{
	float4x4 persp;
	float4 campos;
	float4 recpos, crop, e1;
};

struct VOUT
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VOUT main(uint id: SV_VertexID)
{
	VOUT retval;
	float angle = e1.x;
	float cosangle = cos(angle);
	float sinangle = sin(angle);
	
	float4 orp = float4(0.0, 0.0, 0.0, 0.0);
	float4 pos = float4(0.0, 0.0, 0.0, 0.0);

	float2 txcrds = float2(0.0, 0.0);
	float2 ts = float2(e1.b, e1.a);

	retval.position = float4(0, 0, 0, 1);
	retval.tex = float2(0, 0);

	switch (id)
	{	
	case 0: //top left
		retval.tex = float2(0, 0);
		orp = float4(-recpos.b / 2, -recpos.a / 2, 0.0, 1.0);
		break;
	case 4:
	case 1: //top right
		retval.tex = float2(1, 0);
		orp = float4(recpos.b / 2, -recpos.a / 2, 0.0, 1.0);
		break;
	case 3:
	case 2: //bottom left
		retval.tex = float2(0, 1);
		orp = float4(-recpos.b / 2, recpos.a / 2, 0.0, 1.0);
		break;
	case 5: //bottom right
		retval.tex = float2(1, 1);
		orp = float4(recpos.b / 2, recpos.a / 2, 0.0, 1.0);
		break;
	}

	float4x4 mTw = {
		cosangle, sinangle, 0, 0,
		-sinangle, cosangle, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	//retval.position = orp;
	//retval.position.x = retval.position.x * cosangle - retval.position.y * sinangle;
	//retval.position.y = retval.position.x * sinangle + retval.position.y * cosangle;
	//retval.position.xy += recpos.xy + recpos.ba/2;
	//retval.position = mul(persp, retval.position);

	float4 temp = mul(mTw, orp) + float4(recpos.xy + recpos.ba / 2, 0.0, 0.0);
	retval.position = mul(persp, temp);

	return retval;
}