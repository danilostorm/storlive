#include "ObsEngine.h"

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#ifdef STORLIVE_HAS_LIBOBS
extern "C" {
#include <obs.h>
}
#endif

ObsEngine::~ObsEngine()
{
    shutdown();
}

bool ObsEngine::resetAudioVideo()
{
#ifdef STORLIVE_HAS_LIBOBS
    obs_audio_info audioInfo {};
    audioInfo.samples_per_sec = 48000;
    audioInfo.speakers = SPEAKERS_STEREO;
    if (!obs_reset_audio(&audioInfo)) {
        m_status = QStringLiteral("Falha ao inicializar áudio do libobs");
        return false;
    }

    obs_video_info videoInfo {};
#ifdef Q_OS_WIN
    videoInfo.graphics_module = "libobs-d3d11";
#else
    videoInfo.graphics_module = "libobs-opengl";
#endif
    videoInfo.fps_num = 60;
    videoInfo.fps_den = 1;
    videoInfo.base_width = 1920;
    videoInfo.base_height = 1080;
    videoInfo.output_width = 1920;
    videoInfo.output_height = 1080;
    videoInfo.output_format = VIDEO_FORMAT_NV12;
    videoInfo.adapter = 0;
    videoInfo.gpu_conversion = true;
    videoInfo.colorspace = VIDEO_CS_709;
    videoInfo.range = VIDEO_RANGE_PARTIAL;
    videoInfo.scale_type = OBS_SCALE_BICUBIC;

    const int videoError = obs_reset_video(&videoInfo);
    if (videoError != OBS_VIDEO_SUCCESS) {
        m_status = QStringLiteral("Falha ao inicializar vídeo do libobs (código %1)").arg(videoError);
        return false;
    }

    return true;
#else
    return false;
#endif
}

bool ObsEngine::initialize()
{
#ifdef STORLIVE_HAS_LIBOBS
    if (m_initialized)
        return true;

    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    const QByteArray config = QFileInfo(configDir).absoluteFilePath().toUtf8();

    if (!obs_startup("pt-BR", config.constData(), nullptr)) {
        m_status = QStringLiteral("Falha ao iniciar libobs");
        return false;
    }

    if (!resetAudioVideo()) {
        obs_shutdown();
        return false;
    }

    const QByteArray customBin = qgetenv("STORLIVE_OBS_PLUGIN_PATH");
    const QByteArray customData = qgetenv("STORLIVE_OBS_PLUGIN_DATA_PATH");
    if (!customBin.isEmpty() && !customData.isEmpty())
        obs_add_module_path(customBin.constData(), customData.constData());

#ifdef Q_OS_WIN
    obs_add_module_path("obs-plugins/64bit", "data/obs-plugins/%module%");
#else
    obs_add_module_path("/usr/lib/x86_64-linux-gnu/obs-plugins", "/usr/share/obs/obs-plugins/%module%");
    obs_add_module_path("/usr/lib/obs-plugins", "/usr/share/obs/obs-plugins/%module%");
#endif

    obs_load_all_modules();
    obs_post_load_modules();

    m_initialized = true;
    m_status = QStringLiteral("libobs %1 inicializado • 1080p60 • 48 kHz")
                   .arg(QString::fromUtf8(obs_get_version_string()));
    return true;
#else
    m_status = QStringLiteral("Build sem libobs: interface/core disponíveis; captura e RTMP desativados");
    return false;
#endif
}

void ObsEngine::shutdown()
{
#ifdef STORLIVE_HAS_LIBOBS
    if (m_initialized && obs_initialized())
        obs_shutdown();
#endif
    m_initialized = false;
}
