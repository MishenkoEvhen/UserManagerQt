import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    visible: true
    width: 820; height: 560
    title: "Менеджер пользователей"

    // ---------- Автосохранение при закрытии ----------
    onClosing: userModel.autoSave()

    // ---------- Обработка сообщений из модели ----------
    Connections {
        target: userModel
        function onErrorOccurred(message) {
            msgDialog.title = "Ошибка"
            msgDialog.text  = message
            msgDialog.open()
        }
        function onInfoOccurred(message) {
            msgDialog.title = "Информация"
            msgDialog.text  = message
            msgDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // ---------- Строка ввода ----------
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: nameField
                placeholderText: "Имя"
                Layout.fillWidth: true
                onAccepted: addButton.clicked()
            }
            SpinBox { id: ageSpin; from: 1; to: 150; value: 18 }
            Button {
                id: addButton
                text: "Добавить"
                onClicked: {
                    if (userModel.addUser(nameField.text, ageSpin.value))
                        nameField.clear()
                }
            }
        }

        // ---------- Поиск + сортировка ----------
        RowLayout {
            Layout.fillWidth: true

            Label { text: "Поиск:" }
            TextField {
                id: searchField
                placeholderText: "Введите имя..."
                Layout.preferredWidth: 220
                onTextChanged: filterModel.filterText = text
            }
            Button {
                text: "✕"
                onClicked: { searchField.text = ""; filterModel.filterText = "" }
                ToolTip.visible: hovered
                ToolTip.text: "Очистить"
            }

            Item { Layout.fillWidth: true }

            Label { text: "Сортировка:" }
            Button { text: "Имя ↑";  onClicked: userModel.sortBy(0, true)  }
            Button { text: "Имя ↓";  onClicked: userModel.sortBy(0, false) }
            Button { text: "Возр ↑"; onClicked: userModel.sortBy(1, true)  }
            Button { text: "Возр ↓"; onClicked: userModel.sortBy(1, false) }
            Button { text: "Дата";   onClicked: userModel.sortBy(2, true)  }
        }

        // ---------- Список ----------
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: filterModel
            delegate: Rectangle {
                width: listView.width
                height: 42
                color: index % 2 ? "#f5f5f5" : "white"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    Text {
                        text: name
                        Layout.preferredWidth: 220
                        elide: Text.ElideRight
                        font.bold: true
                    }
                    Text {
                        text: "Возраст: " + age
                        Layout.preferredWidth: 120
                    }
                    Text {
                        text: created
                        Layout.fillWidth: true
                        color: "#666"
                    }

                    Button {
                        text: "✏"
                        ToolTip.visible: hovered
                        ToolTip.text: "Редактировать"
                        onClicked: {
                            editNameField.text = name
                            editAgeSpin.value  = age
                            editDialog.sourceRow = userModel.index(index, 0)
                            editDialog.open()
                        }
                    }
                    Button {
                        text: "✕"
                        ToolTip.visible: hovered
                        ToolTip.text: "Удалить"
                        onClicked: {
                            confirmDialog.pendingName = name
                            confirmDialog.pendingRow  = index
                            confirmDialog.open()
                        }
                    }
                }
            }
        }

        // ---------- Нижние кнопки + счётчик ----------
        RowLayout {
            Layout.fillWidth: true

            Button { text: "Сохранить JSON"; onClicked: saveDialog.open() }
            Button { text: "Загрузить JSON"; onClicked: openDialog.open() }
            Button { text: "Экспорт CSV";    onClicked: csvExportDialog.open() }
            Button { text: "Импорт CSV";     onClicked: csvImportDialog.open() }

            Item { Layout.fillWidth: true }

            Label {
                text: "Показано: " + filterModel.rowCount()
                      + " / Всего: " + userModel.rowCount()
                color: "#666"
            }
        }
    }

    // ---------- Диалог редактирования ----------
    Dialog {
        id: editDialog
        title: "Редактирование пользователя"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel

        // Храним QModelIndex исходной строки
        property var sourceRow

        ColumnLayout {
            spacing: 8
            Label { text: "Имя:" }
            TextField { id: editNameField; Layout.preferredWidth: 260 }
            Label { text: "Возраст:" }
            SpinBox { id: editAgeSpin; from: 1; to: 150 }
        }

        onAccepted: {
            if (sourceRow !== undefined) {
                // Преобразуем прокси-индекс в индекс исходной модели
                const srcIdx = filterModel.mapToSource(sourceRow)
                if (userModel.updateUser(srcIdx.row,
                                         editNameField.text,
                                         editAgeSpin.value)) {
                    console.log("Обновлено")
                }
            }
        }
    }

    // ---------- Диалог подтверждения удаления ----------
    Dialog {
        id: confirmDialog
        title: "Подтверждение"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No

        property string pendingName: ""
        property int    pendingRow:  -1

        Label {
            text: "Удалить пользователя \"" + confirmDialog.pendingName + "\"?"
        }

        onAccepted: {
            if (pendingRow >= 0) {
                const srcIdx = filterModel.mapToSource(filterModel.index(pendingRow, 0))
                userModel.removeUser(srcIdx.row)
            }
        }
    }

    // ---------- Универсальный диалог сообщений ----------
    Dialog {
        id: msgDialog
        property alias text: msgLabel.text
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok
        Label {
            id: msgLabel
            wrapMode: Text.WordWrap
            Layout.maximumWidth: 360
        }
    }

    // ---------- Файловые диалоги ----------
    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["JSON файлы (*.json)", "Все файлы (*)"]
        onAccepted: userModel.saveToFile(urlToPath(selectedFile))
    }
    FileDialog {
        id: openDialog
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON файлы (*.json)", "Все файлы (*)"]
        onAccepted: userModel.loadFromFile(urlToPath(selectedFile))
    }
    FileDialog {
        id: csvExportDialog
        fileMode: FileDialog.SaveFile
        defaultSuffix: "csv"
        nameFilters: ["CSV файлы (*.csv)", "Все файлы (*)"]
        onAccepted: userModel.exportToCsv(urlToPath(selectedFile))
    }
    FileDialog {
        id: csvImportDialog
        fileMode: FileDialog.OpenFile
        nameFilters: ["CSV файлы (*.csv)", "Все файлы (*)"]
        onAccepted: userModel.importFromCsv(urlToPath(selectedFile))
    }

    function urlToPath(url) {
        return url.toString().replace("file:///", "")
    }
}