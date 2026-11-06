// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import localplex.player 1.0

ApplicationWindow {
    id: root
    title: "localplex"
    width: 1120
    height: 700
    minimumWidth: 800
    minimumHeight: 500
    color: "#090a0d"
    visible: true

    FontLoader {
        id: interFont
        source: "qrc:/assets/fonts/Inter-Variable.ttf"
    }

    font.family: interFont.status === FontLoader.Ready ? interFont.name : "sans-serif"

    property bool hudVisible: true
    property int hudHideDelay: 3000
    property string initialFilePath: ""

    Component.onCompleted: {
        if (initialFilePath !== "") {
            player.loadFile(initialFilePath)
        }
    }

    Timer {
        id: hudTimer
        interval: root.hudHideDelay
        repeat: false
        onTriggered: {
            if (player.hasMedia && !player.paused) {
                root.hudVisible = false
            }
        }
    }

    function pokeHud() {
        root.hudVisible = true
        hudTimer.restart()
    }

    function formatTime(seconds) {
        if (isNaN(seconds) || seconds < 0) return "00:00"
        var s = Math.floor(seconds)
        var h = Math.floor(s / 3600)
        var m = Math.floor((s % 3600) / 60)
        var remSec = s % 60
        var pad = function(n) { return n < 10 ? "0" + n : n }

        if (h > 0) {
            return pad(h) + ":" + pad(m) + ":" + pad(remSec)
        } else {
            return pad(m) + ":" + pad(remSec)
        }
    }

    // Main Video Surface
    Item {
        id: videoContainer
        anchors.fill: parent
        focus: true

        MpvPlayer {
            id: player
            objectName: "mpvPlayer"
            anchors.fill: parent

            Component.onCompleted: {
                if (Qt.application.arguments.indexOf("--benchmark-mode") !== -1) {
                    Qt.callLater(function() {
                        Qt.quit()
                    })
                }
            }
        }

        // Empty state overlay when no media loaded
        Rectangle {
            anchors.fill: parent
            color: "#090a0d"
            visible: !player.hasMedia

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 16

                Text {
                    text: "localplex"
                    color: "#9aa0ac"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    font.letterSpacing: 1.0
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "No media loaded"
                    color: "#e6e8ec"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Drop a video file here or choose a file from disk."
                    color: "#9aa0ac"
                    font.pixelSize: 13
                    Layout.alignment: Qt.AlignHCenter
                }

                Item { Layout.preferredHeight: 8 }

                Button {
                    text: "Open video file"
                    Layout.alignment: Qt.AlignHCenter
                    background: Rectangle {
                        implicitWidth: 140
                        implicitHeight: 36
                        color: parent.down ? "#222631" : (parent.hovered ? "#191c24" : "#12141a")
                        border.color: parent.visualFocus ? "#f3f4f6" : "#232733"
                        border.width: parent.visualFocus ? 2 : 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#e6e8ec"
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: fileDialog.open()
                }
            }
        }

        // Drag and drop zone
        DropArea {
            anchors.fill: parent
            onDropped: function(drop) {
                if (drop.hasUrls && drop.urls.length > 0) {
                    var url = drop.urls[0]
                    player.loadFile(url)
                    root.pokeHud()
                }
            }
        }

        // Mouse activity monitor to show/hide HUD
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            onPositionChanged: root.pokeHud()
            onClicked: {
                if (player.hasMedia) {
                    player.togglePause()
                    root.pokeHud()
                }
            }
        }

        // Keyboard Controls
        Keys.onSpacePressed: {
            if (player.hasMedia) {
                player.togglePause()
                root.pokeHud()
            }
        }
        Keys.onLeftPressed: {
            player.seek(-5, true)
            root.pokeHud()
        }
        Keys.onRightPressed: {
            player.seek(5, true)
            root.pokeHud()
        }
        Keys.onUpPressed: {
            player.volume = Math.min(100.0, player.volume + 5.0)
            root.pokeHud()
        }
        Keys.onDownPressed: {
            player.volume = Math.max(0.0, player.volume - 5.0)
            root.pokeHud()
        }
        Keys.onEscapePressed: {
            if (root.visibility === Window.FullScreen) {
                root.visibility = Window.Windowed
            }
        }

        Shortcut {
            sequence: "F"
            onActivated: {
                if (root.visibility === Window.FullScreen) {
                    root.visibility = Window.Windowed
                } else {
                    root.visibility = Window.FullScreen
                }
                root.pokeHud()
            }
        }

        Shortcut {
            sequence: "M"
            onActivated: {
                player.muted = !player.muted
                root.pokeHud()
            }
        }

        Shortcut {
            sequence: "J"
            onActivated: {
                player.seek(-10, true)
                root.pokeHud()
            }
        }

        Shortcut {
            sequence: "L"
            onActivated: {
                player.seek(10, true)
                root.pokeHud()
            }
        }

        Shortcut {
            sequence: "K"
            onActivated: {
                if (player.hasMedia) {
                    player.togglePause()
                    root.pokeHud()
                }
            }
        }
    }

    // Top Header Bar
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 52
        color: "#12141a"
        opacity: root.hudVisible ? 0.95 : 0.0
        visible: opacity > 0.0
        border.color: "#232733"
        border.width: 1

        Behavior on opacity {
            NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 16

            Text {
                text: "localplex"
                color: "#9aa0ac"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
            }

            Rectangle {
                width: 1
                height: 14
                color: "#232733"
            }

            Text {
                text: player.mediaTitle ? player.mediaTitle : (player.source ? player.source : "Ready")
                color: "#e6e8ec"
                font.pixelSize: 13
                font.weight: Font.Normal
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            Button {
                text: "Open File"
                background: Rectangle {
                    implicitWidth: 84
                    implicitHeight: 28
                    color: parent.down ? "#222631" : (parent.hovered ? "#191c24" : "transparent")
                    border.color: "#232733"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "#e6e8ec"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: fileDialog.open()
            }
        }
    }

    // Bottom Player HUD
    Rectangle {
        id: bottomHud
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 72
        color: "#12141a"
        opacity: root.hudVisible ? 0.96 : 0.0
        visible: opacity > 0.0
        border.color: "#232733"
        border.width: 1

        Behavior on opacity {
            NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // Progress scrubber bar
            Item {
                id: progressBarArea
                Layout.fillWidth: true
                Layout.preferredHeight: 12

                Rectangle {
                    id: progressTrack
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: progressMouseArea.containsMouse ? 5 : 3
                    color: "#232733"
                    radius: 2

                    Behavior on height {
                        NumberAnimation { duration: 100 }
                    }

                    Rectangle {
                        id: progressFill
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: player.duration > 0 ? parent.width * Math.min(1.0, (player.position / player.duration)) : 0
                        color: "#d97736"
                        radius: 2
                    }
                }

                MouseArea {
                    id: progressMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    preventStealing: true

                    function seekToMouse(mouse) {
                        if (player.duration > 0) {
                            var ratio = Math.max(0.0, Math.min(1.0, mouse.x / width))
                            player.seek(ratio * player.duration, false)
                            root.pokeHud()
                        }
                    }

                    onPressed: function(mouse) {
                        seekToMouse(mouse)
                    }

                    onPositionChanged: function(mouse) {
                        if (pressed) {
                            seekToMouse(mouse)
                        }
                    }
                }
            }

            // Controls row
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                // Play / Pause Toggle
                Button {
                    id: playPauseBtn
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    background: Rectangle {
                        color: playPauseBtn.hovered ? "#1c1f26" : "transparent"
                        radius: 4
                    }
                    contentItem: Image {
                        source: player.paused ? "qrc:/assets/icons/play.svg" : "qrc:/assets/icons/pause.svg"
                        sourceSize.width: 18
                        sourceSize.height: 18
                        anchors.centerIn: parent
                    }
                    onClicked: {
                        player.togglePause()
                        root.pokeHud()
                    }
                }

                // Timecode
                Text {
                    text: root.formatTime(player.position) + " / " + root.formatTime(player.duration)
                    color: "#9aa0ac"
                    font.pixelSize: 12
                    font.weight: Font.Normal
                    font.features: { "tnum": 1 }
                }

                Item { Layout.fillWidth: true }

                // Speed indicator
                Text {
                    text: player.speed !== 1.0 ? player.speed.toFixed(1) + "x" : ""
                    color: "#d97736"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    visible: player.speed !== 1.0
                }

                // Volume / Mute
                Button {
                    id: muteBtn
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    background: Rectangle {
                        color: muteBtn.hovered ? "#1c1f26" : "transparent"
                        radius: 4
                    }
                    contentItem: Image {
                        source: "qrc:/assets/icons/volume.svg"
                        sourceSize.width: 18
                        sourceSize.height: 18
                        anchors.centerIn: parent
                        opacity: player.muted ? 0.4 : 1.0
                    }
                    onClicked: {
                        player.muted = !player.muted
                        root.pokeHud()
                    }
                }

                // Fullscreen
                Button {
                    id: fsBtn
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    background: Rectangle {
                        color: fsBtn.hovered ? "#1c1f26" : "transparent"
                        radius: 4
                    }
                    contentItem: Image {
                        source: "qrc:/assets/icons/fullscreen.svg"
                        sourceSize.width: 18
                        sourceSize.height: 18
                        anchors.centerIn: parent
                    }
                    onClicked: {
                        if (root.visibility === Window.FullScreen) {
                            root.visibility = Window.Windowed
                        } else {
                            root.visibility = Window.FullScreen
                        }
                        root.pokeHud()
                    }
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select video file"
        nameFilters: ["Video files (*.mp4 *.mkv *.avi *.webm *.mov *.flv *.ts)", "All files (*)"]
        onAccepted: {
            player.loadFile(selectedFile)
            root.pokeHud()
        }
    }
}
