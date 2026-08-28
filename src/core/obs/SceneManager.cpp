#include "SceneManager.h"

#include <QByteArray>
#include <QVariantMap>

namespace {
#ifdef STORLIVE_HAS_LIBOBS
QString comboFormatName(obs_combo_format format)
{
    switch (format) {
    case OBS_COMBO_FORMAT_INT: return QStringLiteral("int");
    case OBS_COMBO_FORMAT_FLOAT: return QStringLiteral("float");
    case OBS_COMBO_FORMAT_BOOL: return QStringLiteral("bool");
    case OBS_COMBO_FORMAT_STRING: return QStringLiteral("string");
    default: return QStringLiteral("string");
    }
}

QVariant valueForFormat(obs_data_t *settings, const char *name, const QString &format)
{
    if (format == QStringLiteral("int"))
        return QVariant::fromValue<qlonglong>(obs_data_get_int(settings, name));
    if (format == QStringLiteral("float"))
        return obs_data_get_double(settings, name);
    if (format == QStringLiteral("bool"))
        return obs_data_get_bool(settings, name);
    return QString::fromUtf8(obs_data_get_string(settings, name));
}

void applyDefaultLayout(obs_sceneitem_t *item, const QString &kind)
{
    if (!item)
        return;

    if (kind == QStringLiteral("process") ||
        kind == QStringLiteral("game") ||
        kind == QStringLiteral("window") ||
        kind == QStringLiteral("display") ||
        kind == QStringLiteral("media")) {
        vec2 pos {0.0f, 0.0f};
        vec2 bounds {1920.0f, 1080.0f};
        obs_sceneitem_set_pos(item, &pos);
        obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
        obs_sceneitem_set_bounds(item, &bounds);
        return;
    }

    if (kind == QStringLiteral("webcam")) {
        vec2 pos {1420.0f, 790.0f};
        vec2 bounds {480.0f, 270.0f};
        obs_sceneitem_set_pos(item, &pos);
        obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
        obs_sceneitem_set_bounds(item, &bounds);
    }
}
#endif
}

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
    if (kind == QStringLiteral("process"))
        return {QStringLiteral("game_capture")};
    if (kind == QStringLiteral("game"))
        return {QStringLiteral("game_capture")};
    if (kind == QStringLiteral("window"))
        return {QStringLiteral("window_capture")};
    if (kind == QStringLiteral("display"))
        return {QStringLiteral("monitor_capture")};
    if (kind == QStringLiteral("media"))
        return {QStringLiteral("ffmpeg_source")};
    if (kind == QStringLiteral("webcam"))
        return {QStringLiteral("dshow_input")};
    if (kind == QStringLiteral("mic"))
        return {QStringLiteral("wasapi_input_capture")};
    if (kind == QStringLiteral("desktop_audio"))
        return {QStringLiteral("wasapi_output_capture")};
#else
    if (kind == QStringLiteral("process"))
        return {QStringLiteral("game_capture")};
    if (kind == QStringLiteral("game"))
        return {QStringLiteral("game_capture")};
    if (kind == QStringLiteral("window"))
        return {QStringLiteral("pipewire-window-capture-source"), QStringLiteral("xcomposite_input")};
    if (kind == QStringLiteral("display"))
        return {QStringLiteral("pipewire-screen-capture-source"), QStringLiteral("pipewire-desktop-capture-source"), QStringLiteral("xshm_input_v2"), QStringLiteral("xshm_input")};
    if (kind == QStringLiteral("media"))
        return {QStringLiteral("ffmpeg_source")};
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
    if (kind == QStringLiteral("process")) return QStringLiteral("Processo / jogo em execução");
    if (kind == QStringLiteral("game")) return QStringLiteral("Captura de jogo (automática)");
    if (kind == QStringLiteral("window")) return QStringLiteral("Captura de janela");
    if (kind == QStringLiteral("display")) return QStringLiteral("Captura de monitor/tela");
    if (kind == QStringLiteral("media")) return QStringLiteral("Mídia / vídeo / URL de live");
    if (kind == QStringLiteral("webcam")) return QStringLiteral("Webcam / dispositivo de vídeo");
    if (kind == QStringLiteral("mic")) return QStringLiteral("Microfone");
    if (kind == QStringLiteral("desktop_audio")) return QStringLiteral("Áudio do computador");
    return kind;
}

