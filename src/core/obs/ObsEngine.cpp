#include "ObsEngine.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QStandardPaths>
#include <QStringList>

#ifdef STORLIVE_HAS_LIBOBS
extern "C" {
#include <obs.h>
}
#endif

namespace {
#ifdef STORLIVE_HAS_LIBOBS
struct GraphicsModuleChoice {
    QByteArray module;
    QString diagnostic;
};

GraphicsModuleChoice resolveGraphicsModule()
{
    const QByteArray overrideModule = qgetenv("STORLIVE_OBS_GRAPHICS_MODULE");
    if (!overrideModule.isEmpty())
        return {overrideModule, QStringLiteral("override STORLIVE_OBS_GRAPHICS_MODULE")};

    QStringList candidates;
#ifdef Q_OS_WIN
    const QDir appDir(QCoreApplication::applicationDirPath());
    candidates << appDir.filePath(QStringLiteral("libobs-d3d11.dll"));
    candidates << QStringLiteral("libobs-d3d11.dll");
    candidates << QStringLiteral("libobs-d3d11");
#else
    // Runtime packages normally expose the ABI-versioned SONAME only.
    // libobs-dev additionally installs the unversioned .so symlink, so using
    // just "libobs-opengl" makes a development machine work while a normal
    // end-user installation fails with OBS_VIDEO_MODULE_NOT_FOUND (-5).
    candidates << QStringLiteral("libobs-opengl.so.30");
    candidates << QStringLiteral("libobs-opengl.so.0");
    candidates << QStringLiteral("libobs-opengl.so");
    candidates << QStringLiteral("libobs-opengl");
#endif

    QStringList failures;
    for (const QString &candidate : candidates) {
#ifdef Q_OS_WIN
        if (QFileInfo(candidate).isAbsolute() && !QFileInfo::exists(candidate)) {
            failures << QStringLiteral("%1: arquivo ausente").arg(candidate);
            continue;
        }
#endif
        QLibrary library(candidate);
        library.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        if (library.load()) {
            library.unload();
            return {candidate.toUtf8(), QStringLiteral("módulo gráfico resolvido: %1").arg(candidate)};
        }
        failures << QStringLiteral("%1: %2").arg(candidate, library.errorString());
    }

    // Keep a deterministic value so obs_reset_video returns its canonical
    // error code while the status message carries the loader diagnostics.
    return {candidates.constFirst().toUtf8(), failures.join(QStringLiteral(" | "))};
}

QString videoErrorText(int error)
{
    switch (error) {
    case OBS_VIDEO_SUCCESS:
        return QStringLiteral("sucesso");
    case OBS_VIDEO_FAIL:
        return QStringLiteral("falha genérica");
    case OBS_VIDEO_NOT_SUPPORTED:
        return QStringLiteral("GPU/adapter não suportado");
    case OBS_VIDEO_INVALID_PARAM:
        return QStringLiteral("parâmetro de vídeo inválido");
    case OBS_VIDEO_CURRENTLY_ACTIVE:
        return QStringLiteral("vídeo já está ativo");
    case OBS_VIDEO_MODULE_NOT_FOUND:
        return QStringLiteral("módulo gráfico não encontrado ou dependência ausente");
    default:
        return QStringLiteral("erro desconhecido");
    }
}
#endif
}

ObsEngine::~ObsEngine()
{
    shutdown();
}

bool ObsEngine::resetAudioVideo()
{
#ifdef STORLIVE_HAS_LIBOBS
    const GraphicsModuleChoice graphics = resolveGraphicsModule();

    obs_video_info videoInfo {};
    videoInfo.graphics_module = graphics.module.constData();
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

    // Match libobs' documented frontend initialization order: video first,
    // then audio, before loading source/output modules.
    const int videoError = obs_reset_video(&videoInfo);
    if (videoError != OBS_VIDEO_SUCCESS) {
        m_status = QStringLiteral("Falha ao inicializar vídeo do libobs: %1 (código %2) • %3")
                       .arg(videoErrorText(videoError))
                       .arg(videoError)
                       .arg(graphics.diagnostic);
        return false;
    }

    obs_audio_info audioInfo {};
    audioInfo.samples_per_sec = 48000;
    audioInfo.speakers = SPEAKERS_STEREO;
    if (!obs_reset_audio(&audioInfo)) {
        m_status = QStringLiteral("Falha ao inicializar áudio do libobs");
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

#ifdef Q_OS_WIN
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QByteArray libobsData = appDir.filePath(QStringLiteral("data/libobs")).toUtf8();
    if (QFileInfo::exists(QString::fromUtf8(libobsData)))
        obs_add_data_path(libobsData.constData());
#endif

    if (!resetAudioVideo()) {
        obs_shutdown();
        return false;
    }

    const QByteArray customBin = qgetenv("STORLIVE_OBS_PLUGIN_PATH");
    const QByteArray customData = qgetenv("STORLIVE_OBS_PLUGIN_DATA_PATH");
    if (!customBin.isEmpty() && !customData.isEmpty())
        obs_add_module_path(customBin.constData(), customData.constData());

#ifdef Q_OS_WIN
    const QByteArray portableBin = appDir.filePath(QStringLiteral("obs-plugins/64bit")).toUtf8();
    const QByteArray portableData = appDir.filePath(QStringLiteral("data/obs-plugins/%module%")).toUtf8();
    obs_add_module_path(portableBin.constData(), portableData.constData());
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
    m_status = QStringLiteral("Build sem libobs: captura e RTMP indisponíveis");
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
