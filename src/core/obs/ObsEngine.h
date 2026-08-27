#pragma once

#include <QString>

class ObsEngine final
{
public:
    ObsEngine() = default;
    ~ObsEngine();

    bool initialize();
    void shutdown();

    bool isInitialized() const noexcept { return m_initialized; }
    QString status() const { return m_status; }

private:
    bool resetAudioVideo();

    bool m_initialized {false};
    QString m_status {QStringLiteral("libobs ainda não inicializado")};
};
