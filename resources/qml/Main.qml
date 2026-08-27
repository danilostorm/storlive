import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1360
    height: 820
    minimumWidth: 1050
    minimumHeight: 680
    visible: true
    title: "StorLive 0.1.0-dev"
    color: "#111318"

    property color panel: "#191c23"
    property color panel2: "#20242d"
    property color textPrimary: "#f4f5f7"
    property color textMuted: "#9ea6b4"

    Dialog {
        id: destinationDialog
        modal: true
        anchors.centerIn: parent
        width: 520
        title: "Configurar destino"
        property int destinationIndex: -1
        standardButtons: Dialog.Save | Dialog.Cancel
        onAccepted: controller.setDestinationCredentials(destinationIndex, serverField.text, keyField.text)
        ColumnLayout {
            width: parent.width
            Label { id: destinationLabel; color: root.textPrimary; font.bold: true }
            TextField { id: serverField; Layout.fillWidth: true; placeholderText: "Servidor RTMP / RTMPS" }
            TextField { id: keyField; Layout.fillWidth: true; placeholderText: "Stream key (deixe vazio para manter a atual)"; echoMode: TextInput.Password }
            Label { text: "A chave não é exibida de volta pela interface."; color: root.textMuted; font.pixelSize: 11 }
        }
    }

    Dialog {
        id: sourceConfigDialog
        modal: true
        anchors.centerIn: parent
        width: Math.min(680, root.width - 80)
        height: Math.min(700, root.height - 80)
        title: sourceName.length > 0 ? "Configurar " + sourceName : "Configurar fonte"
        standardButtons: Dialog.Close
        property string sourceName: ""
        property var propertyModel: []

        function refresh() {
            propertyModel = controller.sourceProperties(sourceName)
        }

        function openFor(name) {
            sourceName = name
            refresh()
            open()
        }

        function applyProperty(name, format, value) {
            controller.setSourceProperty(sourceName, name, value, format)
            refresh()
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label {
                text: "Opções fornecidas pelo plugin OBS desta fonte"
                color: root.textMuted
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            Label {
                visible: sourceConfigDialog.propertyModel.length === 0
                text: "Esta fonte não expôs propriedades editáveis suportadas nesta versão."
                color: root.textMuted
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 6
                model: sourceConfigDialog.propertyModel
                delegate: Rectangle {
                    width: ListView.view.width
                    height: propertyColumn.implicitHeight + 16
                    radius: 7
                    color: modelData.type === "section" ? "#161920" : root.panel2
                    opacity: modelData.enabled === false ? 0.55 : 1.0

                    ColumnLayout {
                        id: propertyColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 8
                        spacing: 5

                        Label {
                            text: modelData.label || modelData.name || "Opção"
                            color: root.textPrimary
                            font.bold: modelData.type === "section"
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }

                        Loader {
                            id: propertyLoader
                            Layout.fillWidth: true
                            active: modelData.type !== "section"
                            sourceComponent: {
                                if (modelData.type === "list") return listEditor
                                if (modelData.type === "bool") return boolEditor
                                if (modelData.type === "int") return intEditor
                                if (modelData.type === "float") return floatEditor
                                if (modelData.type === "text") return textEditor
                                return infoEditor
                            }
                        }
                    }

                    Component {
                        id: listEditor
                        ComboBox {
                            enabled: modelData.enabled !== false
                            model: modelData.options || []
                            textRole: "label"
                            Layout.fillWidth: true

                            function syncIndex() {
                                var wanted = String(modelData.value)
                                for (var i = 0; i < model.length; ++i) {
                                    if (String(model[i].value) === wanted) {
                                        currentIndex = i
                                        return
                                    }
                                }
                                currentIndex = -1
                            }

                            Component.onCompleted: syncIndex()
                            onActivated: {
                                if (currentIndex >= 0)
                                    sourceConfigDialog.applyProperty(modelData.name, modelData.format, model[currentIndex].value)
                            }
                        }
                    }

                    Component {
                        id: boolEditor
                        Switch {
                            enabled: modelData.enabled !== false
                            checked: Boolean(modelData.value)
                            text: checked ? "Ativado" : "Desativado"
                            onToggled: sourceConfigDialog.applyProperty(modelData.name, "bool", checked)
                        }
                    }

                    Component {
                        id: intEditor
                        SpinBox {
                            enabled: modelData.enabled !== false
                            from: modelData.min !== undefined ? modelData.min : -2147483647
                            to: modelData.max !== undefined ? modelData.max : 2147483647
                            stepSize: modelData.step !== undefined && modelData.step > 0 ? modelData.step : 1
                            value: Number(modelData.value)
                            editable: true
                            onValueModified: sourceConfigDialog.applyProperty(modelData.name, "int", value)
                        }
                    }

                    Component {
                        id: floatEditor
                        TextField {
                            enabled: modelData.enabled !== false
                            text: Number(modelData.value).toString()
                            validator: DoubleValidator {
                                bottom: modelData.min !== undefined ? modelData.min : -1000000000
                                top: modelData.max !== undefined ? modelData.max : 1000000000
                            }
                            onEditingFinished: sourceConfigDialog.applyProperty(modelData.name, "float", Number(text))
                        }
                    }

                    Component {
                        id: textEditor
                        TextField {
                            enabled: modelData.enabled !== false
                            text: modelData.value || ""
                            echoMode: modelData.password ? TextInput.Password : TextInput.Normal
                            onEditingFinished: sourceConfigDialog.applyProperty(modelData.name, "string", text)
                        }
                    }

                    Component {
                        id: infoEditor
                        Label {
                            text: modelData.value || ""
                            color: root.textMuted
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: sourceDialog
        modal: true
        anchors.centerIn: parent
        width: 500
        height: 470
        title: "Adicionar fonte"
        standardButtons: Dialog.Close
        ColumnLayout {
            anchors.fill: parent
            Label { text: "Fontes detectadas nos plugins do libobs"; color: root.textMuted }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: controller.sourceOptions
                delegate: ItemDelegate {
                    width: ListView.view.width
                    enabled: modelData.available
                    onClicked: {
                        var result = controller.addSource(modelData.kind)
                        sourceDialog.close()
                        if (result.ok)
                            sourceConfigDialog.openFor(result.name)
                    }
                    contentItem: RowLayout {
                        Label { text: modelData.label; color: parent.parent.enabled ? root.textPrimary : "#676d79"; Layout.fillWidth: true }
                        Label { text: modelData.backend; color: root.textMuted; font.pixelSize: 11 }
                    }
                }
            }
        }
    }

    header: ToolBar {
        background: Rectangle { color: "#151820" }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            Label { text: "StorLive"; color: root.textPrimary; font.pixelSize: 22; font.bold: true }
            Label { text: "MULTI-LIVE"; color: "#8a93a5"; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            Label { text: controller.engineStatus; color: controller.transmissionReady ? "#70d6a8" : "#e9b872"; font.pixelSize: 12 }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 245
                Layout.fillHeight: true
                radius: 10
                color: root.panel
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10
                    Label { text: "CENAS"; color: root.textMuted; font.bold: true }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        radius: 6
                        color: root.panel2
                        Label { anchors.centerIn: parent; text: "Gameplay"; color: root.textPrimary; font.bold: true }
                    }
                    Label { text: "FONTES"; color: root.textMuted; font.bold: true }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: controller.sources
                        clip: true
                        spacing: 4
                        delegate: ItemDelegate {
                            width: ListView.view.width
                            height: 40
                            text: modelData
                            onClicked: sourceConfigDialog.openFor(modelData)
                        }
                    }
                    Label { visible: controller.sources.length === 0; text: "Nenhuma fonte adicionada"; color: root.textMuted; font.pixelSize: 11 }
                    Button { Layout.fillWidth: true; text: "+ Adicionar fonte"; onClicked: sourceDialog.open() }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#0b0d11"
                border.color: "#2b303b"
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12
                    Label { text: "PREVIEW"; color: "#596172"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                    Label { text: "Cena Gameplay está conectada à saída do libobs"; color: root.textMuted; Layout.alignment: Qt.AlignHCenter }
                    Label { text: "Renderização do preview na UI entra na próxima camada gráfica"; color: "#6e7686"; Layout.alignment: Qt.AlignHCenter }
                }
            }

            Rectangle {
                Layout.preferredWidth: 350
                Layout.fillHeight: true
                radius: 10
                color: root.panel
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10
                    Label { text: "DESTINOS"; color: root.textMuted; font.bold: true }

                    Repeater {
                        model: controller.destinations
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 72
                            radius: 8
                            color: root.panel2
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 9
                                Switch {
                                    checked: modelData.enabled
                                    onToggled: controller.setDestinationEnabled(index, checked)
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Label { text: modelData.name; color: root.textPrimary; font.bold: true }
                                    Label { text: modelData.hasKey ? "Chave configurada" : "Chave não configurada"; color: modelData.hasKey ? "#70d6a8" : root.textMuted; font.pixelSize: 11 }
                                }
                                ColumnLayout {
                                    Label { text: modelData.state; color: modelData.state === "Ao vivo" ? "#70d6a8" : root.textMuted; font.pixelSize: 11 }
                                    Button {
                                        text: "Configurar"
                                        onClicked: {
                                            destinationDialog.destinationIndex = index
                                            destinationLabel.text = modelData.name
                                            serverField.text = modelData.server
                                            keyField.text = ""
                                            destinationDialog.open()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    GroupBox {
                        title: "Encoder"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            ComboBox {
                                Layout.fillWidth: true
                                model: controller.encoderOptions
                                currentIndex: controller.encoderOptions.indexOf(controller.encoderMode)
                                onActivated: controller.encoderMode = currentText
                            }
                            Label { text: "Perfis iguais reutilizam o mesmo encode H.264/AAC."; color: root.textMuted; wrapMode: Text.WordWrap; Layout.fillWidth: true; font.pixelSize: 11 }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            radius: 10
            color: root.panel
            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 18
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: "ÁUDIO / SAÍDA"; color: root.textMuted; font.bold: true }
                    Label { text: controller.outputStats.length > 0 ? (controller.outputStats.length + " saída(s) monitorada(s)") : "Nenhuma transmissão ativa"; color: root.textPrimary }
                    Label { text: "1080p60 • 48 kHz • reconexão independente"; color: root.textMuted; font.pixelSize: 11 }
                }
                ColumnLayout {
                    Layout.preferredWidth: 470
                    Label { text: controller.activityStatus; color: root.textMuted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        Button { text: "Parar"; onClicked: controller.stopAll() }
                        Button {
                            text: "● INICIAR MULTI-LIVE"
                            highlighted: true
                            enabled: controller.transmissionReady
                            onClicked: controller.startAll()
                            ToolTip.visible: hovered && !controller.transmissionReady
                            ToolTip.text: "O libobs precisa estar ativo com plugin RTMP e encoder H.264/AAC."
                        }
                    }
                }
            }
        }
    }
}
