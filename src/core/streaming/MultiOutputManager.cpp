#include "MultiOutputManager.h"

#include <QByteArray>
#include <QHash>
#include <QVariantMap>

MultiOutputManager::MultiOutputManager(QObject *parent)
    : QObject(parent)
{
}

MultiOutputManager::~MultiOutputManager()
{
    stop();
}

QVariantList MultiOutputManager::describeGroups(const QVector<StreamDestination> &destinations) const
{
    QHash<QString, QStringList> groups;

    for (const auto &destination : destinations) {
        if (!destination.enabled)
            continue;
        groups[destination.profile.signature()].append(destination.name);
    }

    QVariantList result;
    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        result.append(QVariantMap {
            {QStringLiteral("profile"), it.key()},
            {QStringLiteral("destinations"), it.value()},
            {QStringLiteral("encoderCount"), 1}
        });
    }
    return result;
}

QString MultiOutputManager::validate(const QVector<StreamDestination> &destinations) const
{
    int enabled = 0;
    QStringList invalid;

    for (const auto &destination : destinations) {
        if (!destination.enabled)
            continue;
        ++enabled;
        if (destination.server.trimmed().isEmpty() || destination.streamKey.trimmed().isEmpty())
            invalid.append(destination.name);
    }

    if (enabled == 0)
        return QStringLiteral("Ative pelo menos um destino.");

    if (!invalid.isEmpty())
        return QStringLiteral("Servidor/chave ausente em: %1").arg(invalid.join(QStringLiteral(", ")));

    return {};
}

#ifdef STORLIVE_HAS_LIBOBS
static bool containsHardwareHint(const QString &value)
{
    const QString lower = value.toLower();
    return lower.contains(QStringLiteral("nvenc")) ||
           lower.contains(QStringLiteral("amf")) ||
           lower.contains(QStringLiteral("qsv")) ||
           lower.contains(QStringLiteral("quick sync")) ||
           lower.contains(QStringLiteral("vaapi"));
}

QString MultiOutputManager::selectVideoEncoder(const QString &mode) const
{
    QString software;
    QString hardware;
    QString fallback;
    const char *id = nullptr;

    for (size_t i = 0; obs_enum_encoder_types(i, &id); ++i) {
        if (!id || obs_get_encoder_type(id) != OBS_ENCODER_VIDEO)
            continue;

        const char *codec = obs_get_encoder_codec(id);
        if (!codec || QString::fromUtf8(codec).compare(QStringLiteral("h264"), Qt::CaseInsensitive) != 0)
            continue;

        const QString qid = QString::fromUtf8(id);
        const QString display = QString::fromUtf8(obs_encoder_get_display_name(id));
        const QString combined = qid + QLatin1Char(' ') + display;
        if (fallback.isEmpty())
            fallback = qid;
        if (qid == QStringLiteral("obs_x264") || combined.contains(QStringLiteral("x264"), Qt::CaseInsensitive))
            software = qid;
        if (containsHardwareHint(combined) && hardware.isEmpty())
            hardware = qid;
    }

    if (mode.startsWith(QStringLiteral("Software")))
        return !software.isEmpty() ? software : fallback;
    if (mode == QStringLiteral("Hardware"))
        return hardware;
    return !hardware.isEmpty() ? hardware : (!software.isEmpty() ? software : fallback);
}

QString MultiOutputManager::selectAudioEncoder() const
{
    QString fallback;
    const char *id = nullptr;
    for (size_t i = 0; obs_enum_encoder_types(i, &id); ++i) {
        if (!id || obs_get_encoder_type(id) != OBS_ENCODER_AUDIO)
            continue;
        const char *codec = obs_get_encoder_codec(id);
        if (!codec || QString::fromUtf8(codec).compare(QStringLiteral("aac"), Qt::CaseInsensitive) != 0)
            continue;
        const QString qid = QString::fromUtf8(id);
        if (qid == QStringLiteral("ffmpeg_aac"))
            return qid;
        if (fallback.isEmpty())
            fallback = qid;
    }
    return fallback;
}

bool MultiOutputManager::typeRegistered(bool outputType, const QString &id) const
{
    const QByteArray wanted = id.toUtf8();
    const char *typeId = nullptr;
    for (size_t i = 0; outputType ? obs_enum_output_types(i, &typeId) : obs_enum_service_types(i, &typeId); ++i) {
        if (typeId && wanted == typeId)
            return true;
    }
    return false;
}

