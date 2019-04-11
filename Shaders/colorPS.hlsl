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
	//const float pi = 3.14159;
	//float s = 0.5f*sin(2 * gTime - 0.25f*pi) + 0.5f;
	
	//float4 c = lerp(pin.colour, pulseColour, s);

   return pin.colour;
}


