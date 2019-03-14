#pragma once

#include "stdafx.h"
#include "Common/Camera.h"

class Controls
{
public:
	Controls();
	~Controls();

	void OnMouseDown(WPARAM btnState, int x, int y, HWND);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnKeyboardInput(float);
	void OnResize(float);

	Camera *mCamera; //In App.cpp the update needs to be changed to mCamera can become private
private:
	POINT mLastMousePos;
	
};

