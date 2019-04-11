
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

[numthreads(1000, 1, 1)]
void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	gOutput[dispatchThreadID.x].position.y = gInput[dispatchThreadID.x].position.y;
	//gOutput[dispatchThreadID.xy] = gInput[dispatchThreadID.xy];
	gOutput[dispatchThreadID.x].position.y += 1;
	gOutput[dispatchThreadID.x].position.z += 1;
	gOutput[dispatchThreadID.x].position.x += 1;
}