int MultiOutputManager::createEncoderGroup(const EncodeProfile &profile, const QString &mode, QString *error)
{
    const QString videoId = selectVideoEncoder(mode);
    const QString audioId = selectAudioEncoder();
    if (videoId.isEmpty() || audioId.isEmpty()) {
        if (error)
            *error = QStringLiteral("Encoder H.264/AAC compatível não foi encontrado nos plugins do libobs");
        return -1;
    }

    obs_data_t *videoSettings = obs_encoder_defaults(videoId.toUtf8().constData());
    if (!videoSettings)
        videoSettings = obs_data_create();
    obs_data_set_int(videoSettings, "bitrate", profile.videoBitrateKbps);
    obs_data_set_string(videoSettings, "rate_control", "CBR");
    obs_data_set_int(videoSettings, "keyint_sec", 2);

    const QByteArray signature = profile.signature().toUtf8();
    const QByteArray videoIdUtf8 = videoId.toUtf8();
    const QByteArray videoName = QByteArray("StorLive Video ") + signature;
    obs_encoder_t *video = obs_video_encoder_create(videoIdUtf8.constData(), videoName.constData(), videoSettings, nullptr);
    obs_data_release(videoSettings);
    if (!video) {
        if (error)
            *error = QStringLiteral("Falha ao criar encoder de vídeo %1").arg(videoId);
        return -1;
    }

    obs_encoder_set_video(video, obs_get_video());
    obs_encoder_set_scaled_size(video, profile.width, profile.height);
    if (profile.fps == 30)
        obs_encoder_set_frame_rate_divisor(video, 2);

    obs_data_t *audioSettings = obs_encoder_defaults(audioId.toUtf8().constData());
    if (!audioSettings)
        audioSettings = obs_data_create();
    obs_data_set_int(audioSettings, "bitrate", profile.audioBitrateKbps);

    const QByteArray audioIdUtf8 = audioId.toUtf8();
    const QByteArray audioName = QByteArray("StorLive Audio ") + signature;
    obs_encoder_t *audio = obs_audio_encoder_create(audioIdUtf8.constData(), audioName.constData(), audioSettings, 0, nullptr);
    obs_data_release(audioSettings);
    if (!audio) {
        obs_encoder_release(video);
        if (error)
            *error = QStringLiteral("Falha ao criar encoder de áudio %1").arg(audioId);
        return -1;
    }
    obs_encoder_set_audio(audio, obs_get_audio());

    EncoderGroup group;
    group.signature = profile.signature();
    group.profile = profile;
    group.video = video;
    group.audio = audio;
    m_groups.append(group);
    return m_groups.size() - 1;
}
#endif

bool MultiOutputManager::outputBackendReady() const
{
#ifdef STORLIVE_HAS_LIBOBS
    return obs_initialized() &&
           typeRegistered(true, QStringLiteral("rtmp_output")) &&
           typeRegistered(false, QStringLiteral("rtmp_custom")) &&
           !selectVideoEncoder(QStringLiteral("Automático")).isEmpty() &&
           !selectAudioEncoder().isEmpty();
#else
    return false;
#endif
}

