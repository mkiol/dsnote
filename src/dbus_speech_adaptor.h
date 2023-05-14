/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp ../dbus/org.mkiol.Speech.xml -a dbus_speech_adaptor
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef DBUS_SPEECH_ADAPTOR_H
#define DBUS_SPEECH_ADAPTOR_H

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
 * Adaptor class for interface org.mkiol.Speech
 */
class SpeechAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mkiol.Speech")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.mkiol.Speech\">\n"
"    <property access=\"read\" type=\"i\" name=\"State\"/>\n"
"    <signal name=\"StatePropertyChanged\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"state\"/>\n"
"    </signal>\n"
"    <property access=\"read\" type=\"i\" name=\"Speech\"/>\n"
"    <signal name=\"SpeechPropertyChanged\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"speech\"/>\n"
"    </signal>\n"
"    <property access=\"readwrite\" type=\"s\" name=\"DefaultSttLang\"/>\n"
"    <signal name=\"DefaultSttLangPropertyChanged\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"lang\"/>\n"
"    </signal>\n"
"    <property access=\"readwrite\" type=\"s\" name=\"DefaultTtsLang\"/>\n"
"    <signal name=\"DefaultTtsLangPropertyChanged\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"lang\"/>\n"
"    </signal>\n"
"    <property access=\"readwrite\" type=\"s\" name=\"DefaultSttModel\"/>\n"
"    <signal name=\"DefaultSttModelPropertyChanged\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"model\"/>\n"
"    </signal>\n"
"    <property access=\"readwrite\" type=\"s\" name=\"DefaultTtsModel\"/>\n"
"    <signal name=\"DefaultTtsModelPropertyChanged\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"model\"/>\n"
"    </signal>\n"
"    <property access=\"read\" type=\"a{sv}\" name=\"SttLangs\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <signal name=\"SttLangsPropertyChanged\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"out\" type=\"a{sv}\" name=\"langs\"/>\n"
"    </signal>\n"
"    <property access=\"read\" type=\"a{sv}\" name=\"TtsLangs\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <signal name=\"TtsLangsPropertyChanged\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"out\" type=\"a{sv}\" name=\"langs\"/>\n"
"    </signal>\n"
"    <property access=\"read\" type=\"a{sv}\" name=\"SttModels\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <signal name=\"SttModelsPropertyChanged\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"out\" type=\"a{sv}\" name=\"models\"/>\n"
"    </signal>\n"
"    <property access=\"read\" type=\"a{sv}\" name=\"TtsModels\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <signal name=\"TtsModelsPropertyChanged\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"out\" type=\"a{sv}\" name=\"models\"/>\n"
"    </signal>\n"
"    <property access=\"read\" type=\"i\" name=\"CurrentTask\"/>\n"
"    <signal name=\"CurrentTaskPropertyChanged\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </signal>\n"
"    <signal name=\"SttIntermediateTextDecoded\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"text\"/>\n"
"      <arg direction=\"out\" type=\"s\" name=\"lang\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </signal>\n"
"    <signal name=\"SttTextDecoded\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"text\"/>\n"
"      <arg direction=\"out\" type=\"s\" name=\"lang\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </signal>\n"
"    <signal name=\"SttFileTranscribeFinished\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </signal>\n"
"    <signal name=\"TtsPlaySpeechFinished\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </signal>\n"
"    <signal name=\"TtsPartialSpeechPlaying\">\n"
"      <arg direction=\"out\" type=\"s\" name=\"text\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </signal>\n"
"    <signal name=\"SttFileTranscribeProgress\">\n"
"      <arg direction=\"out\" type=\"d\" name=\"progress\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </signal>\n"
"    <method name=\"SttGetFileTranscribeProgress\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"task\"/>\n"
"      <arg direction=\"out\" type=\"d\" name=\"progress\"/>\n"
"    </method>\n"
"    <signal name=\"ErrorOccured\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"code\"/>\n"
"    </signal>\n"
"    <method name=\"SttStartListen\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"mode\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"lang\"/>\n"
"      <arg direction=\"in\" type=\"b\" name=\"translate\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </method>\n"
"    <method name=\"SttStopListen\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"task\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"result\"/>\n"
"    </method>\n"
"    <method name=\"SttTranscribeFile\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"file\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"lang\"/>\n"
"      <arg direction=\"in\" type=\"b\" name=\"translate\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </method>\n"
"    <method name=\"TtsPlaySpeech\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"text\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"lang\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"task\"/>\n"
"    </method>\n"
"    <method name=\"TtsStopSpeech\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"task\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"result\"/>\n"
"    </method>\n"
"    <method name=\"Cancel\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"task\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"result\"/>\n"
"    </method>\n"
"    <method name=\"Reload\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"result\"/>\n"
"    </method>\n"
"    <method name=\"KeepAliveTask\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"task\"/>\n"
"      <arg direction=\"out\" type=\"i\" name=\"timer\"/>\n"
"    </method>\n"
"    <method name=\"KeepAliveService\">\n"
"      <arg direction=\"out\" type=\"i\" name=\"timer\"/>\n"
"    </method>\n"
"    <property access=\"read\" type=\"a{sv}\" name=\"Translations\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <property access=\"read\" type=\"a{sv}\" name=\"TttLangs\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <signal name=\"TttLangsPropertyChanged\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"out\" type=\"a{sv}\" name=\"langs\"/>\n"
"    </signal>\n"
"    <property access=\"read\" type=\"a{sv}\" name=\"TttModels\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
"    </property>\n"
"    <signal name=\"TttModelsPropertyChanged\">\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"out\" type=\"a{sv}\" name=\"models\"/>\n"
"    </signal>\n"
"  </interface>\n"
        "")
