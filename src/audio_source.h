/* Copyright (C) 2021 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include <QObject>

class audio_source : public QObject
{
    Q_OBJECT
public:
    enum class source_type { mic, file };
    explicit audio_source(QObject *parent = nullptr) : QObject(parent) {}
    virtual bool ok() const = 0;
    virtual int64_t read_audio(char* buff, int64_t max_size) = 0;
    virtual void clear() = 0;
    virtual double progress() const { return -1; };
    virtual source_type type() const = 0;
    virtual ~audio_source() = default;

signals:
    void audio_available();
    void error();
    void ended();
};

#endif // AUDIO_SOURCE_H
