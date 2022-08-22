
#pragma once

//
// InputController:
// This class enables game controllers (e.g. Xbox controller) which follow a polling model:
//      UpdatePollingDevices - will be called at the beginning of update and as a part of checking for pause requests
//      PollingFireInUse - can use used to indicate a fire button is currently depressed.
//      ResetState - can be augumented to reset an stored state associated with the polling devices
//
// The following controls are mapped from the game controller using Windows::Gaming::Input:
//     GamepadButtons::Menu - is mapped to IsPressComplete in the WaitForInput state and IsPauseRequested in the Active state.
//     Left Thumb stick - is mapped to the move control in Active mode
//     Right Thumb stick - is mapped to the look control in Active mode
//     Right Trigger - is mapped to IsFiring in Active mode or "Fire" button

enum class InputControllerDirection {
    None,
    Left,
    Right,
    Up,
    Down,
};

ref class InputController {
internal:
    InputController(_In_ Windows::UI::Core::CoreWindow^ window);
    void InitWindow(_In_ Windows::UI::Core::CoreWindow^ window);

    void Update();

    InputControllerDirection GetDirectionOfLeftStick() { return m_directionOfLeftStick; }
    InputControllerDirection GetDirectionOfRightStick() { return m_directionOfRightStick; }
    bool IsLeftTriggerPressed() { return m_leftTriggerPressed; }
    bool IsRightTriggerPressed() { return m_rightTriggerPressed; }
    bool IsBackButtonPressed() { return m_backButtonPressed; }
    bool IsMenuButtonPressed() { return m_menuButtonPressed; }
    bool IsViewButtonPressed() { return m_viewButtonPressed; }
    bool IsAButtonPressed() { return m_aButtonPressed; }
    bool IsBButtonPressed() { return m_bButtonPressed; }
    bool IsXButtonPressed() { return m_xButtonPressed; }
    bool IsYButtonPressed() { return m_yButtonPressed; }

private:
    void ResetState();
    void UpdatePollingDevices();

    void OnKeyDown(_In_ Windows::UI::Core::CoreWindow^ sender, _In_ Windows::UI::Core::KeyEventArgs^ args);
    void OnKeyUp(_In_ Windows::UI::Core::CoreWindow^ sender, _In_ Windows::UI::Core::KeyEventArgs^ args);
    void OnBackRequested(_In_ Platform::Object^ sender, _In_ Windows::UI::Core::BackRequestedEventArgs^ args);
    void OnGamepadAdded(_In_ Platform::Object^ sender, _In_ Windows::Gaming::Input::Gamepad^ gamepad);
    void OnGamepadRemoved(_In_ Platform::Object^ sender, _In_ Windows::Gaming::Input::Gamepad^ gamepad);

private:
    // States
    InputControllerDirection m_directionOfLeftStick = InputControllerDirection::None;
    InputControllerDirection m_directionOfRightStick = InputControllerDirection::None;
    bool m_menuButtonPressed;
    bool m_viewButtonPressed;
    bool m_backButtonPressed;
    bool m_rightTriggerPressed;
    bool m_leftTriggerPressed;
    bool m_aButtonPressed;
    bool m_bButtonPressed;
    bool m_xButtonPressed;
    bool m_yButtonPressed;

    // Game controller related members
    Windows::Gaming::Input::Gamepad^ m_activeGamepad;
    std::atomic<bool> m_gamepadsChanged;
};
