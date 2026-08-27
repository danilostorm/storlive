#include "SceneManager.h"

#include <QByteArray>
#include <QVariantMap>

SceneManager::~SceneManager()
{
    shutdown();
}

bool SceneManager::initialize(QString *error)
{
#ifdef STORLIVE_HAS_LIBOBS
    if (m_scene)
        return true;

    if (!obs_initialized()) {
        if (error)
            *error = QStringLiteral("libobs não está inicializado");
        return false;
    }

    m_scene = obs_scene_create("Gameplay");
    if (!m_scene) {
        if (error)
            *error = QStringLiteral("Não foi possível criar a cena Gameplay");
        return false;
    }

    obs_set_output_source(0, obs_scene_get_source(m_scene));
    return true;
#else
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return false;
#endif
}

void SceneManager::shutdown()
{
#ifdef STORLIVE_HAS_LIBOBS
    if (m_scene) {
        if (obs_initialized())
            obs_set_output_source(0, nullptr);
        obs_scene_release(m_scene);
        m_scene = nullptr;
    }
#endif
    m_sourceNames.clear();
}

bool SceneManager::inputRegistered(const QString &id) const
{
#ifdef STORLIVE_HAS_LIBOBS
    const QByteArray wanted = id.toUtf8();
    const char *inputId = nullptr;
    for (size_t i = 0; obs_enum_input_types(i, &inputId); ++i) {
        if (inputId && wanted == inputId)
            return true;
    }
#else
    Q_UNUSED(id)
#endif
    return false;
}

QString SceneManager::findAvailableInput(const QStringList &candidates) const
{
    for (const QString &candidate : candidates) {
        if (inputRegistered(candidate))
            return candidate;
    }
    return {};
}

QStringList SceneManager::candidatesForKind(const QString &kind) const
{
#ifdef Q_OS_WIN
    if (kind == QStringLiteral("game"))
        return {QStringLiteral("game_capture")};
    if (kind == QStringLiteral("window"))
        return {QStringLiteral("window_capture")};
    if (kind == QStringLiteral("display"))
        return {QStringLiteral("monitor_capture")};
    if (kind == QStringLiteral("webcam"))
        return {QStringLiteral("dshow_input")};
    if (kind == QStringLiteral("mic"))
        return {QStringLiteral("wasapi_input_capture")};
    if (kind == QStringLiteral("desktop_audio"))
        return {QStringLiteral("wasapi_output_capture")};
#else
    if (kind == QStringLiteral("game"))
        return {QStringLiteral("game_capture")};
    if (kind == QStringLiteral("window"))
        return {QStringLiteral("pipewire-window-capture-source"), QStringLiteral("xcomposite_input")};
    if (kind == QStringLiteral("display"))
        return {QStringLiteral("pipewire-screen-capture-source"), QStringLiteral("pipewire-desktop-capture-source"), QStringLiteral("xshm_input_v2"), QStringLiteral("xshm_input")};
    if (kind == QStringLiteral("webcam"))
        return {QStringLiteral("v4l2_input")};
    if (kind == QStringLiteral("mic"))
        return {QStringLiteral("pulse_input_capture"), QStringLiteral("pipewire-audio-capture-source")};
    if (kind == QStringLiteral("desktop_audio"))
        return {QStringLiteral("pulse_output_capture"), QStringLiteral("pipewire-audio-capture-source")};
#endif
    return {};
}

QString SceneManager::labelForKind(const QString &kind) const
{
    if (kind == QStringLiteral("game")) return QStringLiteral("Captura de jogo");
    if (kind == QStringLiteral("window")) return QStringLiteral("Captura de janela");
    if (kind == QStringLiteral("display")) return QStringLiteral("Captura de monitor/tela");
    if (kind == QStringLiteral("webcam")) return QStringLiteral("Webcam");
    if (kind == QStringLiteral("mic")) return QStringLiteral("Microfone");
    if (kind == QStringLiteral("desktop_audio")) return QStringLiteral("Áudio do computador");
    return kind;
}

QVariantList SceneManager::sourceOptions() const
{
    QVariantList result;
    const QStringList kinds {
        QStringLiteral("game"),
        QStringLiteral("window"),
        QStringLiteral("display"),
        QStringLiteral("webcam"),
        QStringLiteral("mic"),
        QStringLiteral("desktop_audio")
    };

    for (const QString &kind : kinds) {
        const QString id = findAvailableInput(candidatesForKind(kind));
        result.append(QVariantMap {
            {QStringLiteral("kind"), kind},
            {QStringLiteral("label"), labelForKind(kind)},
            {QStringLiteral("available"), !id.isEmpty()},
            {QStringLiteral("backend"), id.isEmpty() ? QStringLiteral("plugin não encontrado") : id}
        });
    }
    return result;
}

bool SceneManager::addSource(const QString &kind, QString *error)
{
#ifdef STORLIVE_HAS_LIBOBS
    if (!m_scene && !initialize(error))
        return false;

    const QString inputId = findAvailableInput(candidatesForKind(kind));
    if (inputId.isEmpty()) {
        if (error)
            *error = QStringLiteral("Nenhum plugin de captura disponível para %1").arg(labelForKind(kind));
        return false;
    }

    QString name = labelForKind(kind);
    int suffix = 2;
    while (m_sourceNames.contains(name))
        name = QStringLiteral("%1 %2").arg(labelForKind(kind)).arg(suffix++);

    const QByteArray idUtf8 = inputId.toUtf8();
    const QByteArray nameUtf8 = name.toUtf8();
    obs_source_t *source = obs_source_create(idUtf8.constData(), nameUtf8.constData(), nullptr, nullptr);
    if (!source) {
        if (error)
            *error = QStringLiteral("Falha ao criar %1 usando %2").arg(name, inputId);
        return false;
    }

    obs_sceneitem_t *item = obs_scene_add(m_scene, source);
    obs_source_release(source);
    if (!item) {
        if (error)
            *error = QStringLiteral("Falha ao adicionar %1 à cena").arg(name);
        return false;
    }

    m_sourceNames.append(name);
    return true;
#else
    Q_UNUSED(kind)
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return false;
#endif
}
