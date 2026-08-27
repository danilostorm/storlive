#pragma once

#include <QObject>
#include <QVector>
#include <QVariantList>

#include "StreamDestination.h"

#ifdef STORLIVE_HAS_LIBOBS
extern "C" {
#include <obs.h>
}
#endif

class MultiOutputManager final : public QObject
{
    Q_OBJECT
public:
    explicit MultiOutputManager(QObject *parent = nullptr);
    ~MultiOutputManager() override;

    QVariantList describeGroups(const QVector<StreamDestination> &destinations) const;
    QString validate(const QVector<StreamDestination> &destinations) const;

    bool outputBackendReady() const;
    bool start(QVector<StreamDestination> &destinations, const QString &encoderMode, QString *error = nullptr);
    void stop();
    QVariantList stats() const;

private:
#ifdef STORLIVE_HAS_LIBOBS
    struct EncoderGroup {
        QString signature;
        EncodeProfile profile;
        obs_encoder_t *video {nullptr};
        obs_encoder_t *audio {nullptr};
    };

    struct ActiveOutput {
        QString destinationId;
        obs_output_t *output {nullptr};
        obs_service_t *service {nullptr};
    };

    QString selectVideoEncoder(const QString &mode) const;
    QString selectAudioEncoder() const;
    bool typeRegistered(bool outputType, const QString &id) const;
    int createEncoderGroup(const EncodeProfile &profile, const QString &mode, QString *error);

    QVector<EncoderGroup> m_groups;
    QVector<ActiveOutput> m_outputs;
#endif
};
