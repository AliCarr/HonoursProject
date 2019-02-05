#pragma once

#include "stdafx.h"


class Controls
{
public:
	Controls();
	~Controls();


	void OnMouseDown(WPARAM btnState, int x, int y, HWND);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y, float*, float*, float*);

private:
	POINT mLastMousePos;
};

