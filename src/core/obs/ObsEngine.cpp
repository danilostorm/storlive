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
    m_status = QStringLiteral("libobs %1 inicializado").arg(QString::fromUtf8(obs_get_version_string()));
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
