#include "TransformORGB.h"
#include <QDebug>

TransformORGB::TransformORGB(QObject *parent) : QObject(parent)
{

}

void TransformORGB::transform(QString filePath)
{
    qDebug() << " ### file: " << filePath << " is being transformed";

    emit fileReady(filePath);
}