public:
    SpeechAdaptor(QObject *parent);
    virtual ~SpeechAdaptor();

public: // PROPERTIES
    Q_PROPERTY(int CurrentTask READ currentTask)
    int currentTask() const;

    Q_PROPERTY(QString DefaultSttLang READ defaultSttLang WRITE setDefaultSttLang)
    QString defaultSttLang() const;
    void setDefaultSttLang(const QString &value);

    Q_PROPERTY(QString DefaultSttModel READ defaultSttModel WRITE setDefaultSttModel)
    QString defaultSttModel() const;
    void setDefaultSttModel(const QString &value);

    Q_PROPERTY(QString DefaultTtsLang READ defaultTtsLang WRITE setDefaultTtsLang)
    QString defaultTtsLang() const;
    void setDefaultTtsLang(const QString &value);

    Q_PROPERTY(QString DefaultTtsModel READ defaultTtsModel WRITE setDefaultTtsModel)
    QString defaultTtsModel() const;
    void setDefaultTtsModel(const QString &value);

    Q_PROPERTY(int Speech READ speech)
    int speech() const;

    Q_PROPERTY(int State READ state)
    int state() const;

    Q_PROPERTY(QVariantMap SttLangs READ sttLangs)
    QVariantMap sttLangs() const;

    Q_PROPERTY(QVariantMap SttModels READ sttModels)
    QVariantMap sttModels() const;

    Q_PROPERTY(QVariantMap Translations READ translations)
    QVariantMap translations() const;

    Q_PROPERTY(QVariantMap TtsLangs READ ttsLangs)
    QVariantMap ttsLangs() const;

    Q_PROPERTY(QVariantMap TtsModels READ ttsModels)
    QVariantMap ttsModels() const;

    Q_PROPERTY(QVariantMap TttLangs READ tttLangs)
    QVariantMap tttLangs() const;

    Q_PROPERTY(QVariantMap TttModels READ tttModels)
    QVariantMap tttModels() const;

public Q_SLOTS: // METHODS
    int Cancel(int task);
    int KeepAliveService();
    int KeepAliveTask(int task);
    int Reload();
    double SttGetFileTranscribeProgress(int task);
    int SttStartListen(int mode, const QString &lang, bool translate);
    int SttStopListen(int task);
    int SttTranscribeFile(const QString &file, const QString &lang, bool translate);
    int TtsPlaySpeech(const QString &text, const QString &lang);
    int TtsStopSpeech(int task);
Q_SIGNALS: // SIGNALS
    void CurrentTaskPropertyChanged(int task);
    void DefaultSttLangPropertyChanged(const QString &lang);
    void DefaultSttModelPropertyChanged(const QString &model);
    void DefaultTtsLangPropertyChanged(const QString &lang);
    void DefaultTtsModelPropertyChanged(const QString &model);
    void ErrorOccured(int code);
    void SpeechPropertyChanged(int speech);
    void StatePropertyChanged(int state);
    void SttFileTranscribeFinished(int task);
    void SttFileTranscribeProgress(double progress, int task);
    void SttIntermediateTextDecoded(const QString &text, const QString &lang, int task);
    void SttLangsPropertyChanged(const QVariantMap &langs);
    void SttModelsPropertyChanged(const QVariantMap &models);
    void SttTextDecoded(const QString &text, const QString &lang, int task);
    void TtsLangsPropertyChanged(const QVariantMap &langs);
    void TtsModelsPropertyChanged(const QVariantMap &models);
    void TtsPartialSpeechPlaying(const QString &text, int task);
    void TtsPlaySpeechFinished(int task);
    void TttLangsPropertyChanged(const QVariantMap &langs);
    void TttModelsPropertyChanged(const QVariantMap &models);
};

#endif