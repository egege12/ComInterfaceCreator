#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>

class dataContainer : public QObject
{
    Q_OBJECT
public:
    struct signal;
    //Total counters
    static unsigned long long messageCounter;
    static unsigned long long signalCounter;
    static QMap<QString,QList<QString>> warningMessages;
    static QList<QString> infoMessages;
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
    QString getMsTimeOut();
    QString getMsCycleTime();
    QString getComment();
    bool getIfSelected();
    bool getIfExtended();
    bool getIfNotSelectable();
    unsigned short getDLC();
    static const QList<QString> getWarningList();
    static const QList<QString> getMsgWarningList(QString ID);
    static const QList<QString> getInfoList();
    //Setters
    void setName(QString Name);
    void setmessageID(QString messageID);
    void setDLC(unsigned short DLC);
    void setSelected();
    void setNotSelectable();
    void setInserted();
    void setMsTimeOut(QString msTimeout);
    void setMsCycleTime(QString msCycleTime);
    void setComment(QString comment);
    void setExtended(bool isExtended);
    static void setWarning(QString ID,QString const& warningCode);

    bool isTmOutSet;
    bool isCycleTmSet;

    ~dataContainer();
private:
    QString Name;
    QString messageID;
    QString msTimeout;
    QString msCycleTime;
    QString comment;
    unsigned short dlc;
    bool isExtended;
    bool isSelected;
    bool isInserted;
    bool isNotSelectable;
    QList<signal*> signalList;
    void signalChecker(signal *signalPtr);

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
    double defValue;
    QString comment;
    QString appDataType;
    QString comDataType;
    QString convMethod;
    bool isJ1939;

};


#endif // DATACONTAINER_H
