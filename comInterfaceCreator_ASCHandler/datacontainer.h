#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>




class dataContainer : public QObject
{
    Q_OBJECT

    QString Name;
    QString messageID;
    unsigned short dlc;
    bool isExtended;


public:
    struct signal;
    QList<signal*> message;
    explicit dataContainer(QObject *parent = nullptr);
    bool addSignal(signal newSignal);
    void printAll();
    void setName(QString Name);
    void setmessageID(QString messageID);
    void setDLC(unsigned short DLC);
    ~dataContainer();
     bool isSelected;
signals:

};

struct dataContainer::signal{
    //Datas can be impoerted from DBC file
    QString name;
    unsigned short length;
    unsigned short startBit;
    double resolution;
    double offset;
    double maxValue;
    double minValue;
    QString comment;
    QString appDataType;
    QString comDataType;
    QString convDataType;
    bool isJ1939;

};


#endif // DATACONTAINER_H
