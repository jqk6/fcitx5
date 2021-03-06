/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 * Copyright (C) 2012~2013 by Yichao Yu
 * yyc1992@gmail.com
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

#include "notifications.h"
#include "dbus_public.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include <fcntl.h>

#ifndef DBUS_TIMEOUT_USE_DEFAULT
#define DBUS_TIMEOUT_USE_DEFAULT (-1)
#endif

#define NOTIFICATIONS_SERVICE_NAME "org.freedesktop.Notifications"
#define NOTIFICATIONS_INTERFACE_NAME "org.freedesktop.Notifications"
#define NOTIFICATIONS_PATH "/org/freedesktop/Notifications"
#define NOTIFICATIONS_MATCH_NAMES                                              \
    "sender='" NOTIFICATIONS_SERVICE_NAME "',"                                 \
    "interface='" NOTIFICATIONS_INTERFACE_NAME "',"                            \
    "path='" NOTIFICATIONS_PATH "'"
#define NOTIFICATIONS_MATCH_SIGNAL "type='signal'," NOTIFICATIONS_MATCH_NAMES
#define NOTIFICATIONS_MATCH_ACTION                                             \
    NOTIFICATIONS_MATCH_SIGNAL ","                                             \
                               "member='ActionInvoked'"
#define NOTIFICATIONS_MATCH_CLOSED                                             \
    NOTIFICATIONS_MATCH_SIGNAL ","                                             \
                               "member='NotificationClosed'"

