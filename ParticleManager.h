#pragma once

#include "stdafx.h"
#include "Particle.h"


class ParticleManager
{
public:
	ParticleManager();
	~ParticleManager();

	bool Update();
	bool Render();

	int GetIndexCount();

private:

	const int numberOfParticles = 2;
	std::unique_ptr<MeshGeometry> mGeo = nullptr;
	Particle* mParticle;
};

