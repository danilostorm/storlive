#pragma once

#include <QObject>
#include <QTimer>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "obs/ObsEngine.h"
#include "obs/SceneManager.h"
#include "streaming/MultiOutputManager.h"
#include "streaming/StreamDestination.h"

class AppController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList destinations READ destinations NOTIFY destinationsChanged)
    Q_PROPERTY(QVariantList encodeGroups READ encodeGroups NOTIFY destinationsChanged)
    Q_PROPERTY(QVariantList sourceOptions READ sourceOptions NOTIFY sourcesChanged)
    Q_PROPERTY(QStringList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(QStringList encoderOptions READ encoderOptions CONSTANT)
    Q_PROPERTY(QString encoderMode READ encoderMode WRITE setEncoderMode NOTIFY encoderModeChanged)
    Q_PROPERTY(QVariantMap streamProfile READ streamProfile NOTIFY streamProfileChanged)
    Q_PROPERTY(QString engineStatus READ engineStatus NOTIFY statusChanged)
    Q_PROPERTY(QString activityStatus READ activityStatus NOTIFY statusChanged)
    Q_PROPERTY(QVariantList outputStats READ outputStats NOTIFY statsChanged)
    Q_PROPERTY(bool transmissionReady READ transmissionReady NOTIFY statusChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    QVariantList destinations() const;
    QVariantList encodeGroups() const;
    QVariantList sourceOptions() const { return m_scenes.sourceOptions(); }
    QStringList sources() const { return m_scenes.sourceNames(); }
    QStringList encoderOptions() const;
    QString encoderMode() const { return m_encoderMode; }
    QVariantMap streamProfile() const;
    QString engineStatus() const { return m_obs.status(); }
    QString activityStatus() const { return m_activityStatus; }
    QVariantList outputStats() const { return m_outputStats; }
    bool transmissionReady() const;

    void setEncoderMode(const QString &mode);

    Q_INVOKABLE void setDestinationEnabled(int index, bool enabled);
    Q_INVOKABLE void setDestinationCredentials(int index, const QString &server, const QString &streamKey);
    Q_INVOKABLE void addCustomDestination(const QString &name, const QString &server, const QString &streamKey);
    Q_INVOKABLE QVariantMap setStreamProfile(int width,
                                             int height,
                                             int fps,
                                             int videoBitrateKbps,
                                             int audioBitrateKbps);
    Q_INVOKABLE QVariantMap addSource(const QString &kind);
    Q_INVOKABLE QVariantList sourceProperties(const QString &sourceName) const;
    Q_INVOKABLE QVariantMap setSourceProperty(const QString &sourceName,
                                               const QString &propertyName,
                                               const QVariant &value,
                                               const QString &format);
    Q_INVOKABLE void startAll();
    Q_INVOKABLE void stopAll();

signals:
    void destinationsChanged();
    void sourcesChanged();
    void encoderModeChanged();
    void streamProfileChanged();
    void statusChanged();
    void statsChanged();

private slots:
    void refreshStats();

private:
    void addPreset(QString id, QString name, QString server = {});

    ObsEngine m_obs;
    SceneManager m_scenes;
    MultiOutputManager m_multiOutput;
    QVector<StreamDestination> m_destinations;
    QTimer m_statsTimer;
    QVariantList m_outputStats;
    EncodeProfile m_streamProfile;
    QString m_encoderMode {QStringLiteral("Automático")};
    QString m_activityStatus {QStringLiteral("Pronto para configurar")};
};