namespace fcitx {

Notifications::Notifications(Instance *instance)
    : instance_(instance), dbus_(instance_->addonManager().addon("dbus")),
      bus_(dbus_->call<IDBusModule::bus>()), watcher_(*bus_) {
    reloadConfig();

    actionMatch_.reset(bus_->addMatch(
        NOTIFICATIONS_MATCH_ACTION, [this](dbus::Message message) {
            uint32_t id = 0;
            std::string key;
            if (message >> id >> key) {
                auto item = findByGlobalId(id);
                if (item && item->actionCallback_) {
                    item->actionCallback_(key);
                }
            }
            return true;
        }));
    closedMatch_.reset(bus_->addMatch(
        NOTIFICATIONS_MATCH_CLOSED, [this](dbus::Message message) {
            uint32_t id = 0;
            uint32_t reason = 0;
            if (message >> id >> reason) {
                auto item = findByGlobalId(id);
                if (item) {
                    if (item->closedCallback_) {
                        item->closedCallback_(reason);
                    }
                    removeItem(*item);
                }
            }
            return true;
        }));
    watcherEntry_.reset(watcher_.watchService(
        NOTIFICATIONS_SERVICE_NAME,
        [this](const std::string &, const std::string &oldOwner,
               const std::string &newOwner) {
            if (!oldOwner.empty()) {
                capabilities_ = 0;
                call_.reset();
                items_.clear();
                globalToInternalId_.clear();
                internalId_ = epoch_ << 32u;
                epoch_++;
            }
            if (!newOwner.empty()) {
                auto message = bus_->createMethodCall(
                    NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH,
                    NOTIFICATIONS_INTERFACE_NAME, "GetCapabilities");
                call_.reset(message.callAsync(0, [this](dbus::Message reply) {
                    std::vector<std::string> capabilities;
                    reply >> capabilities;
                    for (auto &capability : capabilities) {
                        if (capability == "actions") {
                            capabilities_ |= NotificationsCapability::Actions;
                        } else if (capability == "body") {
                            capabilities_ |= NotificationsCapability::Body;
                        } else if (capability == "body-hyperlinks") {
                            capabilities_ |= NotificationsCapability::Link;
                        } else if (capability == "body-markup") {
                            capabilities_ |= NotificationsCapability::Markup;
                        }
                    }
                    return true;
                }));
            }
        }));
}

Notifications::~Notifications() {}

void Notifications::reloadConfig() {
    auto &standardPath = StandardPath::global();
    auto file = standardPath.open(StandardPath::Type::PkgConfig,
                                  "conf/notifications.conf", O_RDONLY);
    RawConfig config;
    readFromIni(config, file.fd());

    config_.load(config);

    hiddenNotifications_.clear();
    for (const auto &id : config_.hiddenNotifications.value()) {
        hiddenNotifications_.insert(id);
    }
}

void Notifications::save() {
    std::vector<std::string> values_;
    for (const auto &id : hiddenNotifications_) {
        values_.push_back(id);
    }
    config_.hiddenNotifications.setValue(std::move(values_));

    safeSaveAsIni(config_, "conf/notifications.conf");
}

void Notifications::closeNotification(uint64_t internalId) {
    auto item = find(internalId);
    if (item) {
        if (item->globalId_) {
            auto message = bus_->createMethodCall(
                NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH,
                NOTIFICATIONS_INTERFACE_NAME, "CloseNotification");
            message << item->globalId_;
            message.send();
        }
        removeItem(*item);
    }
}

#define TIMEOUT_REAL_TIME (100)
#define TIMEOUT_ADD_TIME (TIMEOUT_REAL_TIME + 10)

uint32_t Notifications::sendNotification(
    const std::string &appName, uint32_t replaceId, const std::string &appIcon,
    const std::string &summary, const std::string &body,
    const std::vector<std::string> &actions, int32_t timeout,
    NotificationActionCallback actionCallback,
    NotificationClosedCallback closedCallback) {
    auto message =
        bus_->createMethodCall(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH,
                               NOTIFICATIONS_INTERFACE_NAME, "Notify");
    auto *replaceItem = find(replaceId);
    if (!replaceItem) {
        replaceId = 0;
    } else {
        replaceId = replaceItem->globalId_;
        removeItem(*replaceItem);
    }

    message << appName << replaceId << appIcon << summary << body;
    message << actions;
    message << dbus::Container(dbus::Container::Type::Array,
                               dbus::Signature("{sv}"));
    message << dbus::ContainerEnd();
    message << timeout;

    internalId_++;
    auto result = items_.emplace(
        std::piecewise_construct, std::forward_as_tuple(internalId_),
        std::forward_as_tuple(internalId_, actionCallback, closedCallback));
    if (!result.second) {
        return 0;
    }

    int internalId = internalId_;
    auto &item = result.first->second;
    item.slot_.reset(message.callAsync(
        TIMEOUT_REAL_TIME * 1000 / 2,
        [this, internalId](dbus::Message message) {
            auto item = find(internalId);
            if (item) {
                if (!message.isError()) {
                    uint32_t globalId;
                    if (message >> globalId) {
                        ;
                    }
                    if (item) {
                        item->globalId_ = globalId;
                        globalToInternalId_[globalId] = internalId;
                    }
                    item->slot_.reset();
                    return true;
                }
                removeItem(*item);
            }
            return true;
        }));

    return internalId;
}

void Notifications::showTip(const std::string &tipId,
                            const std::string &appName,
                            const std::string &appIcon,
                            const std::string &summary, const std::string &body,
                            int32_t timeout) {
    if (hiddenNotifications_.count(tipId)) {
        return;
    }
    std::vector<std::string> actions = {"dont-show", _("Do not show again")};
    if (capabilities_ & NotificationsCapability::Actions) {
        actions.clear();
    }
    lastTipId_ = sendNotification(appName, lastTipId_, appIcon, summary, body,
                                  actions, timeout,
                                  [this, tipId](const std::string &action) {
                                      if (action == "dont-show") {
                                          hiddenNotifications_.insert(tipId);
                                      }
                                  },
                                  {});
}

class NotificationsModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Notifications(manager->instance());
    }
};
}

FCITX_ADDON_FACTORY(fcitx::NotificationsModuleFactory)
