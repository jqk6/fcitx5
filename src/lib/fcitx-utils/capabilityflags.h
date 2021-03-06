/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_UTILS_CAPABILITYFLAGS_H_
#define _FCITX_UTILS_CAPABILITYFLAGS_H_

#include <cstdint>
#include <fcitx-utils/flags.h>

namespace fcitx {

// Use uint64_t for more space for future
enum class CapabilityFlag : uint64_t {
    None = 0,
    ClientSideUI = (1 << 0),
    Preedit = (1 << 1),
    ClientSideControlState = (1 << 2),
    Password = (1 << 3),
    FormattedPreedit = (1 << 4),
    ClientUnfocusCommit = (1 << 5),
    SurroundingText = (1 << 6),
    Email = (1 << 7),
    Digit = (1 << 8),
    Uppercase = (1 << 9),
    Lowercase = (1 << 10),
    NoAutoUpperCase = (1 << 11),
    Url = (1 << 12),
    Dialable = (1 << 13),
    Number = (1 << 14),
    NoOnScreenKeyboard = (1 << 15),
    SpellCheck = (1 << 16),
    NoSpellCheck = (1 << 17),
    WordCompletion = (1 << 18),
    UppercaseWords = (1 << 19),
    UppwercaseSentences = (1 << 20),
    Alpha = (1 << 21),
    Name = (1 << 22),
    RelativeRect = (1 << 23),
    Terminal = (1 << 24),
    Date = (1 << 25),
    Time = (1 << 26),
    Multiline = (1 << 27),
    Sensitive = (1 << 28),
    HiddenText = (1 << 29),
    GetIMInfoOnFocus = (1 << 30),
};

typedef Flags<CapabilityFlag> CapabilityFlags;
}

#endif // _FCITX_UTILS_CAPABILITYFLAGS_H_
