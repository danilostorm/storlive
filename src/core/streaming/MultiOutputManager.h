#pragma once

#include <QObject>
#include <QVector>
#include <QVariantList>

#include "StreamDestination.h"

class MultiOutputManager final : public QObject
{
    Q_OBJECT
public:
    explicit MultiOutputManager(QObject *parent = nullptr);

    QVariantList describeGroups(const QVector<StreamDestination> &destinations) const;
    QString validate(const QVector<StreamDestination> &destinations) const;

    // A conexão dos grupos aos obs_output_t será implementada no backend libobs.
    // O agrupamento já existe para que destinos com o mesmo perfil reutilizem o mesmo encode.
    bool outputBackendReady() const noexcept { return false; }
};
