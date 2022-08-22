// Include the precompiled headers
#include "pch.h"
#include "InputController.h"
#include "Game.h"

// Use some common namespaces to simplify the code
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Popups;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Platform;

ref class App sealed : public IFrameworkView {
public:
    virtual void Initialize(CoreApplicationView^ AppView) {
        AppView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);
        CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &App::Suspending);
        CoreApplication::Resuming += ref new EventHandler<Object^>(this, &App::Resuming);

        m_stopGame = false;
    }

    virtual void SetWindow(CoreWindow^ Window) {
        Window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::Closed);
        m_controller = ref new InputController(CoreWindow::GetForCurrentThread());
    }

    virtual void Run() {
        m_game.Initialize();

        CoreWindow^ Window = CoreWindow::GetForCurrentThread();

        while (!m_stopGame) {
            m_controller->Update();
            Window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

            m_stopGame = m_game.Update(m_controller);
            m_game.Render();
        }
    }
        
    void OnActivated(CoreApplicationView^ CoreAppView, IActivatedEventArgs^ Args) {
        CoreWindow^ Window = CoreWindow::GetForCurrentThread();
        Window->Activate();
    }

    void Closed(CoreWindow^ sender, CoreWindowEventArgs^ args) {
        m_stopGame = true;
    }

    void Suspending(Object^ Sender, SuspendingEventArgs^ Args) {}
    void Resuming(Object^ Sender, Object^ Args) {}

    virtual void Load(String^ EntryPoint) {}
    virtual void Uninitialize() {}

private:
    bool m_stopGame;
    InputController^ m_controller;
    CGame m_game;
};

ref class AppSource sealed : IFrameworkViewSource {
public:
    virtual IFrameworkView^ CreateView() {
        return ref new App();  
    }
};

[MTAThread]
int main(Array<String^>^ args) {
    CoreApplication::Run(ref new AppSource());  
    return 0;
}
