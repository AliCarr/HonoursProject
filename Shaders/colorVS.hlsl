cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
	float4 pulseColour;
	float gTime;
};

//cbuffer offsets : register(b1)
//{
//	float xOffset;
//
//};
//
//
//cbuffer offsets : register(b2)
//{
//	float xO4ffset;
//
//};
//
//
//cbuffer offsets : register(b3)
//{
//	float xOff3set;
//
//};
//
//
//cbuffer offsets : register(b4)
//{
//	float xOf2fset;
//
//};
//
//
//cbuffer offsets : register(b0)
//{
//	float xOff1set;
//
//};


//cbuffer offsets : register(b5)
//{
//	float xOffs1et;
//	float yOffs1et;
//	float zOffs1et;
//};

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
	float3 acceleration;
	float3 initialVelocity;
	float energy;
};

StructuredBuffer<particleData> gInput : register (t1);

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.colour = pulseColour;

	vout.PosH = mul(float4(vin.PosL*gTime + gInput[vin.id].position, 1), gWorldViewProj);

	return vout;
}


