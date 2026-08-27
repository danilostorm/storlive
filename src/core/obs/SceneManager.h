#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>

#ifdef STORLIVE_HAS_LIBOBS
extern "C" {
#include <obs.h>
#include <obs-properties.h>
}
#endif

class SceneManager final
{
public:
    SceneManager() = default;
    ~SceneManager();

    bool initialize(QString *error = nullptr);
    void shutdown();

    QVariantList sourceOptions() const;
    QStringList sourceNames() const { return m_sourceNames; }
    bool addSource(const QString &kind, QString *createdName = nullptr, QString *error = nullptr);
    QVariantList sourceProperties(const QString &sourceName, QString *error = nullptr) const;
    bool setSourceProperty(const QString &sourceName,
                           const QString &propertyName,
                           const QVariant &value,
                           const QString &format,
                           QString *error = nullptr);

private:
    QString findAvailableInput(const QStringList &candidates) const;
    bool inputRegistered(const QString &id) const;
    QStringList candidatesForKind(const QString &kind) const;
    QString labelForKind(const QString &kind) const;

#ifdef STORLIVE_HAS_LIBOBS
    static void appendProperties(obs_properties_t *properties,
                                 obs_data_t *settings,
                                 QVariantList &result);
#endif

#ifdef STORLIVE_HAS_LIBOBS
    obs_scene_t *m_scene {nullptr};
#endif
    QStringList m_sourceNames;
};
