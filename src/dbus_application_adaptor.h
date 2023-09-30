/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp ../dbus/org.freedesktop.Application.xml -a dbus_application_adaptor
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef DBUS_APPLICATION_ADAPTOR_H
#define DBUS_APPLICATION_ADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

/*
 * Adaptor class for interface org.freedesktop.Application
 */
class ApplicationAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Application")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Application\">\n"
"    <method name=\"Activate\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.In0\"/>\n"
"      <arg direction=\"in\" type=\"a{sv}\" name=\"platform_data\"/>\n"
"    </method>\n"
"    <method name=\"Open\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.In1\"/>\n"
"      <arg direction=\"in\" type=\"as\" name=\"uris\"/>\n"
"      <arg direction=\"in\" type=\"a{sv}\" name=\"platform_data\"/>\n"
"    </method>\n"
"    <method name=\"ActivateAction\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.In2\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"action_name\"/>\n"
"      <arg direction=\"in\" type=\"av\" name=\"parameter\"/>\n"
"      <arg direction=\"in\" type=\"a{sv}\" name=\"platform_data\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    ApplicationAdaptor(QObject *parent);
    virtual ~ApplicationAdaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    void Activate(const QVariantMap &platform_data);
    void ActivateAction(const QString &action_name, const QVariantList &parameter, const QVariantMap &platform_data);
    void Open(const QStringList &uris, const QVariantMap &platform_data);
Q_SIGNALS: // SIGNALS
};

#endif