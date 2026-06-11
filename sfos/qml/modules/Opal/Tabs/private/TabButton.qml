//@ This file is part of Opal.Tabs.
//@ SPDX-FileCopyrightText: 2024 Mirian Margiani
//@ SPDX-FileCopyrightText: Copyright (C) 2019 - 2020 Open Mobile Platform LLC.
//@ SPDX-License-Identifier: GPL-3.0-or-later
//@ Original license: BSD-3-Clause (see separate license file SILICA-LICENSE)
//@ Original copyright notices are listed above.
import QtQuick 2.6
import Sailfish.Silica 1.0
import"Util.js"as Util
MouseArea{id:root
property int tabIndex:(model&&model.index!==undefined)?model.index:-1
property int tabCount:parent&&parent.tabCount||0
property bool isCurrentTab:_tabView&&_tabView.currentIndex>=0&&_tabView.currentIndex===tabIndex
property alias title:titleLabel.text
property alias description:descriptionLabel.text
property alias icon:highlightImage
property int titleFontSize:parent&&parent.buttonFontSize||Theme.fontSizeLarge
property int count
property Item _tabView:parent&&parent.tabView||null
readonly property Item _page:_tabView?_tabView._page:null
readonly property bool _portrait:_page&&_page.isPortrait
readonly property Item _tabItem:_tabView?(_tabView.exposedItems,_tabView.itemAt(tabIndex)):null
property alias contentItem:contentColumn
property real _extraMargin:parent&&parent.extraMargin||0
property real contentWidth:2*Theme.paddingLarge+contentColumn.implicitWidth+(bubble.active&&highlightImage.width===0?bubble.width:0)
implicitWidth:contentWidth+(root.tabIndex==0?_extraMargin:0)+(root.tabIndex==root.tabCount-1?_extraMargin:0)
implicitHeight:Math.max(_portrait?Theme.itemSizeLarge:Theme.itemSizeSmall,contentColumn.implicitHeight+2*(_portrait?Theme.paddingLarge:Theme.paddingMedium))
property bool highlighted:pressed&&containsMouse
onClicked:{if(_tabView&&tabIndex>=0){_tabView.moveTo(tabIndex)
}}ColorInterpolator{id:colorInterpolator
from:Theme.primaryColor
to:Theme.highlightColor
progress:{if(root.pressed){return 1.0
}else if(!root._tabView||!root._tabItem){return 0.0
}else if(isCurrentTab&&!root._tabView.dragging){return 1.0
}else{return Math.abs(1.0-Math.abs(root._tabItem.x/(root._tabView.width+root._tabView.horizontalSpacing)))
}}}ColorInterpolator{id:secondaryColorInterpolator
from:Theme.secondaryColor
to:Theme.secondaryHighlightColor
progress:colorInterpolator.progress
}Column{id:contentColumn
x:{if(root.tabCount>1&&root.tabIndex==0){return root.width-width-Theme.paddingMedium
}else if(root.tabCount>1&&root.tabIndex==root.tabCount-1){return Theme.paddingMedium
}else{return((root.width-width)/2)-(highlightImage.status===Image.Ready?bubble.width*0.5:0)
}}y:(root.height-height)/2
HighlightImage{id:highlightImage
anchors.horizontalCenter:parent.horizontalCenter
highlighted:root.highlighted||root.isCurrentTab
}Label{id:titleLabel
x:(contentColumn.width-width)/2
color:highlighted?Theme.highlightColor:colorInterpolator.value
font.pixelSize:highlightImage.status===Image.Ready?Theme.fontSizeTiny:root.titleFontSize
}Label{id:descriptionLabel
x:(contentColumn.width-width)/2
color:highlighted?Theme.secondaryHighlightColor:secondaryColorInterpolator.value
font.pixelSize:highlightImage.status===Image.Ready?Theme.fontSizeTiny:0.8*root.titleFontSize
}}Loader{id:bubble
x:highlightImage.status===Image.Ready?(contentColumn.width-width+highlightImage.width)/2:contentColumn.x+contentColumn.width+Theme.dp(4)
y:Theme.paddingLarge
active:root.count>0
asynchronous:true
opacity:root.highlighted?0.8:1.0
sourceComponent:Component{Rectangle{color:Theme.highlightBackgroundColor
width:bubbleLabel.text?Math.max(bubbleLabel.implicitWidth+Theme.paddingSmall*2,height):Theme.paddingMedium+Theme.paddingSmall
height:bubbleLabel.text?bubbleLabel.implicitHeight:Theme.paddingMedium+Theme.paddingSmall
radius:Theme.dp(2)
Label{id:bubbleLabel
text:{if(root.count<0){return""
}else if(root.count>99){return"99+"
}else{return root.count
}}anchors.centerIn:parent
font.pixelSize:Theme.fontSizeTiny
font.bold:true
}}}}}