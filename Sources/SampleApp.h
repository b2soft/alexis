#pragma once

#include <CoreApplication.h>

using Microsoft::WRL::ComPtr;

class SampleApp : public alexis::CoreApplication
{
public:
	SampleApp(UINT width, UINT height, std::wstring name);


	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

private:
	static const UINT k_frameCount = 2;


};