QVariantList SceneManager::sourceOptions() const
{
    QVariantList result;
    const QStringList kinds {
        QStringLiteral("process"),
        QStringLiteral("game"),
        QStringLiteral("window"),
        QStringLiteral("display"),
        QStringLiteral("media"),
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

QVariantList SceneManager::sourceItems() const
{
    QVariantList result;
    result.reserve(m_sourceNames.size());

    for (const QString &name : m_sourceNames) {
        bool visible = true;
        bool muted = false;
        bool hasAudio = false;
#ifdef STORLIVE_HAS_LIBOBS
        if (m_scene) {
            const QByteArray nameUtf8 = name.toUtf8();
            if (obs_sceneitem_t *item = obs_scene_find_source(m_scene, nameUtf8.constData()))
                visible = obs_sceneitem_visible(item);

            if (obs_source_t *source = obs_get_source_by_name(nameUtf8.constData())) {
                const uint32_t flags = obs_source_get_output_flags(source);
                hasAudio = (flags & OBS_SOURCE_AUDIO) != 0;
                muted = hasAudio && obs_source_muted(source);
                obs_source_release(source);
            }
        }
#endif
        result.append(QVariantMap {
            {QStringLiteral("name"), name},
            {QStringLiteral("visible"), visible},
            {QStringLiteral("muted"), muted},
            {QStringLiteral("hasAudio"), hasAudio}
        });
    }
    return result;
}

bool SceneManager::addSource(const QString &kind, QString *createdName, QString *error)
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

    obs_data_t *initialSettings = nullptr;
    if (kind == QStringLiteral("process")) {
        initialSettings = obs_data_create();
        // game_capture normally starts in "any fullscreen" mode.  For the
        // explicit process workflow we enter the same specific-window mode
        // used by OBS so its Window property immediately exposes running
        // games/applications.  EXE priority keeps matching stable when the
        // game changes its window title.
        obs_data_set_string(initialSettings, "capture_mode", "window");
        obs_data_set_int(initialSettings, "priority", 2);
    } else if (kind == QStringLiteral("media")) {
        initialSettings = obs_data_create();
        obs_data_set_bool(initialSettings, "is_local_file", true);
    }

    const QByteArray idUtf8 = inputId.toUtf8();
    const QByteArray nameUtf8 = name.toUtf8();
    obs_source_t *source = obs_source_create(idUtf8.constData(), nameUtf8.constData(), initialSettings, nullptr);
    if (initialSettings)
        obs_data_release(initialSettings);

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

    applyDefaultLayout(item, kind);
    m_sourceNames.append(name);
    if (createdName)
        *createdName = name;
    return true;
#else
    Q_UNUSED(kind)
    Q_UNUSED(createdName)
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return false;
#endif
}

bool SceneManager::setSourceVisible(const QString &sourceName, bool visible, QString *error)
{
#ifdef STORLIVE_HAS_LIBOBS
    if (!m_scene) {
        if (error)
            *error = QStringLiteral("Cena Gameplay não está inicializada");
        return false;
    }

    const QByteArray nameUtf8 = sourceName.toUtf8();
    obs_sceneitem_t *item = obs_scene_find_source(m_scene, nameUtf8.constData());
    if (!item) {
        if (error)
            *error = QStringLiteral("Fonte não encontrada na cena: %1").arg(sourceName);
        return false;
    }

    obs_sceneitem_set_visible(item, visible);
    return true;
#else
    Q_UNUSED(sourceName)
    Q_UNUSED(visible)
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return false;
#endif
}

bool SceneManager::setSourceMuted(const QString &sourceName, bool muted, QString *error)
{
#ifdef STORLIVE_HAS_LIBOBS
    const QByteArray nameUtf8 = sourceName.toUtf8();
    obs_source_t *source = obs_get_source_by_name(nameUtf8.constData());
    if (!source) {
        if (error)
            *error = QStringLiteral("Fonte não encontrada: %1").arg(sourceName);
        return false;
    }

    const bool hasAudio = (obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO) != 0;
    if (!hasAudio) {
        obs_source_release(source);
        if (error)
            *error = QStringLiteral("Esta fonte não possui áudio");
        return false;
    }

    obs_source_set_muted(source, muted);
    obs_source_release(source);
    return true;
#else
    Q_UNUSED(sourceName)
    Q_UNUSED(muted)
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return false;
#endif
}

bool SceneManager::removeSource(const QString &sourceName, QString *error)
{
#ifdef STORLIVE_HAS_LIBOBS
    if (!m_scene) {
        if (error)
            *error = QStringLiteral("Cena Gameplay não está inicializada");
        return false;
    }

    const QByteArray nameUtf8 = sourceName.toUtf8();
    obs_sceneitem_t *item = obs_scene_find_source(m_scene, nameUtf8.constData());
    if (!item) {
        if (error)
            *error = QStringLiteral("Fonte não encontrada na cena: %1").arg(sourceName);
        return false;
    }

    obs_sceneitem_remove(item);
    m_sourceNames.removeAll(sourceName);
    return true;
#else
    Q_UNUSED(sourceName)
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return false;
#endif
}

#ifdef STORLIVE_HAS_LIBOBS
void SceneManager::appendProperties(obs_properties_t *properties,
                                    obs_data_t *settings,
                                    QVariantList &result)
{
    obs_property_t *property = obs_properties_first(properties);
    while (property) {
        if (obs_property_visible(property)) {
            const char *rawName = obs_property_name(property);
            const char *rawDescription = obs_property_description(property);
            const QString name = rawName ? QString::fromUtf8(rawName) : QString();
            const QString label = rawDescription && *rawDescription
                ? QString::fromUtf8(rawDescription)
                : name;
            const auto type = obs_property_get_type(property);

            if (type == OBS_PROPERTY_GROUP) {
                result.append(QVariantMap {
                    {QStringLiteral("type"), QStringLiteral("section")},
                    {QStringLiteral("label"), label},
                    {QStringLiteral("enabled"), obs_property_enabled(property)}
                });
                obs_properties_t *group = obs_property_group_content(property);
                if (group)
                    appendProperties(group, settings, result);
            } else if (!name.isEmpty()) {
                QVariantMap item {
                    {QStringLiteral("name"), name},
                    {QStringLiteral("label"), label},
                    {QStringLiteral("enabled"), obs_property_enabled(property)}
                };

                switch (type) {
                case OBS_PROPERTY_LIST: {
                    const QString format = comboFormatName(obs_property_list_format(property));
                    QVariantList options;
                    const size_t count = obs_property_list_item_count(property);
                    for (size_t i = 0; i < count; ++i) {
                        QVariant optionValue;
                        switch (obs_property_list_format(property)) {
                        case OBS_COMBO_FORMAT_INT:
                            optionValue = QVariant::fromValue<qlonglong>(obs_property_list_item_int(property, i));
                            break;
                        case OBS_COMBO_FORMAT_FLOAT:
                            optionValue = obs_property_list_item_float(property, i);
                            break;
                        case OBS_COMBO_FORMAT_BOOL:
                            optionValue = obs_property_list_item_bool(property, i);
                            break;
                        case OBS_COMBO_FORMAT_STRING:
                        default:
                            optionValue = QString::fromUtf8(obs_property_list_item_string(property, i));
                            break;
                        }
                        const char *optionName = obs_property_list_item_name(property, i);
                        options.append(QVariantMap {
                            {QStringLiteral("label"), optionName ? QString::fromUtf8(optionName) : QString()},
                            {QStringLiteral("value"), optionValue},
                            {QStringLiteral("disabled"), obs_property_list_item_disabled(property, i)}
                        });
                    }
                    item.insert(QStringLiteral("type"), QStringLiteral("list"));
                    item.insert(QStringLiteral("format"), format);
                    item.insert(QStringLiteral("value"), valueForFormat(settings, rawName, format));
                    item.insert(QStringLiteral("options"), options);
                    break;
                }
                case OBS_PROPERTY_BOOL:
                    item.insert(QStringLiteral("type"), QStringLiteral("bool"));
                    item.insert(QStringLiteral("format"), QStringLiteral("bool"));
                    item.insert(QStringLiteral("value"), obs_data_get_bool(settings, rawName));
                    break;
                case OBS_PROPERTY_INT:
                    item.insert(QStringLiteral("type"), QStringLiteral("int"));
                    item.insert(QStringLiteral("format"), QStringLiteral("int"));
                    item.insert(QStringLiteral("value"), QVariant::fromValue<qlonglong>(obs_data_get_int(settings, rawName)));
                    item.insert(QStringLiteral("min"), obs_property_int_min(property));
                    item.insert(QStringLiteral("max"), obs_property_int_max(property));
                    item.insert(QStringLiteral("step"), obs_property_int_step(property));
                    break;
                case OBS_PROPERTY_FLOAT:
                    item.insert(QStringLiteral("type"), QStringLiteral("float"));
                    item.insert(QStringLiteral("format"), QStringLiteral("float"));
                    item.insert(QStringLiteral("value"), obs_data_get_double(settings, rawName));
                    item.insert(QStringLiteral("min"), obs_property_float_min(property));
                    item.insert(QStringLiteral("max"), obs_property_float_max(property));
                    item.insert(QStringLiteral("step"), obs_property_float_step(property));
                    break;
                case OBS_PROPERTY_TEXT:
                    if (obs_property_text_type(property) == OBS_TEXT_INFO) {
                        item.insert(QStringLiteral("type"), QStringLiteral("info"));
                        item.insert(QStringLiteral("value"), QString::fromUtf8(obs_data_get_string(settings, rawName)));
                    } else {
                        item.insert(QStringLiteral("type"), QStringLiteral("text"));
                        item.insert(QStringLiteral("format"), QStringLiteral("string"));
                        item.insert(QStringLiteral("value"), QString::fromUtf8(obs_data_get_string(settings, rawName)));
                        item.insert(QStringLiteral("password"), obs_property_text_type(property) == OBS_TEXT_PASSWORD);
                    }
                    break;
                case OBS_PROPERTY_PATH:
                    item.insert(QStringLiteral("type"), QStringLiteral("text"));
                    item.insert(QStringLiteral("format"), QStringLiteral("string"));
                    item.insert(QStringLiteral("value"), QString::fromUtf8(obs_data_get_string(settings, rawName)));
                    break;
                default:
                    item.clear();
                    break;
                }

                if (!item.isEmpty())
                    result.append(item);
            }
        }
        obs_property_next(&property);
    }
}
#endif

QVariantList SceneManager::sourceProperties(const QString &sourceName, QString *error) const
{
#ifdef STORLIVE_HAS_LIBOBS
    const QByteArray sourceNameUtf8 = sourceName.toUtf8();
    obs_source_t *source = obs_get_source_by_name(sourceNameUtf8.constData());
    if (!source) {
        if (error)
            *error = QStringLiteral("Fonte não encontrada: %1").arg(sourceName);
        return {};
    }

    obs_data_t *settings = obs_source_get_settings(source);
    obs_properties_t *properties = obs_source_properties(source);
    QVariantList result;

    if (settings && properties) {
        obs_properties_apply_settings(properties, settings);
        appendProperties(properties, settings, result);
    }

    if (properties)
        obs_properties_destroy(properties);
    if (settings)
        obs_data_release(settings);
    obs_source_release(source);

    return result;
#else
    Q_UNUSED(sourceName)
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return {};
#endif
}

bool SceneManager::setSourceProperty(const QString &sourceName,
                                     const QString &propertyName,
                                     const QVariant &value,
                                     const QString &format,
                                     QString *error)
{
#ifdef STORLIVE_HAS_LIBOBS
    const QByteArray sourceNameUtf8 = sourceName.toUtf8();
    obs_source_t *source = obs_get_source_by_name(sourceNameUtf8.constData());
    if (!source) {
        if (error)
            *error = QStringLiteral("Fonte não encontrada: %1").arg(sourceName);
        return false;
    }

    obs_data_t *settings = obs_source_get_settings(source);
    if (!settings) {
        obs_source_release(source);
        if (error)
            *error = QStringLiteral("Não foi possível ler as configurações da fonte");
        return false;
    }

    const QByteArray propertyUtf8 = propertyName.toUtf8();
    if (format == QStringLiteral("bool"))
        obs_data_set_bool(settings, propertyUtf8.constData(), value.toBool());
    else if (format == QStringLiteral("int"))
        obs_data_set_int(settings, propertyUtf8.constData(), value.toLongLong());
    else if (format == QStringLiteral("float"))
        obs_data_set_double(settings, propertyUtf8.constData(), value.toDouble());
    else {
        const QByteArray valueUtf8 = value.toString().toUtf8();
        obs_data_set_string(settings, propertyUtf8.constData(), valueUtf8.constData());
    }

    obs_properties_t *properties = obs_source_properties(source);
    if (properties) {
        obs_properties_apply_settings(properties, settings);
        obs_properties_destroy(properties);
    }

    obs_source_update(source, settings);
    obs_data_release(settings);
    obs_source_release(source);
    return true;
#else
    Q_UNUSED(sourceName)
    Q_UNUSED(propertyName)
    Q_UNUSED(value)
    Q_UNUSED(format)
    if (error)
        *error = QStringLiteral("Build sem libobs");
    return false;
#endif
}
