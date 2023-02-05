#ifndef ASCHANDLER_H
#define ASCHANDLER_H

#include <QObject>
#include "datacontainer.h"
#include <QMap>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>
typedef QMap<QString,dataContainer*>  interface;

class DBCHandler : public QObject
{
    Q_OBJECT

    interface comInterface;
    bool parseMessages(QFile *ascFile);
    bool generateNewMessage(QString messageID, QString messageName , unsigned short messageDLC);
    bool addSignalToMessage(QString messageID, dataContainer::signal curSignal);
    unsigned short parseLength(QString  splitedPart);
    unsigned short parseStartBit(QString  splitedPart);
    double parseResolution(QString  splitedPart);
    double parseOffset(QString  splitedPart);
    double parseMaxValue(QString  splitedPart);
    double parseMinValue(QString  splitedPart);
    QString parseComment(QString splitedPart);

public:
    explicit DBCHandler(QObject *parent = nullptr, QString fileLocation = " ");

    void printMessages();
    void printSelectedMessages();
//Tableview format nested vectors
    QList<QList<QString>> messagesVector();
    QList<QList<QString>> signalsVector(QString messageID);
public slots:
    bool selectMessage(QString messageID);
    dataContainer const* getMessage(QString messageID);
signals:

};

#endif // ASCHANDLER_H
