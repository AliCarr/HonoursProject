#include "Controls.h"



Controls::Controls()
{
	mCamera = new Camera();
	mCamera->SetPosition(0.0f, 2.0f, -15.0f);
}


Controls::~Controls()
{
}

void Controls::OnMouseDown(WPARAM btnState, int x, int y, HWND mhMainWnd)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Controls::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Controls::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera->Pitch(dy);
		mCamera->RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Controls::OnKeyboardInput(float gt)
{
	const float dt = gt;

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera->Walk(10.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera->Walk(-10.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera->Strafe(-10.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera->Strafe(10.0f*dt);

	mCamera->UpdateViewMatrix();
	//mCamera.Update();
}

void Controls::OnResize(float ratio)
{
	mCamera->Resize(ratio);
	mCamera->SetLens(0.25f*MathHelper::Pi, ratio, 1.0f, 1000.0f);
}
//
//Camera Controls::GetCamera()
//{
//	return mCamera;
//}