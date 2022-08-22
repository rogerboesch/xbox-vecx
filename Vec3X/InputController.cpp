
#include "pch.h"
#include "InputController.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace std;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI;
using namespace Windows::Foundation;
using namespace Windows::Devices::Input;
using namespace Windows::Gaming::Input;
using namespace Windows::System;

// Analog control deadzone definitions. Tune these values to adjust the size of the deadzone.
// Thumbstick range in each dimension is defined as [-1, 1].
#define THUMBSTICK_DEADZONE 0.25f

// Trigger range is defined as [0, 1].
#define TRIGGER_DEADZONE 0.1f

InputController::InputController(_In_ CoreWindow^ window):
    m_activeGamepad(nullptr),
    m_gamepadsChanged(true) {
    InitWindow(window);
}

void InputController::InitWindow(_In_ CoreWindow^ window) {
    ResetState();

    window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &InputController::OnKeyDown);
    window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &InputController::OnKeyUp);

    SystemNavigationManager::GetForCurrentView()->BackRequested += ref new EventHandler<BackRequestedEventArgs^>(this, &InputController::OnBackRequested);

    // Detect gamepad connection and disconnection events.
    Gamepad::GamepadAdded += ref new EventHandler<Gamepad^>(this, &InputController::OnGamepadAdded);
    Gamepad::GamepadRemoved += ref new EventHandler<Gamepad^>(this, &InputController::OnGamepadRemoved);
}

void InputController::OnKeyDown(_In_ CoreWindow^, _In_ KeyEventArgs^ args) {
    Windows::System::VirtualKey Key;
    Key = args->VirtualKey;

    if (Key == VirtualKey::Q)
        m_directionOfLeftStick = InputControllerDirection::Left;
    else if (Key == VirtualKey::W)
        m_directionOfLeftStick = InputControllerDirection::Up;
    else if (Key == VirtualKey::E)
        m_directionOfLeftStick = InputControllerDirection::Down;
    else if (Key == VirtualKey::R)
        m_directionOfLeftStick = InputControllerDirection::Right;

    if (Key == VirtualKey::A)
        m_aButtonPressed = true;
    else if (Key == VirtualKey::B)
        m_bButtonPressed = true;
    else if (Key == VirtualKey::X)
        m_xButtonPressed = true;
    else if (Key == VirtualKey::Y)
        m_yButtonPressed = true;
    else if (Key == VirtualKey::M)
        m_menuButtonPressed = true;
    else if (Key == VirtualKey::V)
        m_viewButtonPressed = true;
}

void InputController::OnKeyUp(_In_ CoreWindow^, _In_ KeyEventArgs^ args) {
    Windows::System::VirtualKey Key;
    Key = args->VirtualKey;

    if (Key == VirtualKey::Q)
        m_directionOfLeftStick = InputControllerDirection::None;
    else if (Key == VirtualKey::W)
        m_directionOfLeftStick = InputControllerDirection::None;
    else if (Key == VirtualKey::E)
        m_directionOfLeftStick = InputControllerDirection::None;
    else if (Key == VirtualKey::R)
        m_directionOfLeftStick = InputControllerDirection::None;
    else if (Key == VirtualKey::A)
        m_aButtonPressed = false;
    else if (Key == VirtualKey::B)
        m_bButtonPressed = false;
    else if (Key == VirtualKey::X)
        m_xButtonPressed = false;
    else if (Key == VirtualKey::Y)
        m_yButtonPressed = false;
    else if (Key == VirtualKey::M)
        m_menuButtonPressed = false;
    else if (Key == VirtualKey::V)
        m_viewButtonPressed = false;
}

void InputController::ResetState() {
    m_directionOfLeftStick = InputControllerDirection::None;
    m_directionOfRightStick = InputControllerDirection::None;
    m_backButtonPressed = false;
    m_leftTriggerPressed = false;
    m_rightTriggerPressed = false;
    m_aButtonPressed = false;
    m_bButtonPressed = false;
    m_xButtonPressed = false;
    m_yButtonPressed = false;
    m_menuButtonPressed = false;
    m_viewButtonPressed = false;
}

//----------------------------------------------------------------------

