#ifndef TRANSFORMORGB_H
#define TRANSFORMORGB_H

#include <QObject>
#include <QMatrix4x4>

class TransformORGB : public QObject
{
    Q_OBJECT
public:
    explicit TransformORGB(QObject *parent = nullptr);
    Q_INVOKABLE void transform(QString filePath);

signals:
    void fileReady(QString filePath);

private:
    const QMatrix4x4 toLCC = {0.299,  0.587, 0.114, 0,
                                0.5,    0.5,  -1.0, 0,
                              0.866, -0.866,   0.0, 0,
                                  0,      0,     0, 0};
    QMatrix4x4 fromLCC;

};

#endif // TRANSFORMORGB_H
