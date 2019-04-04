
struct ComputeData
{
	float3 position;
	float3 velocity;
	float3 initialPosition;
};

StructuredBuffer<ComputeData>	gInput	:	register(t0);
RWStructuredBuffer<ComputeData>	gOutput	:	register(u0);

[numthreads(1000, 1, 1)]
void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	int x = dispatchThreadID.x;
	int y = dispatchThreadID.y;


	gOutput[dispatchThreadID.x].position  = gInput[dispatchThreadID.x].position + gInput[dispatchThreadID.x].position;
	gOutput[dispatchThreadID.x].velocity = gInput[dispatchThreadID.x].position + gInput[dispatchThreadID.x].position;
	gOutput[dispatchThreadID.x].initialPosition = gInput[dispatchThreadID.x].position + gInput[dispatchThreadID.x].position;

	gInput[dispatchThreadID.x].position;
	//gOutput[int2(x, y)].position.x += 100000.0f;

	//gOutput[int2(x, y)] =
	//	gWaveConstant0 * gPrevSolInput[int2(x, y)].r +
	//	gWaveConstant1 * gCurrSolInput[int2(x, y)].r +
	//	gWaveConstant2 * (
	//		gCurrSolInput[int2(x, y + 1)].r +
	//		gCurrSolInput[int2(x, y - 1)].r +
	//		gCurrSolInput[int2(x + 1, y)].r +
	//		gCurrSolInput[int2(x - 1, y)].r);
}
