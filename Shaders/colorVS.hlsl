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

struct particleData
{
	float3 position;
	float3 velocity;
	float3 initialPosition;
};


StructuredBuffer<particleData> gInput	:	register(t0);

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	float take = vin.id;

	vout.colour = vin.colour;

	//vout.PosL.x += gInput[vin.id].position.x;

	vout.PosH = mul(float4(vin.PosL, 1), gWorldViewProj);




	return vout;
}


