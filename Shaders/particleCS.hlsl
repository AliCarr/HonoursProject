struct ComputeData
{
	float3 position;
	float3 velocity;
	float3 acceleration;
	float3 initialVelocity;
	float energy;
};

StructuredBuffer<ComputeData> gInput	:	register(t0);
StructuredBuffer<ComputeData> gInput2	:	register(t1);
RWStructuredBuffer<ComputeData>	gOutput	:	register(u0);


[numthreads(32, 1, 1)]
void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	gOutput[dispatchThreadID.x] = gInput[dispatchThreadID.x];

	float3 deltaTime = { 0.0178, 0.0178, 0.0178 };

	gOutput[dispatchThreadID.x].position.y -= gOutput[dispatchThreadID.x].acceleration.y / 30;

	gOutput[dispatchThreadID.x].position += gInput[dispatchThreadID.x].velocity*deltaTime;

	if (gOutput[dispatchThreadID.x].velocity.x <= 0.0014)
	{
		gOutput[dispatchThreadID.x].velocity += gInput[dispatchThreadID.x].velocity / 10000;
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


