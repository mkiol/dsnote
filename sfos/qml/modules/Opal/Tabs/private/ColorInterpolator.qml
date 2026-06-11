//@ This file is part of Opal.Tabs.
//@ SPDX-FileCopyrightText: 2024 Mirian Margiani
//@ SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.6
import Sailfish.Silica 1.0
QtObject{id:root
property color from
property color to
property double progress
readonly property color value:{Qt.tint(Qt.rgba(from.r,from.g,from.b,from.a),Qt.rgba(to.r,to.g,to.b,progress))
}}