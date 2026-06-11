//@ This file is part of Opal.Tabs.
//@ SPDX-FileCopyrightText: 2024 Mirian Margiani
//@ SPDX-FileCopyrightText: Copyright (C) 2013 Jolla Ltd.
//@ SPDX-License-Identifier: GPL-3.0-or-later
//@ Original license: BSD-3-Clause (see separate license file SILICA-LICENSE)
//@ Original copyright notices are listed above.
.pragma library
function findFlickable(item){var parentItem=item?item.parent:null;while(parentItem){if(parentItem.maximumFlickVelocity&&!parentItem.hasOwnProperty("__silica_hidden_flickable")){return parentItem;}parentItem=parentItem.parent;}return null;};function findParentWithProperty(item,propertyName){var parentItem=item?item.parent:null;while(parentItem){if(parentItem.hasOwnProperty(propertyName)){return parentItem;}parentItem=parentItem.parent;}return null;};function findPageStack(item){return findParentWithProperty(item,"_pageStackIndicator");};function findPage(item){return findParentWithProperty(item,"__silica_page");};function childAt(parent,x,y){var child=parent.childAt(x,y);if(child&&child.hasOwnProperty("fragmentShader")&&child.source&&child.source.layer.effect){child=child.source;}return child;};