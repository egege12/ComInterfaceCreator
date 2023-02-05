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
    bool isSelected;

public:

    explicit dataContainer(QObject *parent = nullptr);

    //Total counters
    static unsigned int messageCounter;
    static unsigned int signalCounter;

    //Struct List
    struct signal;
    QList<signal*> message;
    //Data type assigner
    void dataTypeAss(signal *signalPtr);
    //Printers
    void printAll();
    //Getters
    QVector<QString> getMessageVector();
    QVector<QVector<QString>> getSignalVector();

    bool getIfSelected();
    //Setters
    void setName(QString Name);
    void setmessageID(QString messageID);
    void setDLC(unsigned short DLC);
    bool setSelected();
    //Signal adder
    bool addSignal(signal newSignal);

    ~dataContainer();

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
    bool isJ1939;

};


#endif // DATACONTAINER_H