bool MultiOutputManager::start(QVector<StreamDestination> &destinations, const QString &encoderMode, QString *error)
{
    stop();

    const QString validation = validate(destinations);
    if (!validation.isEmpty()) {
        if (error)
            *error = validation;
        return false;
    }

#ifdef STORLIVE_HAS_LIBOBS
    if (!outputBackendReady()) {
        if (error)
            *error = QStringLiteral("Backend RTMP do libobs não está disponível. Verifique obs-plugins e os encoders instalados.");
        return false;
    }

    QHash<QString, int> groupIndexes;
    QStringList failures;
    int started = 0;

    for (auto &destination : destinations) {
        if (!destination.enabled)
            continue;

        const QString signature = destination.profile.signature();
        int groupIndex = groupIndexes.value(signature, -1);
        if (groupIndex < 0) {
            QString groupError;
            groupIndex = createEncoderGroup(destination.profile, encoderMode, &groupError);
            if (groupIndex < 0) {
                destination.state = QStringLiteral("Erro");
                failures.append(QStringLiteral("%1: %2").arg(destination.name, groupError));
                continue;
            }
            groupIndexes.insert(signature, groupIndex);
        }

        EncoderGroup &group = m_groups[groupIndex];
        obs_data_t *serviceSettings = obs_data_create();
        const QByteArray server = destination.server.toUtf8();
        const QByteArray key = destination.streamKey.toUtf8();
        obs_data_set_string(serviceSettings, "server", server.constData());
        obs_data_set_string(serviceSettings, "key", key.constData());

        const QByteArray serviceName = QStringLiteral("StorLive Service %1").arg(destination.id).toUtf8();
        obs_service_t *service = obs_service_create("rtmp_custom", serviceName.constData(), serviceSettings, nullptr);
        obs_data_release(serviceSettings);
        if (!service) {
            destination.state = QStringLiteral("Erro");
            failures.append(QStringLiteral("%1: falha ao criar serviço RTMP").arg(destination.name));
            continue;
        }

        const QByteArray outputName = QStringLiteral("StorLive Output %1").arg(destination.id).toUtf8();
        obs_output_t *output = obs_output_create("rtmp_output", outputName.constData(), nullptr, nullptr);
        if (!output) {
            obs_service_release(service);
            destination.state = QStringLiteral("Erro");
            failures.append(QStringLiteral("%1: falha ao criar output RTMP").arg(destination.name));
            continue;
        }

        obs_output_set_video_encoder(output, group.video);
        obs_output_set_audio_encoder(output, group.audio, 0);
        obs_output_set_service(output, service);
        obs_output_set_reconnect_settings(output, 20, 2);

        destination.state = QStringLiteral("Conectando");
        if (!obs_output_start(output)) {
            const char *lastError = obs_output_get_last_error(output);
            const QString reason = lastError && *lastError ? QString::fromUtf8(lastError) : QStringLiteral("falha desconhecida");
            destination.state = QStringLiteral("Erro");
            failures.append(QStringLiteral("%1: %2").arg(destination.name, reason));
            obs_output_release(output);
            obs_service_release(service);
            continue;
        }

        ActiveOutput active;
        active.destinationId = destination.id;
        active.output = output;
        active.service = service;
        m_outputs.append(active);
        destination.state = QStringLiteral("Ao vivo");
        ++started;
    }

    if (started == 0) {
        stop();
        if (error)
            *error = failures.isEmpty() ? QStringLiteral("Nenhum destino conseguiu iniciar") : failures.join(QStringLiteral(" | "));
        return false;
    }

    if (error && !failures.isEmpty())
        *error = failures.join(QStringLiteral(" | "));
    return true;
#else
    Q_UNUSED(destinations)
    Q_UNUSED(encoderMode)
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return false;
#endif
}

void MultiOutputManager::stop()
{
#ifdef STORLIVE_HAS_LIBOBS
    for (auto &active : m_outputs) {
        if (active.output) {
            if (obs_output_active(active.output))
                obs_output_force_stop(active.output);
            obs_output_release(active.output);
        }
        if (active.service)
            obs_service_release(active.service);
    }
    m_outputs.clear();

    for (auto &group : m_groups) {
        if (group.video)
            obs_encoder_release(group.video);
        if (group.audio)
            obs_encoder_release(group.audio);
    }
    m_groups.clear();
#endif
}

QVariantList MultiOutputManager::stats() const
{
    QVariantList result;
#ifdef STORLIVE_HAS_LIBOBS
    for (const auto &active : m_outputs) {
        if (!active.output)
            continue;
        const char *lastError = obs_output_get_last_error(active.output);
        result.append(QVariantMap {
            {QStringLiteral("id"), active.destinationId},
            {QStringLiteral("active"), obs_output_active(active.output)},
            {QStringLiteral("reconnecting"), obs_output_reconnecting(active.output)},
            {QStringLiteral("bytes"), static_cast<qulonglong>(obs_output_get_total_bytes(active.output))},
            {QStringLiteral("dropped"), obs_output_get_frames_dropped(active.output)},
            {QStringLiteral("frames"), obs_output_get_total_frames(active.output)},
            {QStringLiteral("congestion"), obs_output_get_congestion(active.output)},
            {QStringLiteral("connectMs"), obs_output_get_connect_time_ms(active.output)},
            {QStringLiteral("lastError"), lastError ? QString::fromUtf8(lastError) : QString()}
        });
    }
#endif
    return result;
}
