#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>




class dataContainer : public QObject
{
    Q_OBJECT
    struct signal;
    QString Name;
    QString messageID;
    unsigned int dlc;
    bool isExtended;
    QList<signal*> message;


public:
    explicit dataContainer(QObject *parent = nullptr, QString Name = "Null",QString messageID = "0X00000000", unsigned int dlc = 8, bool isExtended = true);
    bool addSignal(signal newSignal);
    void printAll();
    ~dataContainer();
     bool isSelected;
signals:

};



#endif // DATACONTAINER_H
