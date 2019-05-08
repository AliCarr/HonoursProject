cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
	float4 pulseColour;
	float gTime;
};


struct VertexIn
{
	float3 PosL  : POSITION;
	float2 tex: TEX;
	float4 colour  : COLOR;
	uint id : ID;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float4 colour  : COLOUR;
};


//StructuredBuffer<particleData> gInput : register (t1);

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.colour = vin.colour;

	vout.PosH = mul(float4(vin.PosL, 1), gWorldViewProj);

	return vout;
}


