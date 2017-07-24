/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __OPENSPACE_MODULE_WEBBROWSER__BROWSER_INSTANCE_H
#define __OPENSPACE_MODULE_WEBBROWSER__BROWSER_INSTANCE_H

#include <ghoul/filesystem/filesystem.h>
#include <openspace/engine/moduleengine.h>
#include <openspace/engine/wrapper/windowwrapper.h>
#include <include/wrapper/cef_helpers.h>
#include "modules/webbrowser/include/webrenderhandler.h"
#include "modules/webbrowser/include/browserclient.h"

namespace openspace {

class BrowserInstance {
public:
    BrowserInstance(WebRenderHandler*);
    ~BrowserInstance();

    void loadUrl(const std::string &);
    bool loadLocalPath(std::string);
    void initialize();
    void reshape(const glm::ivec2&);
    void draw();
    void close(bool force = false);

    bool sendKeyEvent(const CefKeyEvent &event);
    bool sendMouseClickEvent(const CefMouseEvent &event, CefBrowserHost::MouseButtonType button, bool mouseUp);
    bool sendMouseMoveEvent(const CefMouseEvent &event);
    bool sendMouseWheelEvent(const CefMouseEvent &event, const glm::ivec2 &delta);
    void reloadBrowser();

    const CefRefPtr<CefBrowser> &getBrowser() const;

private:
    CefRefPtr<WebRenderHandler> _renderHandler;
    CefRefPtr<BrowserClient> _client;
    CefRefPtr<CefBrowser> _browser;
    bool _isInitialized;
};

}

#endif //__OPENSPACE_MODULE_WEBBROWSER__BROWSER_INSTANCE_H