void InputController::UpdatePollingDevices() {
    if (m_gamepadsChanged) {
        m_gamepadsChanged = false;
        unsigned int index = 0;

        // Capture the list of gamepads so it won't change while we are studying it.
        auto gamepads = Gamepad::Gamepads;

        if (gamepads->Size == 0) {
            m_activeGamepad = nullptr;
        }
        else if (!gamepads->IndexOf(m_activeGamepad, &index)) {
            // Check if the cached gamepad is still connected.
            // InputController defaults to the first active gamepad.
            m_activeGamepad = gamepads->GetAt(0);
        }
    }

    ResetState();

    if (m_activeGamepad == nullptr) {
        return;
    }


    GamepadReading reading = m_activeGamepad->GetCurrentReading();

    if ((reading.Buttons & GamepadButtons::Menu) == GamepadButtons::Menu) {
        m_menuButtonPressed = true;
    }
    if ((reading.Buttons & GamepadButtons::View) == GamepadButtons::View) {
        m_viewButtonPressed = true;
    }
    if ((reading.Buttons & GamepadButtons::A) == GamepadButtons::A) {
        m_aButtonPressed = true;
    }
    if ((reading.Buttons & GamepadButtons::B) == GamepadButtons::B) {
        m_bButtonPressed = true;
    }
    if ((reading.Buttons & GamepadButtons::X) == GamepadButtons::X) {
        m_xButtonPressed = true;
    }
    if ((reading.Buttons & GamepadButtons::Y) == GamepadButtons::Y) {
        m_yButtonPressed = true;
    }

    if (reading.LeftThumbstickX > THUMBSTICK_DEADZONE || reading.LeftThumbstickX < -THUMBSTICK_DEADZONE) {
        float x = static_cast<float>(reading.LeftThumbstickX);
        float direction = (x > 0) ? 1 : -1;

        if (direction == 1) {
            m_directionOfLeftStick = InputControllerDirection::Right;
        }
        else if (direction == -1) {
            m_directionOfLeftStick = InputControllerDirection::Left;
        }
    }

    if (reading.LeftThumbstickY > THUMBSTICK_DEADZONE || reading.LeftThumbstickY < -THUMBSTICK_DEADZONE) {
        float y = static_cast<float>(reading.LeftThumbstickY);
        float direction = (y > 0) ? 1 : -1;

        if (direction == 1) {
            m_directionOfLeftStick = InputControllerDirection::Up;
        }
        else if (direction == -1) {
            m_directionOfLeftStick = InputControllerDirection::Down;
        }
    }

    if (reading.RightThumbstickX > THUMBSTICK_DEADZONE || reading.RightThumbstickX < -THUMBSTICK_DEADZONE) {
        float x = static_cast<float>(reading.RightThumbstickX);
        float direction = (x > 0) ? 1 : -1;

        if (direction == 1) {
            m_directionOfRightStick = InputControllerDirection::Right;
        }
        else if (direction == -1) {
            m_directionOfRightStick = InputControllerDirection::Left;
        }
    }

    if (reading.RightThumbstickY > THUMBSTICK_DEADZONE || reading.RightThumbstickY < -THUMBSTICK_DEADZONE) {
        float y = static_cast<float>(reading.RightThumbstickY);
        float direction = (y > 0) ? 1 : -1;

        if (direction == 1) {
            m_directionOfRightStick = InputControllerDirection::Up;
        }
        else if (direction == -1) {
            m_directionOfRightStick = InputControllerDirection::Down;
        }
    }

    if (reading.LeftTrigger > TRIGGER_DEADZONE) {
        m_leftTriggerPressed = true;
    }

    if (reading.RightTrigger > TRIGGER_DEADZONE) {
        m_rightTriggerPressed = true;
    }
}

void InputController::Update() {
    UpdatePollingDevices();
}

void InputController::OnBackRequested(_In_ Platform::Object^ sender, _In_ BackRequestedEventArgs^ args) {
    m_backButtonPressed = true;
    args->Handled = true;
}

void InputController::OnGamepadAdded(_In_ Object^ sender, _In_ Gamepad^ gamepad) {
    m_gamepadsChanged = true;
}

void InputController::OnGamepadRemoved(_In_ Object^ sender, _In_ Gamepad^ gamepad) {
    m_gamepadsChanged = true;
}
