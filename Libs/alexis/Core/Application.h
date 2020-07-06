#include "Utils/D3DUtils.h"
#include "Utils/Timer.h"

namespace alexis
{
	class Application
	{
	protected:
		Application(HINSTANCE hInstance);
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
		virtual ~Application();

	public:
		static Application* Get();

		HINSTANCE AppInstance() const;
		HWND GetHwnd() const;
		float GetAspectRatio() const;

		bool Get4xMsaaState() const;
		void Set4xMsaaState(bool value);

		int Run();

		virtual bool Initialize();
		virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	protected:
		virtual void
	};
}