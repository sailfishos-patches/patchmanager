#ifndef PATCHOBJECT_HPP
#define PATCHOBJECT_HPP

#include <QJSValue>
#include <QObject>
#include <QQmlPropertyMap>

class PatchObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlPropertyMap* details READ details CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit PatchObject(const QVariantMap &data, QObject *parent = nullptr);
    QQmlPropertyMap *details();
    bool busy() const;

signals:
    void busyChanged(bool busy);

public slots:
    void apply();
    void unapply();

private:
    void setBusy(bool busy);

    QQmlPropertyMap *m_details;
    bool m_busy = false;

};

#endif // PATCHOBJECT_HPP
