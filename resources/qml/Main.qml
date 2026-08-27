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
                    onClicked: { controller.addSource(modelData.kind); sourceDialog.close() }
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
                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 40
                            radius: 6
                            color: root.panel2
                            Label { anchors.centerIn: parent; text: modelData; color: root.textPrimary }
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
