//@ This file is part of Opal.Tabs.
//@ SPDX-FileCopyrightText: 2024 Mirian Margiani
//@ SPDX-FileCopyrightText: Copyright (C) 2019 Jolla Ltd.
//@ SPDX-FileCopyrightText: Copyright (C) 2020 Open Mobile Platform LLC.
//@ SPDX-License-Identifier: GPL-3.0-or-later
//@ Original license: BSD-3-Clause (see separate license file SILICA-LICENSE)
//@ Original copyright notices are listed above.
import QtQuick 2.4
import Sailfish.Silica 1.0
import"private/Util.js"as Util
import"private"
PagedView{id:root
property int tabBarPosition:Qt.AlignTop
property bool tabBarVisible:true
property bool _tabBarIsTop:tabBarPosition==Qt.AlignTop
default property alias items:itemContainer.data
model:items
property Component header
property Component footer
property bool hasFooter:footer
property alias tabBarItem:tabBarLoader.item
property real tabBarHeight:tabBarItem&&tabBarVisible?tabBarItem.height:0
property real yOffset:currentItem&&currentItem._yOffset||0
property bool _headerBackgroundVisible:true
property Item _page:Util.findPage(root)
property int __silica_tab_view
header:_tabBarIsTop?tabBarComponent:null
footer:_tabBarIsTop?null:tabBarComponent
verticalAlignment:hasFooter?PagedView.AlignTop:PagedView.AlignBottom
cacheSize:0
//contentItem{y:root.hasFooter?0:tabBarLoader.height
contentItem{y:root.hasFooter?0:tabBarHeight
height:root.height-tabBarHeight
}Item{id:itemContainer
width:0
height:0
property Item _ctxPage:_page
property Item _ctxTabContainer:parent
property int _ctxTopMargin:_tabBarIsTop?tabBarHeight:0
property int _ctxBottomMargin:_tabBarIsTop?0:tabBarHeight
}Component{id:tabBarComponent
TabBar{model:root.model
}}Loader{id:tabBarLoader
visible:root.tabBarVisible
sourceComponent:root.hasFooter?root.footer:root.header
width:parent.width
z:root.yOffset<0&&!root.hasFooter?-1:1
y:root.hasFooter?root.height-tabBarLoader.height:Math.max(0,-root.yOffset)
Item{id:backgroundRectangleContainer
property Item item
anchors{fill:parent
topMargin:(root.yOffset>Theme.paddingSmall)||root.hasFooter?0:Theme.paddingSmall
}}}delegate:Loader{id:tabLoader
property Item _ctxPage:root._page
property Item _ctxTabContainer:tabLoader
property int _ctxTopMargin:_tabBarIsTop?tabBarHeight:0
property int _ctxBottomMargin:_tabBarIsTop?0:tabBarHeight
readonly property bool isCurrentItem:PagedView.isCurrentItem
readonly property real _yOffset:item&&item._yOffset||0
property bool loading:Qt.application.active&&isCurrentItem&&status===Loader.Loading
sourceComponent:model.modelData?model.modelData.body:model.body
source:model.modelData?model.modelData.source:model.source
asynchronous:true
width:item?item.implicitWidth:root.contentItem.width
height:item?item.implicitHeight:root.contentItem.height
onItemChanged:{if(!item)return
tabFadeAnimation.target=null
item.focus=true
item.opacity=0
tabFadeAnimation.target=item
tabFadeAnimation.from=0
tabFadeAnimation.to=1
tabFadeAnimation.restart()
}FadeAnimation{id:tabFadeAnimation
running:false
}BusyIndicator{running:!delayBusy.running&&loading
parent:tabLoader.parent
x:(tabLoader.width-width)/2+tabLoader.x
y:root.height/3-height/2-tabBarLoader.height
size:BusyIndicatorSize.Large
Timer{id:delayBusy
interval:800
running:tabLoader.loading
}}}Component.onCompleted:{backgroundRectangleContainer.item=Qt.createQmlObject("            import QtQuick 2.6\n            import %1 1.0\n\n            BackgroundRectangle {\n                id: backgroundRectangle\n                visible: _headerBackgroundVisible\n                anchors.fill: parent\n                color: __silica_applicationwindow_instance._backgroundColor\n            }\n        ".arg("Sailfish.Silica.private"),backgroundRectangleContainer,"BackgroundRectangle")
}}
