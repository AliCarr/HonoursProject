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
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float4 colour  : COLOUR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	//vin.PosL.xy += 0.5f*sin(vin.PosL.x)*sin(3.0f*gTime);
	//vin.PosL.z *= 0.6f + 0.4f*sin(2.0f*gTime);
	vout.colour = vin.colour;// float4(vin.PosL.x, vin.PosL.y, vin.PosL.z, 1);
	
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	
	
    
    return vout;
}


