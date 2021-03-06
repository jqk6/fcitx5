/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_MODULES_XCB_XCB_PUBLIC_H_
#define _FCITX_MODULES_XCB_XCB_PUBLIC_H_

#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/metastring.h>
#include <fcitx/addoninstance.h>
#include <fcitx/focusgroup.h>
#include <string>
#include <tuple>
#include <xcb/xcb.h>

struct xkb_state;

namespace fcitx {
typedef std::array<std::string, 5> XkbRulesNames;
typedef std::function<bool(xcb_connection_t *conn, xcb_generic_event_t *event)>
    XCBEventFilter;
typedef std::function<void(const std::string &name, xcb_connection_t *conn,
                           int screen, FocusGroup *group)>
    XCBConnectionCreated;
typedef std::function<void(const std::string &name, xcb_connection_t *conn)>
    XCBConnectionClosed;
typedef std::function<void(xcb_atom_t selection)> XCBSelectionNotifyCallback;

template <typename T>
using XCBReply = std::unique_ptr<T, decltype(&std::free)>;

template <typename T>
XCBReply<T> makeXCBReply(T *ptr) {
    return {ptr, &std::free};
}
}

FCITX_ADDON_DECLARE_FUNCTION(
    XCBModule, addEventFilter,
    HandlerTableEntry<XCBEventFilter> *(const std::string &, XCBEventFilter));
FCITX_ADDON_DECLARE_FUNCTION(
    XCBModule, addConnectionCreatedCallback,
    HandlerTableEntry<XCBConnectionCreated> *(XCBConnectionCreated));
FCITX_ADDON_DECLARE_FUNCTION(
    XCBModule, addConnectionClosedCallback,
    HandlerTableEntry<XCBConnectionClosed> *(XCBConnectionClosed));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, xkbState,
                             xkb_state *(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, xkbRulesNames,
                             XkbRulesNames(const std::string &));
FCITX_ADDON_DECLARE_FUNCTION(XCBModule, addSelection,
                             HandlerTableEntry<XCBSelectionNotifyCallback> *(
                                 const std::string &, const std::string &,
                                 XCBSelectionNotifyCallback));

#endif // _FCITX_MODULES_XCB_XCB_PUBLIC_H_
