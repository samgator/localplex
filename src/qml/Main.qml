// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import localplex.player 1.0
import localplex.core 1.0
import localplex.database 1.0
import localplex.scanner 1.0
import localplex.models 1.0

ApplicationWindow {
    id: root
    title: "localplex"
    width: 1180
    height: 740
    minimumWidth: 860
    minimumHeight: 540
    color: "#090a0d"
    visible: true

    FontLoader {
        id: interFont
        source: "qrc:/assets/fonts/Inter-Variable.ttf"
    }

    font.family: interFont.status === FontLoader.Ready ? interFont.name : "sans-serif"

    // View Routing: "library", "show_detail", "player"
    property string activeView: "library"
    property string selectedShowTitle: ""
    property int selectedSeasonNumber: 1
    property var selectedShowEpisodes: []
    property var selectedShowSeasons: []

    // Player HUD state
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
            if (activeView === "player" && player.hasMedia && !player.paused &&
                !audioPopup.opened && !subPopup.opened && !speedPopup.opened) {
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

    function playFile(filePath) {
        player.loadFile(filePath)
        activeView = "player"
        pokeHud()
    }

    function openShowDetails(title) {
        selectedShowTitle = title
        selectedShowSeasons = libraryDatabase.getSeasons(title)
        if (selectedShowSeasons.length > 0) {
            selectedSeasonNumber = selectedShowSeasons[0]
            selectedShowEpisodes = libraryDatabase.getEpisodes(title, selectedSeasonNumber)
        } else {
            selectedShowEpisodes = []
        }
        activeView = "show_detail"
    }

    function selectSeason(seasonNum) {
        selectedSeasonNumber = seasonNum
        selectedShowEpisodes = libraryDatabase.getEpisodes(selectedShowTitle, seasonNum)
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
            } else if (activeView === "player") {
                activeView = "library"
                return true
            } else if (activeView === "show_detail") {
                activeView = "library"
                return true
            } else if (activeView === "fixup") {
                activeView = "library"
                return true
            }
        }
        return false
    }

    Component.onCompleted: {
        if (initialFilePath !== "") {
            playFile(initialFilePath)
        }
    }

    // Models
    CatalogModel {
        id: catalogModel
    }

    ContinueWatchingModel {
        id: continueWatchingModel
    }

    // Top Navigation Header
    Rectangle {
        id: topNav
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 54
        color: "#12141a"
        border.color: "#232733"
        border.width: 1
        z: 10
        visible: activeView !== "player" || root.hudVisible

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 16

            // Brand / Back button
            RowLayout {
                spacing: 10
                Button {
                    visible: activeView !== "library"
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    background: Rectangle {
                        color: parent.hovered ? "#1c1f26" : "transparent"
                        radius: 4
                    }
                    contentItem: Image {
                        source: "qrc:/assets/icons/arrow-left.svg"
                        sourceSize.width: 18
                        sourceSize.height: 18
                        anchors.centerIn: parent
                    }
                    onClicked: {
                        if (activeView === "player") {
                            activeView = selectedShowTitle !== "" ? "show_detail" : "library"
                        } else {
                            activeView = "library"
                        }
                    }
                }

                Text {
                    text: "localplex"
                    color: "#9aa0ac"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                }
            }

            Rectangle {
                width: 1
                height: 16
                color: "#232733"
                visible: activeView === "library"
            }

            // Section Filter Tabs (All / Shows / Movies)
            RowLayout {
                visible: activeView === "library"
                spacing: 4

                Repeater {
                    model: [
                        { label: "All", key: "all" },
                        { label: "Shows", key: "shows" },
                        { label: "Movies", key: "movies" }
                    ]
                    Button {
                        text: modelData.label
                        Layout.preferredHeight: 28
                        background: Rectangle {
                            color: catalogModel.section === modelData.key ? "#1c1f26" : "transparent"
                            border.color: catalogModel.section === modelData.key ? "#232733" : "transparent"
                            border.width: 1
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: catalogModel.section === modelData.key ? "#e6e8ec" : "#9aa0ac"
                            font.pixelSize: 12
                            font.weight: catalogModel.section === modelData.key ? Font.DemiBold : Font.Normal
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: catalogModel.section = modelData.key
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Search Box (Library View)
            Rectangle {
                id: searchBox
                visible: activeView === "library"
                Layout.preferredWidth: 200
                Layout.preferredHeight: 30
                color: "#090a0d"
                border.color: searchInput.activeFocus ? "#f3f4f6" : "#232733"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    Image {
                        source: "qrc:/assets/icons/search.svg"
                        sourceSize.width: 14
                        sourceSize.height: 14
                        opacity: 0.6
                    }

                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        color: "#e6e8ec"
                        font.pixelSize: 12
                        clip: true
                        onTextChanged: catalogModel.searchQuery = text

                        Text {
                            text: "Search library..."
                            color: "#5c6370"
                            font.pixelSize: 12
                            visible: !searchInput.text && !searchInput.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            // Filter Pill (Unwatched toggle)
            Button {
                visible: activeView === "library"
                text: catalogModel.filter === "unwatched" ? "Unwatched only" : "Filter"
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: catalogModel.filter === "unwatched" ? "#d97736" : (parent.hovered ? "#1c1f26" : "transparent")
                    border.color: catalogModel.filter === "unwatched" ? "#d97736" : "#232733"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: catalogModel.filter === "unwatched" ? "#090a0d" : "#e6e8ec"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    catalogModel.filter = (catalogModel.filter === "unwatched") ? "all" : "unwatched"
                }
            }

            // Add Folder Button
            Button {
                visible: activeView === "library" || activeView === "fixup"
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: parent.hovered ? "#1c1f26" : "#12141a"
                    border.color: "#232733"
                    border.width: 1
                    radius: 4
                }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Image {
                        source: "qrc:/assets/icons/plus.svg"
                        sourceSize.width: 14
                        sourceSize.height: 14
                    }
                    Text {
                        text: "Add folder"
                        color: "#e6e8ec"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                }
                onClicked: folderDialog.open()
            }

            // Fix-up Button
            Button {
                visible: activeView === "library" || activeView === "fixup"
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: activeView === "fixup" ? "#2563eb" : (parent.hovered ? "#1c1f26" : "#12141a")
                    border.color: activeView === "fixup" ? "#3b82f6" : "#232733"
                    border.width: 1
                    radius: 4
                }
                contentItem: RowLayout {
                    spacing: 6
                    anchors.centerIn: parent
                    Image {
                        source: "qrc:/assets/icons/edit.svg"
                        sourceSize.width: 14
                        sourceSize.height: 14
                    }
                    Text {
                        text: "Fix-up"
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                }
                onClicked: {
                    activeView = (activeView === "fixup" ? "library" : "fixup")
                    if (activeView === "fixup" && fixupLoader.item) {
                        fixupLoader.item.reloadData()
                    }
                }
            }
        }
    }

    // Scanning progress banner
    Rectangle {
        id: scanBanner
        anchors.top: topNav.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 28
        color: "#191c24"
        border.color: "#232733"
        border.width: 1
        visible: libraryScanner.isScanning && activeView !== "player"
        z: 9

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 12

            Text {
                text: libraryScanner.statusMessage
                color: "#e6e8ec"
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Text {
                text: libraryScanner.progressPercent + "%"
                color: "#d97736"
                font.pixelSize: 11
                font.features: { "tnum": 1 }
            }
        }
    }

    // 1. Library View
    ScrollView {
        id: libraryScroll
        anchors.top: scanBanner.visible ? scanBanner.bottom : topNav.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: activeView === "library"
        clip: true

        ColumnLayout {
            width: libraryScroll.width
            spacing: 28

            Item { Layout.preferredHeight: 4 }

            // Empty Library Placeholder
            Rectangle {
                visible: catalogModel.count === 0 && !libraryScanner.isScanning
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 420
                Layout.preferredHeight: 260
                color: "transparent"

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 16

                    Text {
                        text: "No media folders connected"
                        color: "#e6e8ec"
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Add a folder containing movies or shows to begin building your offline catalog."
                        color: "#9aa0ac"
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: 360
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Item { Layout.preferredHeight: 8 }

                    Button {
                        text: "Add folder"
                        Layout.alignment: Qt.AlignHCenter
                        background: Rectangle {
                            implicitWidth: 130
                            implicitHeight: 36
                            color: parent.down ? "#222631" : (parent.hovered ? "#191c24" : "#12141a")
                            border.color: "#232733"
                            border.width: 1
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
                        onClicked: folderDialog.open()
                    }
                }
            }

            // Continue Watching Shelf
            ColumnLayout {
                visible: continueWatchingModel.count > 0
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                spacing: 12

                Text {
                    text: "CONTINUE WATCHING"
                    color: "#9aa0ac"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.0
                }

                ListView {
                    id: continueWatchingList
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180
                    orientation: ListView.Horizontal
                    spacing: 16
                    clip: true
                    model: continueWatchingModel

                    delegate: Rectangle {
                        width: 240
                        height: 170
                        color: "#12141a"
                        border.color: mouseAreaShelf.containsMouse ? "#d97736" : "#232733"
                        border.width: 1
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // 16:9 Thumbnail Area
                            Item {
                                Layout.preferredWidth: 240
                                Layout.preferredHeight: 125
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    source: model.thumbnailPath ? "file://" + model.thumbnailPath : ""
                                    fillMode: Image.PreserveAspectCrop
                                    visible: model.thumbnailPath !== ""
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    color: "#191c24"
                                    visible: !model.thumbnailPath

                                    Image {
                                        source: "qrc:/assets/icons/play.svg"
                                        sourceSize.width: 28
                                        sourceSize.height: 28
                                        anchors.centerIn: parent
                                        opacity: 0.3
                                    }
                                }

                                // Shelf type pill (e.g. Next Up)
                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.margins: 8
                                    height: 18
                                    width: badgeText.implicitWidth + 10
                                    color: "#090a0d"
                                    radius: 2
                                    opacity: 0.9

                                    Text {
                                        id: badgeText
                                        anchors.centerIn: parent
                                        text: model.shelfType === "next_up" ? "NEXT UP" : "RESUME"
                                        color: model.shelfType === "next_up" ? "#d97736" : "#e6e8ec"
                                        font.pixelSize: 9
                                        font.weight: Font.DemiBold
                                        font.letterSpacing: 0.5
                                    }
                                }

                                // Scrubber bar at bottom of thumbnail
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    height: 3
                                    color: "#232733"
                                    visible: model.duration > 0 && model.position > 0

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        width: parent.width * Math.min(1.0, (model.position / model.duration))
                                        color: "#d97736"
                                    }
                                }
                            }

                            // Meta readout
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.margins: 8
                                spacing: 2

                                Text {
                                    text: model.title
                                    color: "#e6e8ec"
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: model.subtitle
                                    color: "#9aa0ac"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        MouseArea {
                            id: mouseAreaShelf
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.playFile(model.filePath)
                        }
                    }
                }
            }

            // Main Catalog Grid (Virtualized)
            ColumnLayout {
                visible: catalogModel.count > 0
                Layout.fillWidth: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                spacing: 12

                Text {
                    text: catalogModel.section === "shows" ? "TELEVISION SHOWS" : (catalogModel.section === "movies" ? "FEATURE FILMS" : "CATALOG")
                    color: "#9aa0ac"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.0
                }

                GridView {
                    id: catalogGrid
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.ceil(catalogModel.count / Math.max(1, Math.floor(width / cellWidth))) * cellHeight
                    cellWidth: 180
                    cellHeight: 280
                    clip: true
                    interactive: false
                    model: catalogModel

                    delegate: Item {
                        width: catalogGrid.cellWidth
                        height: catalogGrid.cellHeight

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 8
                            color: "#12141a"
                            border.color: mouseAreaCard.containsMouse ? "#d97736" : "#232733"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0

                                // 2:3 Standard Poster Viewport
                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 200
                                    clip: true

                                    Image {
                                        anchors.fill: parent
                                        source: model.posterPath ? "file://" + model.posterPath : ""
                                        fillMode: Image.PreserveAspectCrop
                                        visible: model.posterPath !== ""
                                    }

                                    // Typographic Poster Fallback
                                    Rectangle {
                                        anchors.fill: parent
                                        color: "#191c24"
                                        visible: !model.posterPath

                                        ColumnLayout {
                                            anchors.centerIn: parent
                                            anchors.margins: 12
                                            spacing: 6

                                            Text {
                                                text: model.title
                                                color: "#e6e8ec"
                                                font.pixelSize: 14
                                                font.weight: Font.DemiBold
                                                wrapMode: Text.WordWrap
                                                horizontalAlignment: Text.AlignHCenter
                                                Layout.fillWidth: true
                                                maximumLineCount: 3
                                                elide: Text.ElideRight
                                            }

                                            Text {
                                                text: model.year > 0 ? model.year : ""
                                                color: "#9aa0ac"
                                                font.pixelSize: 11
                                                Layout.alignment: Qt.AlignHCenter
                                            }
                                        }
                                    }

                                    // Unwatched Badge for Shows
                                    Rectangle {
                                        visible: model.mediaType === "show" && model.unwatchedCount > 0
                                        anchors.top: parent.top
                                        anchors.right: parent.right
                                        anchors.margins: 6
                                        height: 18
                                        width: unwatchedBadgeText.implicitWidth + 10
                                        color: "#090a0d"
                                        radius: 2
                                        opacity: 0.9

                                        Text {
                                            id: unwatchedBadgeText
                                            anchors.centerIn: parent
                                            text: model.unwatchedCount + " NEW"
                                            color: "#d97736"
                                            font.pixelSize: 9
                                            font.weight: Font.DemiBold
                                            font.letterSpacing: 0.5
                                        }
                                    }
                                }

                                // Card Title & Subtitle Info
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.margins: 8
                                    spacing: 2

                                    Text {
                                        text: model.title
                                        color: "#e6e8ec"
                                        font.pixelSize: 12
                                        font.weight: Font.Medium
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: model.subtitle
                                        color: "#9aa0ac"
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            MouseArea {
                                id: mouseAreaCard
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (model.mediaType === "show") {
                                        root.openShowDetails(model.title)
                                    } else {
                                        root.playFile(model.filePath)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 24 }
        }
    }

    // 2. Show Detail View
    ScrollView {
        id: showDetailScroll
        anchors.top: topNav.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: activeView === "show_detail"
        clip: true

        ColumnLayout {
            width: showDetailScroll.width
            spacing: 24
            Layout.margins: 24

            Item { Layout.preferredHeight: 8 }

            // Show Header Info
            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Rectangle {
                    width: 140
                    height: 210
                    color: "#191c24"
                    border.color: "#232733"
                    border.width: 1
                    radius: 4

                    Image {
                        source: "qrc:/assets/icons/play.svg"
                        sourceSize.width: 32
                        sourceSize.height: 32
                        anchors.centerIn: parent
                        opacity: 0.3
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: root.selectedShowTitle
                        color: "#e6e8ec"
                        font.pixelSize: 26
                        font.weight: Font.DemiBold
                    }

                    Text {
                        text: root.selectedShowSeasons.length + " Season(s) · " + root.selectedShowEpisodes.length + " Episode(s)"
                        color: "#9aa0ac"
                        font.pixelSize: 13
                    }
                }
            }

            // Season Tabs
            RowLayout {
                spacing: 8
                Repeater {
                    model: root.selectedShowSeasons
                    Button {
                        text: "Season " + modelData
                        background: Rectangle {
                            color: root.selectedSeasonNumber === modelData ? "#d97736" : (parent.hovered ? "#1c1f26" : "#12141a")
                            border.color: root.selectedSeasonNumber === modelData ? "#d97736" : "#232733"
                            border.width: 1
                            radius: 4
                            implicitHeight: 32
                            implicitWidth: 100
                        }
                        contentItem: Text {
                            text: parent.text
                            color: root.selectedSeasonNumber === modelData ? "#090a0d" : "#e6e8ec"
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: root.selectSeason(modelData)
                    }
                }
            }

            // Episodes List
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: root.selectedShowEpisodes
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        color: epArea.containsMouse ? "#1c1f26" : "#12141a"
                        border.color: "#232733"
                        border.width: 1
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 16

                            Text {
                                text: modelData.episodeNumber < 10 ? "0" + modelData.episodeNumber : "" + modelData.episodeNumber
                                color: "#9aa0ac"
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                font.features: { "tnum": 1 }
                                Layout.preferredWidth: 32
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: modelData.itemTitle
                                    color: "#e6e8ec"
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                }

                                Text {
                                    text: modelData.completed ? "Watched" : (modelData.position > 10 ? "Resume from " + root.formatTime(modelData.position) : "Unwatched")
                                    color: modelData.completed ? "#9aa0ac" : (modelData.position > 10 ? "#d97736" : "#5c6370")
                                    font.pixelSize: 11
                                }
                            }

                            Button {
                                text: "Play"
                                Layout.preferredHeight: 28
                                background: Rectangle {
                                    color: parent.hovered ? "#d97736" : "transparent"
                                    border.color: "#232733"
                                    border.width: 1
                                    radius: 4
                                    implicitWidth: 64
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: parent.hovered ? "#090a0d" : "#e6e8ec"
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: root.playFile(modelData.filePath)
                            }
                        }

                        MouseArea {
                            id: epArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.playFile(modelData.filePath)
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 32 }
        }
    }

    // Fix-up / Corrections View (Rule 4 compliant metadata overrides)
    Loader {
        id: fixupLoader
        anchors.top: scanBanner.visible ? scanBanner.bottom : topNav.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        active: activeView === "fixup"
        visible: activeView === "fixup"
        source: "qrc:/src/qml/FixupView.qml"
        onLoaded: {
            item.backRequested.connect(function() { activeView = "library"; });
            item.notifyUser.connect(function(msg) { root.showToast(msg); });
            item.reloadData();
        }
    }

    // 3. Player View (Active Video Pipeline)
    Item {
        id: videoContainer
        anchors.fill: parent
        visible: activeView === "player"
        focus: activeView === "player"

        MpvPlayer {
            id: player
            objectName: "mpvPlayer"
            anchors.fill: parent

            onResumed: function(resumePos, origPos) {
                root.showToast("Resumed at " + root.formatTime(resumePos) + " (rewound 2.5s for context)")
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

        // Central Key Dispatcher
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

                        // Scrubber Thumb Handle
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

                        onPressed: function(mouse) { seekTo(mouse) }
                        onPositionChanged: function(mouse) { if (pressed) seekTo(mouse) }
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

                // Controls Row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

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

                    Text {
                        text: root.formatTime(player.position) + " / " + root.formatTime(player.duration)
                        color: "#9aa0ac"
                        font.pixelSize: 12
                        font.features: { "tnum": 1 }
                    }

                    Item { Layout.fillWidth: true }

                    // Speed Popover Button
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

                    // Audio Popover Button
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

                    // Subtitles Popover Button
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

                    // Volume Button
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

                    // Fullscreen Button
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

    // Feedback Toast (Top Center Pill HUD)
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: activeView === "player" ? 72 : 64
        width: toastContent.implicitWidth + 28
        height: 32
        radius: 16
        color: "#12141a"
        border.color: "#232733"
        border.width: 1
        opacity: root.toastVisible ? 0.95 : 0.0
        visible: opacity > 0.0
        z: 20

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

    // Folder Picker Dialog
    FolderDialog {
        id: folderDialog
        title: "Select media folder to add"
        onAccepted: {
            var path = selectedFolder.toString()
            if (path.indexOf("file://") === 0) {
                path = path.substring(7)
            }
            libraryDatabase.addFolder(path)
            libraryScanner.startScan()
            root.showToast("Folder added. Scanning media...")
        }
    }

    // Drag-and-Drop Import Support
    DropArea {
        id: windowDropArea
        anchors.fill: parent
        z: 999

        property bool isDragging: false

        onEntered: function(drag) {
            if (drag.hasUrls) {
                drag.acceptProposedAction()
                windowDropArea.isDragging = true
            }
        }
        onExited: {
            windowDropArea.isDragging = false
        }
        onDropped: function(drop) {
            windowDropArea.isDragging = false
            if (!drop.hasUrls) return

            var folderCount = 0
            var filePlayed = false

            for (var i = 0; i < drop.urls.length; ++i) {
                var urlStr = drop.urls[i].toString()
                var localPath = urlStr
                if (localPath.indexOf("file://") === 0) {
                    localPath = decodeURIComponent(localPath.substring(7))
                }
                if (libraryDatabase.isDirectory(localPath)) {
                    libraryDatabase.addFolder(localPath)
                    folderCount++
                } else {
                    if (!filePlayed && (activeView === "player" || drop.urls.length === 1)) {
                        root.playFile(localPath)
                        filePlayed = true
                    }
                }
            }

            if (folderCount > 0) {
                libraryScanner.startScan()
                root.showToast("Imported " + folderCount + " folder" + (folderCount > 1 ? "s" : "") + ". Scanning media...")
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "#e6090a0d"
            visible: windowDropArea.isDragging
            border.color: "#3b82f6"
            border.width: 2

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 14

                Image {
                    source: "qrc:/assets/icons/folder-open.svg"
                    sourceSize.width: 48
                    sourceSize.height: 48
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Drop media folder to import or video file to play"
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Rule 4: Your files remain untouched. Metadata is cataloged locally in SQLite."
                    color: "#94a3b8"
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }
}
