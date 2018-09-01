#ifndef TRANSFORMORGB_H
#define TRANSFORMORGB_H

#include <QObject>

class TransformORGB : public QObject
{
    Q_OBJECT
public:
    explicit TransformORGB(QObject *parent = nullptr);
    Q_INVOKABLE void transform(QString filePath);

signals:
    void fileReady(QString filePath);

public slots:
};

#endif // TRANSFORMORGB_H
