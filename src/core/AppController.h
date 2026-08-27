#pragma once

#include <QObject>
#include <QVariantList>
#include <QVector>

#include "obs/ObsEngine.h"
#include "streaming/MultiOutputManager.h"
#include "streaming/StreamDestination.h"

class AppController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList destinations READ destinations NOTIFY destinationsChanged)
    Q_PROPERTY(QVariantList encodeGroups READ encodeGroups NOTIFY destinationsChanged)
    Q_PROPERTY(QStringList encoderOptions READ encoderOptions CONSTANT)
    Q_PROPERTY(QString encoderMode READ encoderMode WRITE setEncoderMode NOTIFY encoderModeChanged)
    Q_PROPERTY(QString engineStatus READ engineStatus NOTIFY statusChanged)
    Q_PROPERTY(QString activityStatus READ activityStatus NOTIFY statusChanged)
    Q_PROPERTY(bool transmissionReady READ transmissionReady NOTIFY statusChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    QVariantList destinations() const;
    QVariantList encodeGroups() const;
    QStringList encoderOptions() const;
    QString encoderMode() const { return m_encoderMode; }
    QString engineStatus() const { return m_obs.status(); }
    QString activityStatus() const { return m_activityStatus; }
    bool transmissionReady() const;

    void setEncoderMode(const QString &mode);

    Q_INVOKABLE void setDestinationEnabled(int index, bool enabled);
    Q_INVOKABLE void setDestinationCredentials(int index, const QString &server, const QString &streamKey);
    Q_INVOKABLE void addCustomDestination(const QString &name, const QString &server, const QString &streamKey);
    Q_INVOKABLE void startAll();
    Q_INVOKABLE void stopAll();

signals:
    void destinationsChanged();
    void encoderModeChanged();
    void statusChanged();

private:
    void addPreset(QString id, QString name, QString server = {});

    ObsEngine m_obs;
    MultiOutputManager m_multiOutput;
    QVector<StreamDestination> m_destinations;
    QString m_encoderMode {QStringLiteral("Automático")};
    QString m_activityStatus {QStringLiteral("Pronto para configurar")};
};
