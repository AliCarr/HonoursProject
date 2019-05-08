cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
	float4 pulseColour;
	float gTime;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float4 colour  : COLOUR;
};

float4 PS(VertexOut pin) : SV_Target
{
   return pin.colour;
}


