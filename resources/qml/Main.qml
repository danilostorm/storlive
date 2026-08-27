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
                Layout.preferredWidth: 235
                Layout.fillHeight: true
                radius: 10
                color: root.panel
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10
                    Label { text: "CENAS"; color: root.textMuted; font.bold: true }
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 125
                        model: ["Gameplay", "Câmera", "Intervalo"]
                        delegate: ItemDelegate {
                            width: ListView.view.width
                            text: modelData
                            highlighted: index === 0
                        }
                    }
                    Label { text: "FONTES"; color: root.textMuted; font.bold: true }
                    Repeater {
                        model: ["Jogo / janela", "Monitor", "Webcam", "Microfone", "Áudio desktop"]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            radius: 6
                            color: root.panel2
                            Label { anchors.centerIn: parent; text: modelData; color: root.textPrimary }
                        }
                    }
                    Item { Layout.fillHeight: true }
                    Button { Layout.fillWidth: true; text: "+ Adicionar fonte"; enabled: false; ToolTip.text: "Será conectado aos source plugins do libobs" }
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
                    Label { text: "A saída gráfica do libobs será renderizada aqui"; color: root.textMuted; Layout.alignment: Qt.AlignHCenter }
                    Label { text: "Captura: jogo • janela • monitor • webcam"; color: "#6e7686"; Layout.alignment: Qt.AlignHCenter }
                }
            }

            Rectangle {
                Layout.preferredWidth: 330
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
                            Layout.preferredHeight: 68
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
                                Label { text: modelData.state; color: root.textMuted; font.pixelSize: 11 }
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
                            Label { text: "Perfis iguais serão agrupados para reutilizar o mesmo encode."; color: root.textMuted; wrapMode: Text.WordWrap; Layout.fillWidth: true; font.pixelSize: 11 }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 86
            radius: 10
            color: root.panel
            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 18
                ColumnLayout {
                    Layout.fillWidth: true
                    Label { text: "ÁUDIO"; color: root.textMuted; font.bold: true }
                    RowLayout {
                        Label { text: "Mic"; color: root.textPrimary }
                        ProgressBar { value: 0; Layout.preferredWidth: 160 }
                        Label { text: "Desktop"; color: root.textPrimary }
                        ProgressBar { value: 0; Layout.preferredWidth: 160 }
                    }
                }
                ColumnLayout {
                    Layout.preferredWidth: 420
                    Label { text: controller.activityStatus; color: root.textMuted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    RowLayout {
                        Button { text: "Parar"; onClicked: controller.stopAll() }
                        Button {
                            text: "● INICIAR MULTI-LIVE"
                            highlighted: true
                            onClicked: controller.startAll()
                            ToolTip.visible: hovered && !controller.transmissionReady
                            ToolTip.text: "A UI/core já está pronta; o output RTMP libobs será ligado na próxima etapa."
                        }
                    }
                }
            }
        }
    }
}
