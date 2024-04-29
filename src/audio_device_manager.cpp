/* Copyright (C) 2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "audio_device_manager.hpp"

#include <pulse/context.h>
#include <pulse/error.h>
#include <pulse/introspect.h>
#include <pulse/mainloop.h>
#include <pulse/subscribe.h>

#include <stdexcept>
#include <string>

#include "config.h"
#include "logger.hpp"

static void state_pa_callback(pa_context *ctx,
                              [[maybe_unused]] void *userdata) {
    switch (pa_context_get_state(ctx)) {
        case PA_CONTEXT_CONNECTING:
            LOGD("pa connecting");
            break;
        case PA_CONTEXT_AUTHORIZING:
            LOGD("pa authorizing");
            break;
        case PA_CONTEXT_SETTING_NAME:
            LOGD("pa setting name");
            break;
        case PA_CONTEXT_READY:
            LOGD("pa ready");
            break;
        case PA_CONTEXT_TERMINATED:
            LOGD("pa terminated");
            break;
        case PA_CONTEXT_FAILED:
            LOGD("pa failed");
            throw std::runtime_error{"pa failed"};
        default:
            LOGD("pa unknown state");
    }
}

audio_device_manager::audio_device_manager(
    sources_changed_cb_t sources_changed_cb) {
    m_pa_loop = pa_mainloop_new();
    if (!m_pa_loop) throw std::runtime_error{"pa_mainloop_new error"};

    try {
        auto *mla = pa_mainloop_get_api(m_pa_loop);

        m_pa_ctx = pa_context_new(mla, APP_ID);
        if (!m_pa_ctx) throw std::runtime_error{"pa_context_new error"};

        if (pa_context_connect(m_pa_ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) <
            0) {
            throw std::runtime_error{std::string{"pa_context_connect error: "} +
                                     pa_strerror(pa_context_errno(m_pa_ctx))};
        }

        pa_context_set_state_callback(m_pa_ctx, state_pa_callback, this);

        while (true) {
            auto ret = pa_mainloop_iterate(m_pa_loop, 0, nullptr);
            auto state = pa_context_get_state(m_pa_ctx);
            if (ret < 0 || state == PA_CONTEXT_FAILED ||
                state == PA_CONTEXT_TERMINATED)
                throw std::runtime_error{"pa error"};
            if (state == PA_CONTEXT_READY) break;
        }

        pa_context_set_subscribe_callback(m_pa_ctx, subscription_pa_callback,
                                          this);
        auto mask =
            static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SOURCE);

        auto *op = pa_context_subscribe(
            m_pa_ctx, mask,
            [](pa_context *ctx, int success, void *userdata) {
                if (success) {
                    pa_operation_unref(pa_context_get_source_info_list(
                        ctx, source_info_pa_callback, userdata));
                }
            },
            this);
        if (!op) throw std::runtime_error("pa_context_subscribe error");
        pa_operation_unref(op);
    } catch (...) {
        clean();

        throw;
    }

    m_sources_changed_cb = std::move(sources_changed_cb);

    m_thread = std::thread{[&]() {
        int ret = 0;
        pa_mainloop_run(m_pa_loop, &ret);

        LOGD("pa loop finished: " << ret);
    }};
}

audio_device_manager::~audio_device_manager() { clean(); }

std::vector<audio_device_manager::device_t> audio_device_manager::sources() {
    decltype(sources()) sources_list;

    {
        std::lock_guard guard{m_mtx};
        std::transform(m_sources.cbegin(), m_sources.cend(),
                       std::back_inserter(sources_list),
                       [](const auto &p) { return p.second; });
    }

    return sources_list;
}

void audio_device_manager::remove_source_by_index(unsigned int index) {
    std::unique_lock guard{m_mtx};
    auto it = std::find_if(
        m_sources.cbegin(), m_sources.cend(),
        [index](const auto &p) { return p.second.index == index; });
    if (it != m_sources.cend()) {
        m_sources.erase(it);
        guard.unlock();
        if (m_sources_changed_cb) m_sources_changed_cb();
    }
}

void audio_device_manager::subscription_pa_callback(
    pa_context *ctx, pa_subscription_event_type_t t, uint32_t idx,
    void *userdata) {
    auto facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    auto type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

    switch (facility) {
        case PA_SUBSCRIPTION_EVENT_SOURCE:
            if (type == PA_SUBSCRIPTION_EVENT_NEW ||
                type == PA_SUBSCRIPTION_EVENT_CHANGE) {
                if (type == PA_SUBSCRIPTION_EVENT_NEW)
                    LOGD("pa source created: " << idx);
                else
                    LOGD("pa source changed: " << idx);
                pa_operation_unref(pa_context_get_source_info_by_index(
                    ctx, idx, source_info_pa_callback, userdata));
            } else if (type == PA_SUBSCRIPTION_EVENT_REMOVE) {
                LOGD("pa sink input removed: " << idx);
                static_cast<audio_device_manager *>(userdata)
                    ->remove_source_by_index(idx);
            }
            break;
        default:
            break;
    }
}

bool audio_device_manager::has_source_name(const std::string &name) {
    std::lock_guard guard{m_mtx};
    return m_sources.count(name) > 0;
}

std::optional<audio_device_manager::device_t>
audio_device_manager::source_by_name(const std::string &name) {
    std::lock_guard guard{m_mtx};
    if (m_sources.count(name) == 0) return std::nullopt;

    return m_sources.at(name);
}

std::optional<audio_device_manager::device_t>
audio_device_manager::source_by_description(const std::string &description) {
    std::lock_guard guard{m_mtx};
    auto it = std::find_if(m_sources.cbegin(), m_sources.cend(),
                           [&description](const auto &p) {
                               return p.second.description == description;
                           });

    if (it == m_sources.cend()) return std::nullopt;

    return it->second;
}

void audio_device_manager::source_info_pa_callback(
    [[maybe_unused]] pa_context *ctx, const pa_source_info *info, int eol,
    void *userdata) {
    auto *dm = static_cast<audio_device_manager *>(userdata);

    if (eol) {
        dm->m_sources_discovery_done = true;
        if (dm->m_sources_changed_cb) dm->m_sources_changed_cb();
        return;
    }

    LOGD("pa source: " << info->name << " " << info->description);

    std::lock_guard guard{dm->m_mtx};

    auto &device = dm->m_sources[info->name];

    device.name = info->name;
    device.description = info->description;
    device.index = info->index;
}

void audio_device_manager::clean() {
    if (m_thread.joinable()) m_thread.join();

    if (m_pa_ctx) {
        pa_context_unref(m_pa_ctx);
        m_pa_ctx = nullptr;
    }

    if (m_pa_loop) {
        pa_mainloop_free(m_pa_loop);
        m_pa_loop = nullptr;
    }
}
