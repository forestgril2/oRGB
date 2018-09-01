#ifndef TRANSFORMORGB_H
#define TRANSFORMORGB_H

#include <QObject>
#include <QMatrix3x3>

class TransformORGB : public QObject
{
    Q_OBJECT
public:
    explicit TransformORGB(QObject *parent = nullptr);
    Q_INVOKABLE void transform(QString filePath);

signals:
    void fileReady(QString filePath);

private:
    QMatrix3x3 toORGB;
    QMatrix3x3 fromORGB;

};

#endif // TRANSFORMORGB_H
