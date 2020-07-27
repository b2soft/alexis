#pragma once

#include <Core/Core.h>

class EditorApp : public alexis::IGame
{
	virtual void OnResize(int width, int height) override;
};