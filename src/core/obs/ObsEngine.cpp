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

QStringList graphicsModuleCandidates()
{
    QStringList candidates;
#ifdef Q_OS_WIN
    const QDir appDir(QCoreApplication::applicationDirPath());
    candidates << appDir.filePath(QStringLiteral("libobs-d3d11.dll"));
    candidates << QStringLiteral("libobs-d3d11");
    candidates << QStringLiteral("libobs-d3d11.dll");
#else
    const QStringList absoluteCandidates {
        QStringLiteral("/usr/lib/x86_64-linux-gnu/libobs-opengl.so.30"),
        QStringLiteral("/usr/lib/x86_64-linux-gnu/libobs-opengl.so.0"),
        QStringLiteral("/usr/lib/x86_64-linux-gnu/libobs-opengl.so"),
        QStringLiteral("/usr/lib/libobs-opengl.so.30"),
        QStringLiteral("/usr/lib/libobs-opengl.so.0"),
        QStringLiteral("/usr/lib/libobs-opengl.so")
    };
    for (const QString &candidate : absoluteCandidates) {
        if (QFileInfo::exists(candidate))
            candidates << candidate;
    }
    candidates << QStringLiteral("libobs-opengl.so.30");
    candidates << QStringLiteral("libobs-opengl.so.0");
    candidates << QStringLiteral("libobs-opengl.so");
    candidates << QStringLiteral("libobs-opengl");
#endif
    candidates.removeDuplicates();
    return candidates;
}

#ifdef Q_OS_WIN
QStringList loadPortableModules(const QDir &appDir)
{
    struct ModuleSpec {
        const char *name;
        bool required;
    };

    const ModuleSpec modules[] = {
        {"win-capture", true},
        {"win-dshow", true},
        {"win-wasapi", true},
        {"obs-ffmpeg", true},
        {"obs-outputs", true},
        {"rtmp-services", true},
        {"obs-x264", true},
        {"obs-nvenc", false},
        {"obs-qsv11", false},
    };

    QStringList failures;
    for (const ModuleSpec &spec : modules) {
        const QString moduleName = QString::fromLatin1(spec.name);
        const QString binaryPath = appDir.filePath(
            QStringLiteral("obs-plugins/64bit/%1.dll").arg(moduleName));
        const QString dataPath = appDir.filePath(
            QStringLiteral("data/obs-plugins/%1").arg(moduleName));

        if (!QFileInfo::exists(binaryPath)) {
            if (spec.required)
                failures << QStringLiteral("%1: DLL ausente").arg(moduleName);
            continue;
        }

        const QByteArray binaryUtf8 = QFileInfo(binaryPath).absoluteFilePath().toUtf8();
        const QByteArray dataUtf8 = QFileInfo::exists(dataPath)
            ? QFileInfo(dataPath).absoluteFilePath().toUtf8()
            : QByteArray();

        obs_module_t *module = nullptr;
        const int openResult = obs_open_module(
            &module,
            binaryUtf8.constData(),
            dataUtf8.isEmpty() ? nullptr : dataUtf8.constData());

        if (openResult != MODULE_SUCCESS || !module) {
            if (spec.required)
                failures << QStringLiteral("%1: obs_open_module=%2")
                                .arg(moduleName)
                                .arg(openResult);
            continue;
        }

        if (!obs_init_module(module) && spec.required)
            failures << QStringLiteral("%1: obs_init_module falhou").arg(moduleName);
    }

    return failures;
}
#endif

int inputTypeCount()
{
    int count = 0;
    const char *inputId = nullptr;
    for (size_t i = 0; obs_enum_input_types(i, &inputId); ++i)
        ++count;
    return count;
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
    obs_video_info videoInfo {};
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

    int lastError = OBS_VIDEO_MODULE_NOT_FOUND;
    QStringList attempts;
    const QStringList candidates = graphicsModuleCandidates();

    for (const QString &candidate : candidates) {
        const QByteArray moduleUtf8 = candidate.toUtf8();
        videoInfo.graphics_module = moduleUtf8.constData();

        QLibrary probe(candidate);
        probe.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        const bool probeLoaded = probe.load();
        const QString probeText = probeLoaded
            ? QStringLiteral("DLL/SO carregável")
            : QStringLiteral("loader: %1").arg(probe.errorString());
        if (probeLoaded)
            probe.unload();

        const int error = obs_reset_video(&videoInfo);
        attempts << QStringLiteral("%1 => %2 (%3), %4")
                        .arg(candidate)
                        .arg(videoErrorText(error))
                        .arg(error)
                        .arg(probeText);

        lastError = error;
        if (error == OBS_VIDEO_SUCCESS)
            break;
    }

    if (lastError != OBS_VIDEO_SUCCESS) {
        m_status = QStringLiteral("Falha ao inicializar vídeo do libobs: %1 (código %2) • tentativas: %3")
                       .arg(videoErrorText(lastError))
                       .arg(lastError)
                       .arg(attempts.join(QStringLiteral(" | ")));
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

    QStringList moduleFailures;
#ifdef Q_OS_WIN
    const QByteArray portableBin = appDir.filePath(QStringLiteral("obs-plugins/64bit")).toUtf8();
    const QByteArray portableData = appDir.filePath(QStringLiteral("data/obs-plugins/%module%")).toUtf8();
    obs_add_module_path(portableBin.constData(), portableData.constData());

    // The portable package contains a controlled set of first-party OBS modules.
    // Open them explicitly so source availability does not depend on discovery
    // heuristics or the process working directory.
    moduleFailures = loadPortableModules(appDir);
#else
    obs_add_module_path("/usr/lib/x86_64-linux-gnu/obs-plugins", "/usr/share/obs/obs-plugins/%module%");
    obs_add_module_path("/usr/lib/obs-plugins", "/usr/share/obs/obs-plugins/%module%");

    obs_module_failure_info failures {};
    obs_load_all_modules2(&failures);
    for (size_t i = 0; i < failures.count; ++i) {
        if (failures.failed_modules && failures.failed_modules[i])
            moduleFailures << QString::fromUtf8(failures.failed_modules[i]);
    }
    obs_module_failure_info_free(&failures);
#endif

    obs_post_load_modules();
    obs_log_loaded_modules();

    m_initialized = true;
    const int inputs = inputTypeCount();
    m_status = QStringLiteral("libobs %1 inicializado • 1080p60 • 48 kHz • %2 tipos de fonte")
                   .arg(QString::fromUtf8(obs_get_version_string()))
                   .arg(inputs);
    if (!moduleFailures.isEmpty())
        m_status += QStringLiteral(" • módulos com falha: %1").arg(moduleFailures.join(QStringLiteral(", ")));
    if (inputs == 0)
        m_status += QStringLiteral(" • nenhum plugin de fonte foi registrado");
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
