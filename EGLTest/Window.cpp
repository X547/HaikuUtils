#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <EGL/egl.h>

extern "C" status_t RenderThread(void *arg);

class TestWindow: public BWindow
{
public:
	TestWindow(BRect frame): BWindow(frame, "EGLTest", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
	}

	void Quit()
	{
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		BWindow::Quit();
	}

};

extern "C" EGLNativeWindowType createNativeWindow(void)
{
	TestWindow* wnd = new TestWindow(BRect(0, 0, 255, 255).OffsetByCopy(64, 64));
	wnd->Show();
	return (EGLNativeWindowType)wnd;
}

class TestApplication: public BApplication
{
private:
	thread_id fThread;
public:
	TestApplication(): BApplication("application/x-vnd.test.app")
	{
	}

	void ReadyToRun() {
		fThread = spawn_thread(RenderThread, "render thread", B_NORMAL_PRIORITY, this);
		resume_thread(fThread);
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
