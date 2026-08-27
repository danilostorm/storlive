#include "MultiOutputManager.h"

#include <QHash>
#include <QVariantMap>

MultiOutputManager::MultiOutputManager(QObject *parent)
    : QObject(parent)
{
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
