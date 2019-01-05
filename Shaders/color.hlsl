//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
	float4 pulseColour;
	float gTime;
};

struct ColourIn
{
	float4 colour  : COLOR;
};

struct VertexIn
{
	float3 PosL  : POSITION;
};



struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float4 colour  : COLOUR;
};

VertexOut VS(VertexIn vin, ColourIn cin)
{
	VertexOut vout;
//	ColourIn cin;
	// Transform to homogeneous clip space.
	vin.PosL.xy += 0.5f*sin(vin.PosL.x)*sin(3.0f*gTime);
	vin.PosL.z *= 0.6f + 0.4f*sin(2.0f*gTime);

	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);


	vout.colour = cin.colour;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	const float pi = 3.14159;
	float s = 0.5f*sin(2 * gTime - 0.25f*pi) + 0.5f;

	float4 c = lerp(pin.colour, pulseColour, s);

   return c;
}


