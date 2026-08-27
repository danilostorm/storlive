#include "AppController.h"

#include <QVariantMap>
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

    if (m_obs.initialize()) {
        QString sceneError;
        if (!m_scenes.initialize(&sceneError))
            m_activityStatus = sceneError;
        else
            m_activityStatus = m_multiOutput.outputBackendReady()
                ? QStringLiteral("Engine pronta • configure fontes e destinos")
                : QStringLiteral("libobs ativo, mas faltam plugins RTMP/encoders");
    }

    m_statsTimer.setInterval(1000);
    connect(&m_statsTimer, &QTimer::timeout, this, &AppController::refreshStats);
    m_statsTimer.start();
    emit statusChanged();
}

AppController::~AppController()
{
    m_statsTimer.stop();
    m_multiOutput.stop();
    m_scenes.shutdown();
}

void AppController::addPreset(QString id, QString name, QString server)
{
    StreamDestination destination;
    destination.id = std::move(id);
    destination.name = std::move(name);
    destination.server = std::move(server);
    destination.profile = m_streamProfile;
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

QVariantMap AppController::streamProfile() const
{
    return {
        {QStringLiteral("width"), m_streamProfile.width},
        {QStringLiteral("height"), m_streamProfile.height},
        {QStringLiteral("fps"), m_streamProfile.fps},
        {QStringLiteral("videoBitrateKbps"), m_streamProfile.videoBitrateKbps},
        {QStringLiteral("audioBitrateKbps"), m_streamProfile.audioBitrateKbps},
        {QStringLiteral("label"), QStringLiteral("%1x%2 @ %3 fps • %4 kbps")
                                      .arg(m_streamProfile.width)
                                      .arg(m_streamProfile.height)
                                      .arg(m_streamProfile.fps)
                                      .arg(m_streamProfile.videoBitrateKbps)}
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
    if (!server.trimmed().isEmpty())
        m_destinations[index].server = server.trimmed();
    if (!streamKey.trimmed().isEmpty())
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
    destination.profile = m_streamProfile;
    m_destinations.append(std::move(destination));
    emit destinationsChanged();
}

QVariantMap AppController::setStreamProfile(int width,
                                             int height,
                                             int fps,
                                             int videoBitrateKbps,
                                             int audioBitrateKbps)
{
    const bool supportedResolution = (width == 1920 && height == 1080)
        || (width == 1280 && height == 720);
    if (!supportedResolution || (fps != 30 && fps != 60)) {
        const QString error = QStringLiteral("Perfil inválido: use 1920x1080 ou 1280x720 em 30/60 fps");
        m_activityStatus = error;
        emit statusChanged();
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
    }

    videoBitrateKbps = qBound(1000, videoBitrateKbps, 20000);
    audioBitrateKbps = qBound(64, audioBitrateKbps, 320);

    m_streamProfile.width = width;
    m_streamProfile.height = height;
    m_streamProfile.fps = fps;
    m_streamProfile.videoBitrateKbps = videoBitrateKbps;
    m_streamProfile.audioBitrateKbps = audioBitrateKbps;

    for (auto &destination : m_destinations)
        destination.profile = m_streamProfile;

    m_activityStatus = m_outputStats.isEmpty()
        ? QStringLiteral("Perfil atualizado: %1").arg(streamProfile().value(QStringLiteral("label")).toString())
        : QStringLiteral("Perfil atualizado • reinicie a transmissão para aplicar aos outputs ativos");

    emit streamProfileChanged();
    emit destinationsChanged();
    emit statusChanged();
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QString()}};
}

QVariantMap AppController::addSource(const QString &kind)
{
    QString error;
    QString createdName;
    const bool ok = m_scenes.addSource(kind, &createdName, &error);
    if (ok) {
        m_activityStatus = QStringLiteral("%1 adicionada • escolha a janela/dispositivo nas propriedades").arg(createdName);
        emit sourcesChanged();
    } else {
        m_activityStatus = error;
    }
    emit statusChanged();

    return {
        {QStringLiteral("ok"), ok},
        {QStringLiteral("name"), createdName},
        {QStringLiteral("error"), error}
    };
}

QVariantList AppController::sourceProperties(const QString &sourceName) const
{
    return m_scenes.sourceProperties(sourceName);
}

QVariantMap AppController::setSourceProperty(const QString &sourceName,
                                              const QString &propertyName,
                                              const QVariant &value,
                                              const QString &format)
{
    QString error;
    const bool ok = m_scenes.setSourceProperty(sourceName, propertyName, value, format, &error);
    if (ok)
        m_activityStatus = QStringLiteral("Configuração de %1 atualizada").arg(sourceName);
    else
        m_activityStatus = error;
    emit statusChanged();

    return {
        {QStringLiteral("ok"), ok},
        {QStringLiteral("error"), error}
    };
}

void AppController::startAll()
{
    QString resultMessage;
    const bool started = m_multiOutput.start(m_destinations, m_encoderMode, &resultMessage);
    if (started) {
        m_activityStatus = resultMessage.isEmpty()
            ? QStringLiteral("Multi-live iniciada")
            : QStringLiteral("Multi-live iniciada com avisos: %1").arg(resultMessage);
    } else {
        m_activityStatus = resultMessage;
    }
    emit destinationsChanged();
    emit statusChanged();
    refreshStats();
}

void AppController::stopAll()
{
    m_multiOutput.stop();
    for (auto &destination : m_destinations) {
        if (destination.enabled)
            destination.state = QStringLiteral("Parado");
    }
    m_outputStats.clear();
    m_activityStatus = QStringLiteral("Transmissões paradas");
    emit destinationsChanged();
    emit statsChanged();
    emit statusChanged();
}

void AppController::refreshStats()
{
    const QVariantList newStats = m_multiOutput.stats();
    if (newStats != m_outputStats) {
        m_outputStats = newStats;
        emit statsChanged();
    }

    bool stateChanged = false;
    for (const QVariant &entry : m_outputStats) {
        const QVariantMap stat = entry.toMap();
        const QString id = stat.value(QStringLiteral("id")).toString();
        for (auto &destination : m_destinations) {
            if (destination.id != id)
                continue;
            QString nextState;
            if (stat.value(QStringLiteral("reconnecting")).toBool())
                nextState = QStringLiteral("Reconectando");
            else if (stat.value(QStringLiteral("active")).toBool())
                nextState = QStringLiteral("Ao vivo");
            else if (!stat.value(QStringLiteral("lastError")).toString().isEmpty())
                nextState = QStringLiteral("Erro");
            else
                nextState = QStringLiteral("Parado");
            if (destination.state != nextState) {
                destination.state = nextState;
                stateChanged = true;
            }
        }
    }
    if (stateChanged)
        emit destinationsChanged();
}
