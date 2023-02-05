#ifndef ASCHANDLER_H
#define ASCHANDLER_H

#include <QObject>
#include "datacontainer.h"
#include <QMap>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QFileSystemWatcher>
#include <QTimer>
typedef QMap<QString,dataContainer*>  interface;

class DBCHandler : public QObject
{
    Q_OBJECT

public:
    explicit DBCHandler(QObject *parent = nullptr);
    void readFile(QString fileLocation);
//Tableview format nested vectors
    QList<QList<QString>> messagesVector();
    QList<QList<QString>> signalsVector(QString messageID);

    const dataContainer *getMessage(QString messageID);


public slots:
    bool selectMessage(QString messageID);
    void update();
signals:

private:
    interface comInterface;
    QFileSystemWatcher watcher;
    QTimer timer;
    // Reading Process from asc file

    void openFile();
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
    // Reading Process from asc file

    bool isAllInserted;
    QString dbcPath;
};

#endif // ASCHANDLER_H
