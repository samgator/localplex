// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import localplex.player 1.0
import localplex.core 1.0
import localplex.database 1.0

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
    property int hudHideDelay: 2800
    property string initialFilePath: ""

    // Toast state
    property string toastText: ""
    property bool toastVisible: false

    function showToast(message) {
        toastText = message
        toastVisible = true
        toastTimer.restart()
    }

    Timer {
        id: toastTimer
        interval: 1800
        repeat: false
        onTriggered: root.toastVisible = false
    }

    Timer {
        id: hudTimer
        interval: root.hudHideDelay
        repeat: false
        onTriggered: {
            if (player.hasMedia && !player.paused && !audioPopup.opened && !subPopup.opened && !speedPopup.opened) {
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

    function handleAction(action) {
        if (!action) return false

        pokeHud()
        if (action === "play_pause") {
            if (player.hasMedia) {
                player.togglePause()
                showToast(player.paused ? "Paused" : "Playing")
            }
            return true
        } else if (action === "seek_backward_10") {
            player.seek(-10, true)
            showToast("-10s")
            return true
        } else if (action === "seek_forward_10") {
            player.seek(10, true)
            showToast("+10s")
            return true
        } else if (action === "seek_backward_5") {
            player.seek(-5, true)
            showToast("-5s")
            return true
        } else if (action === "seek_forward_5") {
            player.seek(5, true)
            showToast("+5s")
            return true
        } else if (action === "volume_up") {
            player.volume = Math.min(100.0, player.volume + 5.0)
            showToast("Volume " + Math.round(player.volume) + "%")
            return true
        } else if (action === "volume_down") {
            player.volume = Math.max(0.0, player.volume - 5.0)
            showToast("Volume " + Math.round(player.volume) + "%")
            return true
        } else if (action === "toggle_mute") {
            player.muted = !player.muted
            showToast(player.muted ? "Muted" : "Unmuted")
            return true
        } else if (action === "toggle_fullscreen") {
            if (root.visibility === Window.FullScreen) {
                root.visibility = Window.Windowed
            } else {
                root.visibility = Window.FullScreen
            }
            return true
        } else if (action === "cycle_captions") {
            player.cycleSubtitleTrack()
            showToast("Subtitles: " + (player.currentSubtitleLanguage !== "" ? player.currentSubtitleLanguage.toUpperCase() : (player.currentSubtitleTrackId > 0 ? "Track " + player.currentSubtitleTrackId : "Off")))
            return true
        } else if (action === "cycle_audio") {
            player.cycleAudioTrack()
            showToast("Audio: " + (player.currentAudioLanguage !== "" ? player.currentAudioLanguage.toUpperCase() : "Track " + player.currentAudioTrackId))
            return true
        } else if (action === "speed_increase") {
            player.adjustSpeed(0.25)
            showToast(player.speed.toFixed(2) + "x")
            return true
        } else if (action === "speed_decrease") {
            player.adjustSpeed(-0.25)
            showToast(player.speed.toFixed(2) + "x")
            return true
        } else if (action === "speed_reset") {
            player.resetSpeed()
            showToast("1.00x")
            return true
        } else if (action === "back_or_exit_fullscreen") {
            if (root.visibility === Window.FullScreen) {
                root.visibility = Window.Windowed
                return true
            }
        }
        return false
    }

    Component.onCompleted: {
        if (initialFilePath !== "") {
            player.loadFile(initialFilePath)
        }
    }

    // Main Interactive Video Area
    Item {
        id: videoContainer
        anchors.fill: parent
        focus: true

        MpvPlayer {
            id: player
            objectName: "mpvPlayer"
            anchors.fill: parent

            onResumed: function(resumePos, origPos) {
                root.showToast("Resumed at " + root.formatTime(resumePos) + " (rewound 2.5s for context)")
            }

            Component.onCompleted: {
                if (Qt.application.arguments.indexOf("--benchmark-mode") !== -1) {
                    Qt.callLater(function() {
                        Qt.quit()
                    })
                }
            }
        }

        // Empty state overlay
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
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
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

        // Mouse activity & cursor auto-hide monitor
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            cursorShape: (root.hudVisible || !player.hasMedia) ? Qt.ArrowCursor : Qt.BlankCursor

            onPositionChanged: root.pokeHud()
            onClicked: {
                if (player.hasMedia) {
                    player.togglePause()
                    root.showToast(player.paused ? "Paused" : "Playing")
                    root.pokeHud()
                }
            }
        }

        // Central Key Dispatcher matching KeybindManager actions
        Keys.onPressed: function(event) {
            var keyName = ""
            if (event.key === Qt.Key_Space) keyName = "Space"
            else if (event.key === Qt.Key_Left) keyName = "Left"
            else if (event.key === Qt.Key_Right) keyName = "Right"
            else if (event.key === Qt.Key_Up) keyName = "Up"
            else if (event.key === Qt.Key_Down) keyName = "Down"
            else if (event.key === Qt.Key_Escape) keyName = "Escape"
            else if (event.key === Qt.Key_Backspace) keyName = "Backspace"
            else if (event.key === Qt.Key_BracketLeft) keyName = "["
            else if (event.key === Qt.Key_BracketRight) keyName = "]"
            else if (event.text !== "") keyName = event.text.toUpperCase()

            if (keyName !== "") {
                var action = keybindManager.matchAction(keyName)
                if (action !== "") {
                    if (root.handleAction(action)) {
                        event.accepted = true
                        return
                    }
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
        opacity: root.hudVisible ? 0.96 : 0.0
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
        height: 80
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
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            spacing: 6

            // Progress Scrubber Bar Area
            Item {
                id: progressBarArea
                Layout.fillWidth: true
                Layout.preferredHeight: 18

                // Progress Track Background
                Rectangle {
                    id: progressTrack
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: progressMouseArea.containsMouse ? 6 : 3
                    color: "#232733"
                    radius: 2

                    Behavior on height {
                        NumberAnimation { duration: 120 }
                    }

                    // Buffer (demuxer cache) Region
                    Rectangle {
                        id: bufferFill
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: player.duration > 0 ? parent.width * Math.max(0.0, Math.min(1.0, player.bufferPosition / player.duration)) : 0
                        color: "#323846"
                        radius: 2
                    }

                    // Watched / Elapsed Fill
                    Rectangle {
                        id: progressFill
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: player.duration > 0 ? parent.width * Math.max(0.0, Math.min(1.0, (player.position / player.duration))) : 0
                        color: "#d97736"
                        radius: 2
                    }

                    // Chapter Ticks
                    Repeater {
                        model: player.chapters
                        Rectangle {
                            width: 2
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            color: "#090a0d"
                            x: player.duration > 0 ? (modelData.time / player.duration) * progressTrack.width : 0
                            visible: x > 0 && x < progressTrack.width
                        }
                    }

                    // Scrubber Thumb Handle (visible on hover or drag)
                    Rectangle {
                        id: scrubberThumb
                        width: 12
                        height: 12
                        radius: 6
                        color: "#e6e8ec"
                        border.color: "#d97736"
                        border.width: 2
                        anchors.verticalCenter: parent.verticalCenter
                        x: Math.max(0, Math.min(parent.width - width, (progressFill.width - width / 2)))
                        visible: progressMouseArea.containsMouse || progressMouseArea.pressed
                    }
                }

                // Interactive Scrubber Mouse Area
                MouseArea {
                    id: progressMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    preventStealing: true

                    function seekTo(mouse) {
                        if (player.duration > 0) {
                            var ratio = Math.max(0.0, Math.min(1.0, mouse.x / width))
                            player.seek(ratio * player.duration, false)
                            root.pokeHud()
                        }
                    }

                    onPressed: function(mouse) {
                        seekTo(mouse)
                    }

                    onPositionChanged: function(mouse) {
                        if (pressed) {
                            seekTo(mouse)
                        }
                    }
                }

                // Hover Time Tooltip
                Rectangle {
                    id: timeTooltip
                    visible: progressMouseArea.containsMouse && player.duration > 0
                    width: tooltipText.implicitWidth + 12
                    height: 22
                    color: "#191c24"
                    border.color: "#232733"
                    border.width: 1
                    radius: 4
                    anchors.bottom: progressTrack.top
                    anchors.bottomMargin: 8
                    x: Math.max(4, Math.min(parent.width - width - 4, progressMouseArea.mouseX - width / 2))

                    Text {
                        id: tooltipText
                        anchors.centerIn: parent
                        text: {
                            if (player.duration <= 0) return "00:00"
                            var ratio = Math.max(0.0, Math.min(1.0, progressMouseArea.mouseX / progressBarArea.width))
                            return root.formatTime(ratio * player.duration)
                        }
                        color: "#e6e8ec"
                        font.pixelSize: 11
                        font.features: { "tnum": 1 }
                    }
                }
            }

            // Controls & Options Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                // Play / Pause Button
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
                        root.showToast(player.paused ? "Paused" : "Playing")
                        root.pokeHud()
                    }
                }

                // Timecode Readout
                Text {
                    text: root.formatTime(player.position) + " / " + root.formatTime(player.duration)
                    color: "#9aa0ac"
                    font.pixelSize: 12
                    font.features: { "tnum": 1 }
                }

                Item { Layout.fillWidth: true }

                // Speed Selector Button & Popover
                Button {
                    id: speedBtn
                    Layout.preferredHeight: 32
                    background: Rectangle {
                        color: speedBtn.hovered || speedPopup.opened ? "#1c1f26" : "transparent"
                        radius: 4
                    }
                    contentItem: RowLayout {
                        spacing: 4
                        anchors.centerIn: parent
                        Image {
                            source: "qrc:/assets/icons/speed.svg"
                            sourceSize.width: 16
                            sourceSize.height: 16
                        }
                        Text {
                            text: player.speed.toFixed(2) + "x"
                            color: player.speed !== 1.0 ? "#d97736" : "#9aa0ac"
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }
                    }
                    onClicked: {
                        speedPopup.open()
                        root.pokeHud()
                    }
                }

                // Audio Track Button & Popover
                Button {
                    id: audioBtn
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    background: Rectangle {
                        color: audioBtn.hovered || audioPopup.opened ? "#1c1f26" : "transparent"
                        radius: 4
                    }
                    contentItem: Image {
                        source: "qrc:/assets/icons/audio.svg"
                        sourceSize.width: 18
                        sourceSize.height: 18
                        anchors.centerIn: parent
                        opacity: player.audioTracks.length > 0 ? 1.0 : 0.4
                    }
                    onClicked: {
                        if (player.audioTracks.length > 0) {
                            audioPopup.open()
                            root.pokeHud()
                        }
                    }
                }

                // Subtitles Button & Popover
                Button {
                    id: subBtn
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    background: Rectangle {
                        color: subBtn.hovered || subPopup.opened ? "#1c1f26" : "transparent"
                        radius: 4
                    }
                    contentItem: Image {
                        source: "qrc:/assets/icons/subtitles.svg"
                        sourceSize.width: 18
                        sourceSize.height: 18
                        anchors.centerIn: parent
                        opacity: player.currentSubtitleTrackId > 0 ? 1.0 : 0.4
                    }
                    onClicked: {
                        subPopup.open()
                        root.pokeHud()
                    }
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
                        opacity: player.muted ? 0.35 : 1.0
                    }
                    onClicked: {
                        player.muted = !player.muted
                        root.showToast(player.muted ? "Muted" : "Unmuted")
                        root.pokeHud()
                    }
                }

                // Fullscreen Toggle
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

    // Audio Track Selection Popover
    Popup {
        id: audioPopup
        x: Math.max(8, bottomHud.x + audioBtn.x - width / 2 + audioBtn.width / 2)
        y: bottomHud.y - height - 8
        width: 220
        padding: 8
        background: Rectangle {
            color: "#222631"
            border.color: "#232733"
            border.width: 1
            radius: 6
        }

        contentItem: ColumnLayout {
            spacing: 4

            Text {
                text: "AUDIO TRACKS"
                color: "#9aa0ac"
                font.pixelSize: 10
                font.weight: Font.DemiBold
                font.letterSpacing: 0.8
                Layout.leftMargin: 8
                Layout.topMargin: 4
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#232733"
            }

            Repeater {
                model: player.audioTracks
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 32
                    radius: 4
                    color: audioItemArea.containsMouse ? "#1c1f26" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Text {
                            text: modelData.title
                            color: modelData.id === player.currentAudioTrackId ? "#d97736" : "#e6e8ec"
                            font.pixelSize: 13
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Image {
                            source: "qrc:/assets/icons/checkmark.svg"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            visible: modelData.id === player.currentAudioTrackId
                        }
                    }

                    MouseArea {
                        id: audioItemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            player.setAudioTrack(modelData.id)
                            root.showToast("Audio: " + modelData.title)
                            audioPopup.close()
                        }
                    }
                }
            }
        }
    }

    // Subtitles Selection Popover
    Popup {
        id: subPopup
        x: Math.max(8, bottomHud.x + subBtn.x - width / 2 + subBtn.width / 2)
        y: bottomHud.y - height - 8
        width: 220
        padding: 8
        background: Rectangle {
            color: "#222631"
            border.color: "#232733"
            border.width: 1
            radius: 6
        }

        contentItem: ColumnLayout {
            spacing: 4

            Text {
                text: "SUBTITLES"
                color: "#9aa0ac"
                font.pixelSize: 10
                font.weight: Font.DemiBold
                font.letterSpacing: 0.8
                Layout.leftMargin: 8
                Layout.topMargin: 4
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#232733"
            }

            // Option: Off
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 32
                radius: 4
                color: subOffArea.containsMouse ? "#1c1f26" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8

                    Text {
                        text: "Off"
                        color: player.currentSubtitleTrackId <= 0 ? "#d97736" : "#e6e8ec"
                        font.pixelSize: 13
                        Layout.fillWidth: true
                    }

                    Image {
                        source: "qrc:/assets/icons/checkmark.svg"
                        sourceSize.width: 14
                        sourceSize.height: 14
                        visible: player.currentSubtitleTrackId <= 0
                    }
                }

                MouseArea {
                    id: subOffArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        player.setSubtitleTrack(-1)
                        root.showToast("Subtitles: Off")
                        subPopup.close()
                    }
                }
            }

            Repeater {
                model: player.subtitleTracks
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 32
                    radius: 4
                    color: subItemArea.containsMouse ? "#1c1f26" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Text {
                            text: modelData.title
                            color: modelData.id === player.currentSubtitleTrackId ? "#d97736" : "#e6e8ec"
                            font.pixelSize: 13
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Image {
                            source: "qrc:/assets/icons/checkmark.svg"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            visible: modelData.id === player.currentSubtitleTrackId
                        }
                    }

                    MouseArea {
                        id: subItemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            player.setSubtitleTrack(modelData.id)
                            root.showToast("Subtitles: " + modelData.title)
                            subPopup.close()
                        }
                    }
                }
            }
        }
    }

    // Playback Speed Popover
    Popup {
        id: speedPopup
        x: Math.max(8, bottomHud.x + speedBtn.x - width / 2 + speedBtn.width / 2)
        y: bottomHud.y - height - 8
        width: 180
        padding: 8
        background: Rectangle {
            color: "#222631"
            border.color: "#232733"
            border.width: 1
            radius: 6
        }

        contentItem: ColumnLayout {
            spacing: 4

            Text {
                text: "PLAYBACK SPEED"
                color: "#9aa0ac"
                font.pixelSize: 10
                font.weight: Font.DemiBold
                font.letterSpacing: 0.8
                Layout.leftMargin: 8
                Layout.topMargin: 4
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#232733"
            }

            Repeater {
                model: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0]
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 30
                    radius: 4
                    color: speedItemArea.containsMouse ? "#1c1f26" : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8

                        Text {
                            text: modelData.toFixed(2) + "x"
                            color: Math.abs(player.speed - modelData) < 0.05 ? "#d97736" : "#e6e8ec"
                            font.pixelSize: 13
                            font.weight: Math.abs(player.speed - modelData) < 0.05 ? Font.DemiBold : Font.Normal
                            Layout.fillWidth: true
                        }

                        Image {
                            source: "qrc:/assets/icons/checkmark.svg"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            visible: Math.abs(player.speed - modelData) < 0.05
                        }
                    }

                    MouseArea {
                        id: speedItemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            player.speed = modelData
                            root.showToast(modelData.toFixed(2) + "x")
                            speedPopup.close()
                        }
                    }
                }
            }
        }
    }

    // Feedback Toast (Pill HUD in top center)
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 72
        width: toastContent.implicitWidth + 28
        height: 32
        radius: 16
        color: "#12141a"
        border.color: "#232733"
        border.width: 1
        opacity: root.toastVisible ? 0.95 : 0.0
        visible: opacity > 0.0

        Behavior on opacity {
            NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
        }

        Text {
            id: toastContent
            anchors.centerIn: parent
            text: root.toastText
            color: "#e6e8ec"
            font.pixelSize: 12
            font.weight: Font.Medium
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
