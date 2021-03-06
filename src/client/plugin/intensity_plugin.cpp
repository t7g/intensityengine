
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "SDL.h"

#include <npfunctions.h>

#include "intensity_plugin.h"

#include "base/file_util.h"
#include "base/time.h"


using namespace boost;

#define TO_STRING(type, post)                  \
std::string _toString##post(type val)          \
{                                        \
    std::stringstream ss;                \
    std::string ret;                     \
    ss << val;                           \
    return ss.str();                     \
}

TO_STRING(int,)
TO_STRING(long,)
TO_STRING(double,_)

std::string _toString(double val)
{
    std::string ret = _toString_(val);
    size_t i = ret.find(".");
    if (i < 0) return ret;
    return ret.substr(0, i+4);
}

SDLKey DOMSymToSDL(int key)
{
    if (key >= 65 && key <= 65+25)
        return (SDLKey)(key + 32);
    if (key >= 112 && key <= 112+11)
        return (SDLKey)(SDLK_F1 + key - 112);

    #define TRANS(k, v) case k: return v; break;
    switch(key)
    {
        TRANS(37, SDLK_LEFT)
        TRANS(38, SDLK_UP)
        TRANS(39, SDLK_RIGHT)
        TRANS(40, SDLK_DOWN)
        TRANS(191, SDLK_SLASH)
        TRANS(16, SDLK_RSHIFT)
        TRANS(17, SDLK_RCTRL)
        TRANS(18, SDLK_RALT)
        TRANS(46, SDLK_DELETE)
    }
    return (SDLKey)key;
}

IntensityPluginObject::~IntensityPluginObject()
{
    base::KillProcess(processHandle, 0, true);
    base::CloseProcessHandle(processHandle);
}

bool IntensityPluginObject::setWindow(NPWindow *window)
{
    printf("setWindow\r\n");

    if (!initialized)
        setupComm();

    printf("update window\r\n");

    window_ = window;

    printf("SetWindow: %d, %d\r\n", window->width, window->height);
    std::string message = "sw|" + _toString((int)window->width) + "|" + _toString((int)window->height);
    channelOut->write(message);

    if (!initialized)
    {
        initialize(window);
        initialized = true;
    }

    return true;
}

void IntensityPluginObject::initialize(NPWindow *window)
{
    printf("initialize : %lu\r\n", (unsigned long)npp);

    // SDL_WINDOWID hack
   	char* buffer = new char[1000];
    sprintf(buffer, "SDL_WINDOWID=%lu",(unsigned long)(window->window));
    putenv(buffer);
    printf("env: %s\r\n", buffer);

    // Start engine in other process
    printf("Launching process in %s\r\n", INTENSITY_INSTALL_ROOT);
    file_util::SetCurrentDirectory(FilePath(INTENSITY_INSTALL_ROOT));
    std::vector<std::string> argv;
    #ifdef LINUX
        argv.push_back("./intensity_client.sh");
    #else
        argv.push_back("intensity_client.bat");
    #endif
    argv.push_back("-P"); // Tell child process it should talk to us
    base::LaunchApp(CommandLine(argv), false, false, &processHandle);

    delete[] buffer;
}

void IntensityPluginObject::setupComm()
{
    printf("setupComm\r\n");
    channelIn = new ServerChannel("ICPO");
    channelOut = new ServerChannel("ICPI");
}

void IntensityPluginObject::onMouseMove(double x, double y)
{
    double now = base::Time::Now().ToDoubleT();
    if (now - lastMouseMove < 0.005) return; // Max 200fps of mouse movements
    lastMouseMove = now;
    std::string message = "mm|" + _toString(x/window_->width) + "|" + _toString(y/window_->height);
    channelOut->write(message);
}

void IntensityPluginObject::onMouseButton(int button, bool down)
{
    std::string message = "mb|" + _toString(button) + "|" + _toString(down);
    channelOut->write(message);
}

void IntensityPluginObject::onKeyboard(int key, int unicode, bool down, bool isRepeat)
{
    key = DOMSymToSDL(key);
//printf("key: %d,%d\r\n", key, unicode);
    std::string message = "kb|" + _toString(key) + "|" + _toString(unicode) + "|" + _toString(down) + "|" + _toString(isRepeat);
    channelOut->write(message);
}

std::string IntensityPluginObject::browserCommunicate(std::string data)
{
    NPObject* window;
    assert(NPN_GetValue(npp, NPNVWindowNPObject, &window) == NPERR_NO_ERROR);
    NPIdentifier handler = NPN_GetStringIdentifier("intensityCommunicate");
    NPVariant npData, result;
    STRINGZ_TO_NPVARIANT(data.c_str(), npData);
    int success =  NPN_Invoke(npp, window, handler, &npData, 1, &result);
    if (!success) return "";

    std::string ret = "";
    if (NPVARIANT_IS_STRING(result))
    {
        NPString &npsResult = NPVARIANT_TO_STRING(result);
        ret.resize(npsResult.UTF8Length);
        for (unsigned int i = 0; i < npsResult.UTF8Length; i++)
            ret[i] = npsResult.UTF8Characters[i]; // XXX speed this up
    }
    NPN_ReleaseVariantValue(&result);
    return ret;
}

void IntensityPluginObject::frameTrigger()
{
    if (initialized && window_)
    {
        while (true)
        {
            std::vector<std::string> parsed = channelIn->readParsed();
            if (parsed.size() == 0) break;

            std::string command = parsed[0];
//            printf("Processing command: %s\r\n", command.c_str());

            if (command == "gu")
            {
                assert(parsed.size() == 1);

                // Request user info
                // XXX: Risk of deadlocks in other approaches to this, like doing it
                // during our own initialization, as part of SetWindow etc.,
                // apparently due to a Chromium issue: http://code.google.com/p/chromium/issues/detail?id=32797
                std::string userInfo = browserCommunicate("user_info");
                if (userInfo != "")
                    channelOut->write("ui|" + userInfo);
            } else {
                assert(0);
            }
        }
    }
}

