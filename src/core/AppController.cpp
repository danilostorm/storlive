#include "AppController.h"

#include <QUuid>
#include <utility>

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_multiOutput(this)
{
    addPreset(QStringLiteral("youtube"), QStringLiteral("YouTube"), QStringLiteral("rtmp://a.rtmp.youtube.com/live2"));
    addPreset(QStringLiteral("twitch"), QStringLiteral("Twitch"));
    addPreset(QStringLiteral("kick"), QStringLiteral("Kick"));
    addPreset(QStringLiteral("facebook"), QStringLiteral("Facebook"));
    addPreset(QStringLiteral("custom"), QStringLiteral("RTMP personalizado"));

    m_obs.initialize();
    emit statusChanged();
}

void AppController::addPreset(QString id, QString name, QString server)
{
    StreamDestination destination;
    destination.id = std::move(id);
    destination.name = std::move(name);
    destination.server = std::move(server);
    m_destinations.append(std::move(destination));
}

QVariantList AppController::destinations() const
{
    QVariantList result;
    result.reserve(m_destinations.size());
    for (const auto &destination : m_destinations)
        result.append(destination.publicMap());
    return result;
}

QVariantList AppController::encodeGroups() const
{
    return m_multiOutput.describeGroups(m_destinations);
}

QStringList AppController::encoderOptions() const
{
    return {
        QStringLiteral("Automático"),
        QStringLiteral("Hardware"),
        QStringLiteral("Software (x264)")
    };
}

bool AppController::transmissionReady() const
{
    return m_obs.isInitialized() && m_multiOutput.outputBackendReady();
}

void AppController::setEncoderMode(const QString &mode)
{
    if (mode == m_encoderMode)
        return;
    m_encoderMode = mode;
    emit encoderModeChanged();
}

void AppController::setDestinationEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_destinations.size())
        return;
    m_destinations[index].enabled = enabled;
    emit destinationsChanged();
}

void AppController::setDestinationCredentials(int index, const QString &server, const QString &streamKey)
{
    if (index < 0 || index >= m_destinations.size())
        return;
    m_destinations[index].server = server.trimmed();
    m_destinations[index].streamKey = streamKey.trimmed();
    emit destinationsChanged();
}

void AppController::addCustomDestination(const QString &name, const QString &server, const QString &streamKey)
{
    StreamDestination destination;
    destination.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    destination.name = name.trimmed().isEmpty() ? QStringLiteral("RTMP personalizado") : name.trimmed();
    destination.server = server.trimmed();
    destination.streamKey = streamKey.trimmed();
    destination.enabled = true;
    m_destinations.append(std::move(destination));
    emit destinationsChanged();
}

void AppController::startAll()
{
    const QString validation = m_multiOutput.validate(m_destinations);
    if (!validation.isEmpty()) {
        m_activityStatus = validation;
        emit statusChanged();
        return;
    }

    if (!transmissionReady()) {
        m_activityStatus = QStringLiteral("Destinos e perfis estão válidos. Falta conectar o MultiOutputManager aos obs_output_t antes de liberar a transmissão.");
        emit statusChanged();
        return;
    }
}

void AppController::stopAll()
{
    m_activityStatus = QStringLiteral("Transmissões paradas");
    emit statusChanged();
}
