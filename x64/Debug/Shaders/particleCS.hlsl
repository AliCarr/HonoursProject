struct ComputeData
{
	float3 position;
	float3 velocity;
	float3 acceleration;
	float3 initialVelocity;
	float energy;
};

StructuredBuffer<ComputeData> gInput	:	register(t0);
RWStructuredBuffer<ComputeData>	gOutput	:	register(u0);
//uint	gOutput	:	register(b1);
cbuffer Manipulaters : register(b1)
{
	float size;
}



[numthreads(32, 1, 1)]
void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	//Pass over all input to output
	gOutput[dispatchThreadID.x] = gInput[dispatchThreadID.x];

	//Needs replaced with proper delta time
	float3 deltaTime = { 0.0178, 0.0178, 0.0178 };

	////Add gravitational pull, then general velocity
	//gOutput[dispatchThreadID.x].position.x *= offsetX;
	//gOutput[dispatchThreadID.x].position.y *= offsetX;
	//gOutput[dispatchThreadID.x].position.z *= offsetX;


	gOutput[dispatchThreadID.x].position.y -= (gOutput[dispatchThreadID.x].acceleration.y / 30) ;
	gOutput[dispatchThreadID.x].position.x += (gInput[dispatchThreadID.x].velocity.x )*deltaTime.x;
	gOutput[dispatchThreadID.x].position.z += (gInput[dispatchThreadID.x].velocity.z )*deltaTime.z;
	gOutput[dispatchThreadID.x].position.y += (gInput[dispatchThreadID.x].velocity.y )*deltaTime.y;
//	gOutput[dispatchThreadID.x].position.x = gOutput[dispatchThreadID.x].position.x*size;
	//Cap the velocity
	if (gOutput[dispatchThreadID.x].velocity.x <= 0.0018)
		gOutput[dispatchThreadID.x].velocity += gInput[dispatchThreadID.x].velocity / 10000;

	//Cap the acceleration 
	if (gOutput[dispatchThreadID.x].acceleration.y <= 0.1)
		gOutput[dispatchThreadID.x].acceleration += deltaTime / 10;
	
	//Deplete energy, then check if the particle has vanished 
	gOutput[dispatchThreadID.x].energy -= 0.01;

	if (gOutput[dispatchThreadID.x].energy <= 0)
	{
		gOutput[dispatchThreadID.x].position.x = 0;
		gOutput[dispatchThreadID.x].position.y = 0;
		gOutput[dispatchThreadID.x].position.z = 0;
		gOutput[dispatchThreadID.x].energy = 1;
		gOutput[dispatchThreadID.x].velocity = gOutput[dispatchThreadID.x].initialVelocity;
	}
}


