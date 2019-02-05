#include "Controls.h"



Controls::Controls()
{
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

void Controls::OnMouseMove(WPARAM btnState, int x, int y, float *mTheta, float *mPhi, float *mRadius)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		*mTheta += dx;
		*mPhi += dy;

		// Restrict the angle mPhi.
		*mPhi = MathHelper::Clamp(*mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		*mRadius += dx - dy;

		// Restrict the radius.
		*mRadius = MathHelper::Clamp(*mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}
