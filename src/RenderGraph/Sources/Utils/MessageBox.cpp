#include "MessageBox.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "Assert.hpp"

#ifdef GEARSVK_SDL_MESSAGEBOX

// #include <SDL.h>
Result ShowMessageBox (const std::string& title, const std::string& message)
{
    static const std::vector<Result> buttonOrder = {
        MessageBoxResult::No,
        MessageBoxResult::Yes,
        MessageBoxResult::Third,
    };

    static const std::vector<SDL_MessageBoxButtonData> buttons = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" },
        { 0, 2, "Ignore" },
    };

    const SDL_MessageBoxData messageBoxData = {
        SDL_MESSAGEBOX_INFORMATION,
        nullptr, // parent window

        title.c_str (),
        message.c_str (),

        static_cast<int> (buttons.size ()),
        buttons.data (),

        nullptr, // colorscheme
    };

    int buttonIndex = -1;
    if (SDL_ShowMessageBox (&messageBoxData, &buttonIndex) < 0) {
        return MessageBoxResult::Error;
    }

    if (buttonIndex < 0 || buttonIndex > static_cast<int> (buttons.size ()) - 1) {
        return MessageBoxResult::Error;
    }

    return buttonOrder[buttonIndex];
}

#elif _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

MessageBoxResult ShowMessageBox (const std::string& title, const std::string& message)
{
    const int msgBoxResult = MessageBox (NULL, message.c_str (), title.c_str (), MB_YESNOCANCEL);
    switch (msgBoxResult) {
        case IDYES: return MessageBoxResult::Yes;
        case IDNO: return MessageBoxResult::No;
        case IDCANCEL: return MessageBoxResult::Third;
        default: return MessageBoxResult::Error;
    }
}

#else

// "messagebox" in terminal

MessageBoxResult ShowMessageBox (const std::string& title, const std::string& message)
{
    std::cout << title << std::endl;
    std::cout << "\t" << message << std::endl;

    char choice = 0;

    while (choice != 'y' && choice != 'n' && choice != 'i') {
        std::cout << "[y/n/i] ";
        std::cin >> choice;
        choice = std::tolower (choice);
    }

    if (choice == 'y') {
        return MessageBoxResult::Yes;
    } else if (choice == 'n') {
        return MessageBoxResult::No;
    } else if (choice == 'i') {
        return MessageBoxResult::Third;
    }

    return MessageBoxResult::No;
}

#endif
