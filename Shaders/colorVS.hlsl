cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
	float4 pulseColour;
	float gTime;
};

struct ColourIn
{
	float4 colour  : COLOR;
	float3 PosL  : POSITION;
};

struct VertexIn
{
	float4 colour  : COLOR;
	float3 PosL  : POSITION;
	
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float4 colour  : COLOUR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	ColourIn cin;
	// Transform to homogeneous clip space.
	//vin.PosL.xy += 0.5f*sin(vin.PosL.x)*sin(3.0f*gTime);
	//vin.PosL.z *= 0.6f + 0.4f*sin(2.0f*gTime);

	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);


	vout.colour = float4(0.0f, 1.0f, 0.0f, 1.0f);
    
    return vout;
}


