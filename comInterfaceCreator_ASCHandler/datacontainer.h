#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>

class dataContainer : public QObject
{
    Q_OBJECT
public:
    struct signal;
    //Total counters
    static unsigned int messageCounter;
    static unsigned int signalCounter;

    explicit dataContainer(QObject *parent = nullptr);

    //Struct List

    //Data type assigner
    void dataTypeAss(signal *signalPtr);

    //Signal adder
    bool addSignal(signal newSignal);

    //Getters
    const QList<signal*> * getSignalList();
    QString getName();
    QString getID();
    bool getIfSelected();
    unsigned short getDLC();

    //Setters
    void setName(QString Name);
    void setmessageID(QString messageID);
    void setDLC(unsigned short DLC);
    void setSelected();
    void setInserted();


    ~dataContainer();
private:
    QString Name;
    QString messageID;
    unsigned short dlc;
    bool isExtended;
    bool isSelected;
    bool isInserted;

    QList<signal*> signalList;
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
