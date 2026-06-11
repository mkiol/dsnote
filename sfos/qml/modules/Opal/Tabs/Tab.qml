//@ This file is part of Opal.Tabs.
//@ SPDX-FileCopyrightText: 2024 Mirian Margiani
//@ SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import Sailfish.Silica 1.0
QtObject{property string title
property string description
property int count:0
property string icon
default property Component body
property url source
}