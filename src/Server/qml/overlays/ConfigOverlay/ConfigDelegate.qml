import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5

import ".."

Item {
    id: configDelegate

    anchors.left: parent ? parent.left : anchors.left
    anchors.right: parent ? parent.right : anchors.right

    anchors.top: {
        if(!parent) return anchors.top
        if(treeItem.index === 0) return parent.top

        var target = parent.children[treeItem.index - 1]
        if(target !== this) {
            return parent.children[treeItem.index - 1].bottom
        }

        return anchors.top
    }
    height: totalHeight(submodelRepeater)

    property var treeItem: model.modelData

    property bool hasChildren: treeItem ? treeItem.children.length > 0 : false
    property bool hidden: false
    property bool childrenVisible: configDelegate.hasChildren && !configDelegate.hidden
    property var repeater: submodelRepeater

    function totalHeight(repeater) {
        var h = 32
        if(repeater.parentWrapper.childrenVisible) {
            for(var i = 0; i < repeater.count; i++) {
                h += totalHeight(repeater.itemAt(i).repeater)
            }
        }
        return h
    }

    RowLayout {
        id: itemRow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 32
        spacing: 0

        Button {
            Layout.minimumWidth: 32
            Layout.fillHeight: true

            enabled: configDelegate.hasChildren

            text: {
                return configDelegate.hidden ? "+" : "-"
            }

            onClicked: {
                configDelegate.hidden = !configDelegate.hidden
            }
        }

        Button {
            id: itemButton

            Layout.fillWidth: true
            Layout.fillHeight: true
            text: treeItem ? treeItem.objectName : ""


            background: Rectangle {
                color: {
                    if(treeItem === configView.selectedItem) return "#C0FFFFFF"
                    if(itemButton.pressed) return "#C0FFFFFF"
                    if(itemButton.hovered) return "#80FFFFFF"
                    return "#40FFFFFF"
                }
                border.width: 2
                border.color: "#65FFFFFF"
            }

            onClicked: {
                configView.selectedItem = treeItem
            }
        }
    }

    Item {
        id: submodelWrapper
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: itemRow.bottom

        anchors.leftMargin: 32

        property bool hidden: false
        visible: configDelegate.childrenVisible

        Repeater {
            id: submodelRepeater
            model: treeItem ? treeItem.children : null

            delegate: recursiveDelegate
            property var parentWrapper: configDelegate
        }
    }
}
