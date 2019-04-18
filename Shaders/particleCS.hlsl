
struct ComputeData
{
	float3 position;
	float3 velocity;
	float3 acceleration;
	float energy;
};

StructuredBuffer<ComputeData> gInput	:	register(t0);
StructuredBuffer<ComputeData> gInput2	:	register(t1);
RWStructuredBuffer<ComputeData>	gOutput	:	register(u0);

float2 rand_2_0004(in float2 uv)
{
	float noiseX = (frac(sin(dot(uv, float2(120.9898, 708.233))) * 43758.5453));
	float noiseY = (frac(sin(dot(uv, float2(120.9898, 708.233) * 2.0)) * 43758.5453));
	return float2(noiseX, noiseY) * 0.04;
}

//

[numthreads(64, 1, 1)]
void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	gOutput[dispatchThreadID.x] = gInput[dispatchThreadID.x];

	float3 deltaTime = { 0.0178, 0.0178, 0.0178 };

	//Create Mass
	float mass = gInput[dispatchThreadID.x].energy;

	

	gOutput[dispatchThreadID.x].position.y -= gOutput[dispatchThreadID.x].acceleration.y / 30;

	float2 test = { 2, 3 };

	gOutput[dispatchThreadID.x].position += gInput[dispatchThreadID.x].velocity*deltaTime;

	if (gOutput[dispatchThreadID.x].velocity.x <= 0.001)
	{
		gOutput[dispatchThreadID.x].velocity += gInput[dispatchThreadID.x].velocity / 1000;
	}

	if (gOutput[dispatchThreadID.x].acceleration.y <= 0.1)
	{
		gOutput[dispatchThreadID.x].acceleration += deltaTime / 10;
	}
	
	gOutput[dispatchThreadID.x].energy -= 0.01;
	if (gOutput[dispatchThreadID.x].energy <= 0)
	{
		gOutput[dispatchThreadID.x].position.x = 0;
		gOutput[dispatchThreadID.x].position.y = 0;
		gOutput[dispatchThreadID.x].position.z = 0;
		gOutput[dispatchThreadID.x].energy = 1;
	}



}


