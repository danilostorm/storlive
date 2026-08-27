#pragma once

#include <QString>
#include <QVariantMap>

struct EncodeProfile {
    QString codec {QStringLiteral("h264")};
    int width {1920};
    int height {1080};
    int fps {60};
    int videoBitrateKbps {6000};
    int audioBitrateKbps {160};

    QString signature() const
    {
        return QStringLiteral("%1:%2x%3:%4:%5:%6")
            .arg(codec)
            .arg(width)
            .arg(height)
            .arg(fps)
            .arg(videoBitrateKbps)
            .arg(audioBitrateKbps);
    }
};

struct StreamDestination {
    QString id;
    QString name;
    QString server;
    QString streamKey;
    bool enabled {false};
    QString state {QStringLiteral("Parado")};
    EncodeProfile profile;

    QVariantMap publicMap() const
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("server"), server},
            {QStringLiteral("enabled"), enabled},
            {QStringLiteral("state"), state},
            {QStringLiteral("hasKey"), !streamKey.isEmpty()},
            {QStringLiteral("profile"), profile.signature()}
        };
    }
};
