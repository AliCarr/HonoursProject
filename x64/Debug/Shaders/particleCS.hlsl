
struct ComputeData
{
	float3 position;
	float3 velocity;
	float3 initialPosition;
};

StructuredBuffer<ComputeData> gInput	:	register(t0);
StructuredBuffer<ComputeData> gInput2	:	register(t1);
RWStructuredBuffer<ComputeData>	gOutput	:	register(u0);

[numthreads(1000, 1, 1)]
void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	int x = dispatchThreadID.x;
	int y = dispatchThreadID.y;
	
	gOutput[dispatchThreadID.x].position = gInput[dispatchThreadID.x].position + 1;
	gOutput[dispatchThreadID.x].velocity.x = gInput[dispatchThreadID.x].velocity.x + 1;
	gOutput[dispatchThreadID.x].initialPosition.x = gInput[dispatchThreadID.x].initialPosition.x + 1;

}
