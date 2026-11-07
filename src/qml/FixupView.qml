// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import localplex.database 1.0
import localplex.core 1.0

Rectangle {
    id: fixupViewRoot
    color: themeManager.backgroundDeep

    signal backRequested()
    signal notifyUser(string message)

    property string searchQuery: ""
    property string typeFilter: "all" // "all", "episode", "movie"
    property var allItems: []
    property var selectedPathMap: ({})
    property int selectedCount: 0
    property string activeTab: "assign_show" // "assign_show", "assign_movie", "merge_shows", "split_show"

    function reloadData() {
        allItems = libraryDatabase.getAllItemsForFixup(searchQuery, typeFilter)
        updateSelectedCount()
    }

    function toggleSelect(path) {
        var copy = Object.assign({}, selectedPathMap)
        if (copy[path]) {
            delete copy[path]
        } else {
            copy[path] = true
        }
        selectedPathMap = copy
        updateSelectedCount()
    }

    function selectAll() {
        var copy = {}
        for (var i = 0; i < allItems.length; ++i) {
            copy[allItems[i].filePath] = true
        }
        selectedPathMap = copy
        updateSelectedCount()
    }

    function clearSelection() {
        selectedPathMap = {}
        updateSelectedCount()
    }

    function getSelectedPaths() {
        var paths = []
        // Keep order matching the items list
        for (var i = 0; i < allItems.length; ++i) {
            var p = allItems[i].filePath
            if (selectedPathMap[p]) {
                paths.push(p)
            }
        }
        return paths
    }

    function updateSelectedCount() {
        var count = 0
        for (var k in selectedPathMap) {
            if (selectedPathMap[k]) count++
        }
        selectedCount = count
    }

    Component.onCompleted: {
        reloadData()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Sub-header & Notice Banner
        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: themeManager.surfaceCard
            border.color: themeManager.borderDivider
            border.width: themeManager.borderWidth

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 12

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: themeManager.accent
                }

                Text {
                    text: "Rule 4 Compliance: Metadata corrections are stored exclusively in localplex SQLite database. Files on disk are NEVER touched or renamed."
                    color: themeManager.textSecondary
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Text {
                    text: selectedCount + " selected"
                    color: selectedCount > 0 ? themeManager.accent : themeManager.textDim
                    font.pixelSize: 12
                    font.weight: Font.SemiBold
                }

                Button {
                    text: "Select All"
                    visible: allItems.length > 0
                    Layout.preferredHeight: 28
                    background: Rectangle {
                        color: parent.hovered ? themeManager.surfacePopover : themeManager.surfaceElevated
                        border.color: themeManager.borderDivider
                        border.width: themeManager.borderWidth
                        radius: themeManager.radiusSmall
                    }
                    contentItem: Text {
                        text: parent.text
                        color: themeManager.textPrimary
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: selectAll()
                }

                Button {
                    text: "Clear"
                    visible: selectedCount > 0
                    Layout.preferredHeight: 28
                    background: Rectangle {
                        color: parent.hovered ? themeManager.surfacePopover : themeManager.surfaceElevated
                        border.color: themeManager.borderDivider
                        border.width: themeManager.borderWidth
                        radius: themeManager.radiusSmall
                    }
                    contentItem: Text {
                        text: parent.text
                        color: themeManager.textSecondary
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: clearSelection()
                }
            }
        }

        // Filter and Search Toolbar
        Rectangle {
            Layout.fillWidth: true
            height: 52
            color: themeManager.surfaceElevated
            border.color: themeManager.borderDivider
            border.width: themeManager.borderWidth

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 16

                // Search Bar
                Rectangle {
                    Layout.preferredWidth: 280
                    Layout.preferredHeight: 32
                    color: themeManager.backgroundDeep
                    radius: themeManager.radiusSmall
                    border.color: searchInput.activeFocus ? themeManager.focusRing : themeManager.borderDivider
                    border.width: searchInput.activeFocus ? 2 : themeManager.borderWidth

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        Image {
                            source: "qrc:/assets/icons/search.svg"
                            sourceSize.width: 14
                            sourceSize.height: 14
                            opacity: 0.6
                        }

                        TextInput {
                            id: searchInput
                            Layout.fillWidth: true
                            color: themeManager.textPrimary
                            font.pixelSize: 12
                            clip: true
                            onTextChanged: {
                                fixupViewRoot.searchQuery = text.trim()
                                fixupViewRoot.reloadData()
                            }

                            Text {
                                text: "Search catalog or path..."
                                color: themeManager.textDim
                                font.pixelSize: 12
                                visible: !searchInput.text && !searchInput.activeFocus
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }

                // Filter Pills
                RowLayout {
                    spacing: 4

                    Repeater {
                        model: [
                            { label: "All Items", val: "all" },
                            { label: "Episodes", val: "episode" },
                            { label: "Movies", val: "movie" }
                        ]

                        Button {
                            Layout.preferredHeight: 28
                            Layout.preferredWidth: 80
                            background: Rectangle {
                                color: fixupViewRoot.typeFilter === modelData.val ? themeManager.accent : (parent.hovered ? themeManager.surfaceCard : "transparent")
                                radius: themeManager.radiusSmall
                            }
                            contentItem: Text {
                                text: modelData.label
                                color: fixupViewRoot.typeFilter === modelData.val ? "#ffffff" : themeManager.textSecondary
                                font.pixelSize: 11
                                font.weight: fixupViewRoot.typeFilter === modelData.val ? Font.DemiBold : Font.Normal
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                fixupViewRoot.typeFilter = modelData.val
                                fixupViewRoot.reloadData()
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: 80
                    background: Rectangle {
                        color: parent.hovered ? themeManager.surfaceCard : themeManager.surfaceElevated
                        border.color: themeManager.borderDivider
                        border.width: themeManager.borderWidth
                        radius: themeManager.radiusSmall
                    }
                    contentItem: Text {
                        text: "Refresh"
                        color: themeManager.textPrimary
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: reloadData()
                }
            }
        }

        // Main Body: Split between List of Items and Action Fix-up Panel
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Left: Table / ListView
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: themeManager.backgroundDeep

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Table Column Headers
                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        color: themeManager.surfaceElevated
                        border.color: themeManager.borderDivider
                        border.width: themeManager.borderWidth

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 12

                            Item { width: 24 } // Checkbox space

                            Text {
                                text: "TITLE / SHOW"
                                color: themeManager.textDim
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                Layout.preferredWidth: 220
                            }

                            Text {
                                text: "SEASON"
                                color: themeManager.textDim
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                Layout.preferredWidth: 60
                            }

                            Text {
                                text: "EPISODE"
                                color: themeManager.textDim
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                Layout.preferredWidth: 60
                            }

                            Text {
                                text: "YEAR"
                                color: themeManager.textDim
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                Layout.preferredWidth: 50
                            }

                            Text {
                                text: "TYPE"
                                color: themeManager.textDim
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                Layout.preferredWidth: 70
                            }

                            Text {
                                text: "FILE PATH"
                                color: themeManager.textDim
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                Layout.fillWidth: true
                            }
                        }
                    }

                    // Items ListView
                    ListView {
                        id: itemsList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: fixupViewRoot.allItems

                        delegate: Rectangle {
                            id: rowRect
                            width: itemsList.width
                            height: 40
                            color: isSelected ? themeManager.surfaceCard : (rowMouse.containsMouse ? themeManager.surfaceElevated : "transparent")
                            border.color: themeManager.borderDivider
                            border.width: themeManager.borderWidth

                            property var itemData: modelData
                            property bool isSelected: !!fixupViewRoot.selectedPathMap[itemData.filePath]

                            MouseArea {
                                id: rowMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: fixupViewRoot.toggleSelect(itemData.filePath)
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 16
                                anchors.rightMargin: 16
                                spacing: 12

                                // Checkbox
                                Rectangle {
                                    width: 18
                                    height: 18
                                    radius: themeManager.radiusSmall
                                    color: isSelected ? themeManager.accent : themeManager.surfaceCard
                                    border.color: isSelected ? themeManager.accent : themeManager.borderDivider
                                    border.width: 1

                                    Image {
                                        visible: isSelected
                                        source: "qrc:/assets/icons/checkmark.svg"
                                        sourceSize.width: 12
                                        sourceSize.height: 12
                                        anchors.centerIn: parent
                                    }
                                }

                                // Show / Title
                                Text {
                                    Layout.preferredWidth: 220
                                    text: itemData.showTitle ? (itemData.showTitle + " - " + itemData.itemTitle) : (itemData.itemTitle ? itemData.itemTitle : "Untitled")
                                    color: themeManager.textPrimary
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    elide: Text.ElideRight
                                }

                                // Season
                                Text {
                                    Layout.preferredWidth: 60
                                    text: itemData.seasonNumber > 0 ? ("S" + itemData.seasonNumber) : "-"
                                    color: themeManager.textSecondary
                                    font.pixelSize: 11
                                }

                                // Episode
                                Text {
                                    Layout.preferredWidth: 60
                                    text: itemData.episodeNumber > 0 ? ("E" + itemData.episodeNumber) : "-"
                                    color: themeManager.textSecondary
                                    font.pixelSize: 11
                                }

                                // Year
                                Text {
                                    Layout.preferredWidth: 50
                                    text: itemData.year > 0 ? itemData.year : "-"
                                    color: themeManager.textSecondary
                                    font.pixelSize: 11
                                }

                                // Type badge
                                Rectangle {
                                    Layout.preferredWidth: 64
                                    Layout.preferredHeight: 20
                                    radius: themeManager.radiusSmall
                                    color: itemData.mediaType === "movie" ? themeManager.badgeMovieBg : themeManager.badgeEpisodeBg
                                    border.color: itemData.mediaType === "movie" ? themeManager.badgeMovieFg : themeManager.badgeEpisodeFg
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: itemData.mediaType === "movie" ? "Movie" : "Episode"
                                        color: itemData.mediaType === "movie" ? themeManager.badgeMovieFg : themeManager.badgeEpisodeFg
                                        font.pixelSize: 10
                                        font.weight: Font.DemiBold
                                    }
                                }

                                // File path
                                Text {
                                    Layout.fillWidth: true
                                    text: itemData.filePath
                                    color: themeManager.textDim
                                    font.pixelSize: 11
                                    elide: Text.ElideMiddle
                                }
                            }
                        }

                        // Empty State
                        Rectangle {
                            anchors.fill: parent
                            visible: fixupViewRoot.allItems.length === 0
                            color: "transparent"

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 8

                                Text {
                                    text: "No items match criteria"
                                    color: themeManager.textPrimary
                                    font.pixelSize: 14
                                    font.weight: Font.Medium
                                    Layout.alignment: Qt.AlignHCenter
                                }

                                Text {
                                    text: "Add media folders or adjust search/filters"
                                    color: themeManager.textSecondary
                                    font.pixelSize: 12
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                        }
                    }
                }
            }

            // Right: Batch Action Panel
            Rectangle {
                Layout.preferredWidth: 340
                Layout.fillHeight: true
                color: themeManager.surfaceElevated
                border.color: themeManager.borderDivider
                border.width: themeManager.borderWidth

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    Text {
                        text: "Batch Fix-up Actions"
                        color: themeManager.textPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    // Mode Selection Tabs
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Repeater {
                            model: [
                                { id: "assign_show", label: "Show" },
                                { id: "assign_movie", label: "Movie" },
                                { id: "merge_shows", label: "Merge" },
                                { id: "split_show", label: "Split" }
                            ]

                            Button {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                background: Rectangle {
                                    color: fixupViewRoot.activeTab === modelData.id ? themeManager.accent : (parent.hovered ? themeManager.surfacePopover : themeManager.surfaceCard)
                                    radius: themeManager.radiusSmall
                                }
                                contentItem: Text {
                                    text: modelData.label
                                    color: fixupViewRoot.activeTab === modelData.id ? "#ffffff" : themeManager.textSecondary
                                    font.pixelSize: 11
                                    font.weight: Font.Medium
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: fixupViewRoot.activeTab = modelData.id
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: themeManager.borderDivider
                    }

                    // Tab 1: Assign Show & Season
                    ColumnLayout {
                        visible: fixupViewRoot.activeTab === "assign_show"
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: "Assign Show & Episodes"
                            color: themeManager.textPrimary
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "Show Title"
                            color: themeManager.textSecondary
                            font.pixelSize: 11
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: themeManager.surfaceCard
                            radius: themeManager.radiusSmall
                            border.color: showTitleInput.activeFocus ? themeManager.focusRing : themeManager.borderDivider
                            border.width: showTitleInput.activeFocus ? 2 : themeManager.borderWidth

                            TextInput {
                                id: showTitleInput
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                verticalAlignment: TextInput.AlignVCenter
                                color: themeManager.textPrimary
                                font.pixelSize: 12
                                text: ""
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text {
                                    text: "Season"
                                    color: themeManager.textSecondary
                                    font.pixelSize: 11
                                }
                                SpinBox {
                                    id: seasonSpin
                                    from: 1
                                    to: 99
                                    value: 1
                                    Layout.fillWidth: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text {
                                    text: "Start Episode"
                                    color: themeManager.textSecondary
                                    font.pixelSize: 11
                                }
                                SpinBox {
                                    id: startEpSpin
                                    from: 1
                                    to: 999
                                    value: 1
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        Text {
                            text: "Sequentially numbers selected files starting from Episode " + startEpSpin.value + "."
                            color: themeManager.textDim
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            enabled: fixupViewRoot.selectedCount > 0 && showTitleInput.text.trim().length > 0
                            opacity: enabled ? 1.0 : 0.5
                            background: Rectangle {
                                color: parent.enabled ? (parent.hovered ? themeManager.accentHover : themeManager.accent) : themeManager.surfaceCard
                                radius: themeManager.radiusSmall
                            }
                            contentItem: Text {
                                text: "Apply Show & Sequence"
                                color: "#ffffff"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                var paths = fixupViewRoot.getSelectedPaths()
                                var title = showTitleInput.text.trim()
                                var season = seasonSpin.value
                                var startEp = startEpSpin.value
                                if (libraryDatabase.assignShowAndSeason(paths, title, season, startEp)) {
                                    fixupViewRoot.notifyUser("Assigned " + paths.length + " episode(s) to " + title)
                                    fixupViewRoot.clearSelection()
                                    fixupViewRoot.reloadData()
                                }
                            }
                        }
                    }

                    // Tab 2: Assign Movie
                    ColumnLayout {
                        visible: fixupViewRoot.activeTab === "assign_movie"
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: "Assign as Movie"
                            color: themeManager.textPrimary
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "Movie Title"
                            color: themeManager.textSecondary
                            font.pixelSize: 11
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: themeManager.surfaceCard
                            radius: themeManager.radiusSmall
                            border.color: movieTitleInput.activeFocus ? themeManager.focusRing : themeManager.borderDivider
                            border.width: movieTitleInput.activeFocus ? 2 : themeManager.borderWidth

                            TextInput {
                                id: movieTitleInput
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                verticalAlignment: TextInput.AlignVCenter
                                color: themeManager.textPrimary
                                font.pixelSize: 12
                                text: ""
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text {
                                text: "Year (Optional)"
                                color: themeManager.textSecondary
                                font.pixelSize: 11
                            }
                            SpinBox {
                                id: movieYearSpin
                                from: 1900
                                to: 2099
                                value: 2024
                                Layout.fillWidth: true
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            enabled: fixupViewRoot.selectedCount > 0 && movieTitleInput.text.trim().length > 0
                            opacity: enabled ? 1.0 : 0.5
                            background: Rectangle {
                                color: parent.enabled ? (parent.hovered ? themeManager.accentHover : themeManager.accent) : themeManager.surfaceCard
                                radius: themeManager.radiusSmall
                            }
                            contentItem: Text {
                                text: "Apply Movie Metadata"
                                color: "#ffffff"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                var paths = fixupViewRoot.getSelectedPaths()
                                var title = movieTitleInput.text.trim()
                                var year = movieYearSpin.value
                                for (var i = 0; i < paths.length; ++i) {
                                    libraryDatabase.assignMovie(paths[i], title, year)
                                }
                                fixupViewRoot.notifyUser("Assigned movie: " + title)
                                fixupViewRoot.clearSelection()
                                fixupViewRoot.reloadData()
                            }
                        }
                    }

                    // Tab 3: Merge Shows
                    ColumnLayout {
                        visible: fixupViewRoot.activeTab === "merge_shows"
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: "Merge Two Shows"
                            color: themeManager.textPrimary
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "Source Show Title (to merge from)"
                            color: themeManager.textSecondary
                            font.pixelSize: 11
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: themeManager.surfaceCard
                            radius: themeManager.radiusSmall
                            border.color: sourceShowInput.activeFocus ? themeManager.focusRing : themeManager.borderDivider
                            border.width: sourceShowInput.activeFocus ? 2 : themeManager.borderWidth

                            TextInput {
                                id: sourceShowInput
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                verticalAlignment: TextInput.AlignVCenter
                                color: themeManager.textPrimary
                                font.pixelSize: 12
                            }
                        }

                        Text {
                            text: "Target Show Title (to merge into)"
                            color: themeManager.textSecondary
                            font.pixelSize: 11
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: themeManager.surfaceCard
                            radius: themeManager.radiusSmall
                            border.color: targetShowInput.activeFocus ? themeManager.focusRing : themeManager.borderDivider
                            border.width: targetShowInput.activeFocus ? 2 : themeManager.borderWidth

                            TextInput {
                                id: targetShowInput
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                verticalAlignment: TextInput.AlignVCenter
                                color: themeManager.textPrimary
                                font.pixelSize: 12
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            enabled: sourceShowInput.text.trim().length > 0 && targetShowInput.text.trim().length > 0
                            opacity: enabled ? 1.0 : 0.5
                            background: Rectangle {
                                color: parent.enabled ? (parent.hovered ? themeManager.accentHover : themeManager.accent) : themeManager.surfaceCard
                                radius: themeManager.radiusSmall
                            }
                            contentItem: Text {
                                text: "Merge Shows"
                                color: "#ffffff"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                var src = sourceShowInput.text.trim()
                                var dst = targetShowInput.text.trim()
                                if (libraryDatabase.mergeShows(src, dst)) {
                                    fixupViewRoot.notifyUser("Merged '" + src + "' into '" + dst + "'")
                                    sourceShowInput.text = ""
                                    targetShowInput.text = ""
                                    fixupViewRoot.reloadData()
                                }
                            }
                        }
                    }

                    // Tab 4: Split Show
                    ColumnLayout {
                        visible: fixupViewRoot.activeTab === "split_show"
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: "Split into New Show"
                            color: themeManager.textPrimary
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "New Show Title"
                            color: themeManager.textSecondary
                            font.pixelSize: 11
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: themeManager.surfaceCard
                            radius: themeManager.radiusSmall
                            border.color: splitTitleInput.activeFocus ? themeManager.focusRing : themeManager.borderDivider
                            border.width: splitTitleInput.activeFocus ? 2 : themeManager.borderWidth

                            TextInput {
                                id: splitTitleInput
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                verticalAlignment: TextInput.AlignVCenter
                                color: themeManager.textPrimary
                                font.pixelSize: 12
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text {
                                text: "Season"
                                color: themeManager.textSecondary
                                font.pixelSize: 11
                            }
                            SpinBox {
                                id: splitSeasonSpin
                                from: 1
                                to: 99
                                value: 1
                                Layout.fillWidth: true
                            }
                        }

                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            enabled: fixupViewRoot.selectedCount > 0 && splitTitleInput.text.trim().length > 0
                            opacity: enabled ? 1.0 : 0.5
                            background: Rectangle {
                                color: parent.enabled ? (parent.hovered ? themeManager.accentHover : themeManager.accent) : themeManager.surfaceCard
                                radius: themeManager.radiusSmall
                            }
                            contentItem: Text {
                                text: "Split Selected Episodes"
                                color: "#ffffff"
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                var paths = fixupViewRoot.getSelectedPaths()
                                var newTitle = splitTitleInput.text.trim()
                                var season = splitSeasonSpin.value
                                if (libraryDatabase.splitShow(paths, newTitle, season)) {
                                    fixupViewRoot.notifyUser("Split " + paths.length + " episode(s) into '" + newTitle + "'")
                                    fixupViewRoot.clearSelection()
                                    fixupViewRoot.reloadData()
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
