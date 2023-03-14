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
#include <QDomDocument>
#include <QDateTime>
#include <random>



typedef QMap<QString,dataContainer*>  interface;
typedef QList<QList<QString>> fbType;
class DBCHandler : public QObject
{
    Q_OBJECT


public:

    static unsigned long long selectedMessageCounter;
    static QList<QList<QString>> pouHolder;
    static unsigned counterfbBYTETOWORD;
    static unsigned counterfbBYTETODWORD;
    static unsigned counterfbBYTETOLWORD;
    static unsigned counterfb8BITTOBYTE;
    static unsigned counterfbDWORDTOLWORD;
    static unsigned counterfbLWORDTOBYTE;
    static unsigned counterfbDWORDTOBYTE;
    static unsigned counterfbWORDTOBYTE;
    static unsigned counterfbBYTETO8BIT;
    static bool allSelected;
    explicit DBCHandler(QObject *parent = nullptr);
    ~DBCHandler();
    QDateTime creationDate;

/*PROPERTIES STARTS*/
    Q_PROPERTY(QString errCode READ errCode WRITE setErrCode NOTIFY errCodeChanged)
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)
/*PROPERTIES ENDS*/
/*ERROR CODE- It stored as texts but will be converted into database for language*/
    QString errCode() const;
    void setErrCode(const QString &newErrCode);
/*END*/
/*Signal list getter - currently not used anywhere*/
    const dataContainer *getMessage(QString messageID);
    /*END*/
    qreal progress() const;
    void setProgress(qreal newProgress);
    qreal m_progress;
    QString displayReqSignalID;
public slots:

    void update();
    void clearData();
    void readFile(QString fileLocation);
    void setSelected(QString messageID);
    void setAllSelected();
    void setDisplayReqSignal(QString signalID);
    void setFolderLoc(QString folderLoc);
    void setDutName(QString dutName);
    void setIOType(QString setIOType);
    void setCANLine(QString canName);
    void setTestMode(bool checkStat);
    //Tableview format nested vectors
    QList<QList<QString>> messagesList();
    QList<QList<QString>> signalsList();
    void startToGenerate();
    QList<QString> getWarningList();
    QList<QString> getMsgWarningList();
    QList<QString> getInfoList();
    static bool getAllSelected();
    //Worker thread functions


    /*QList<QString> getInfoList();*/
signals:

    void interfaceReady();
    void selectedViewChanged();
    void selectedStatChanged();
    void procesStarted();
    void errCodeChanged();
    void fileandLockOk();
    void allSelectedChanged();
    void progressChanged();
    void progressCompleted();
    void progressStarted();
    void infoListChanged();



private:

    //Private Variables starts
    //***********************************
    interface comInterface;
    struct structFbdBlock;
    QList<structFbdBlock*> fbdBlocks;
    QString folderLoc;
    QString dbcPath;

    QString dutName;
    QString dutHeader;
    QString IOType;
    bool isAllInserted;
    QString m_errCode;
    fbType fbNameandObjId;
    QString dutObjID;
    QString pouObjID;
    QString canLine;
    bool  enableTest;
    //***********************************
    //Private Variables ends
    //Private methods starts
    //***********************************

    //Private methods ends
    //***********************************
    // Reading Process from dbc file start
    //***********************************
    void openFile();
    bool parseMessages(QFile *ascFile);
    bool generateNewMessage(QString messageID, QString messageName , unsigned short messageDLC, bool isExtended);
    bool createObjIds();
    QString getRandomID();

    bool addSignalToMessage(QString messageID, dataContainer::signal curSignal);
    unsigned short parseLength(QString  splitedPart);
    unsigned short parseStartBit(QString  splitedPart);
    double parseResolution(QString  splitedPart);
    double parseOffset(QString  splitedPart);
    double parseMaxValue(QString  splitedPart);
    double parseMinValue(QString  splitedPart);
    QString parseComment(QString splitedPart);
    QString getBetween(QString first, QString second,QString fullText);
    void setTmOutCycleTmWarnings();
    void checkRepatedSignal();
    //***********************************
    // Reading Process from dbc file end
    //Generate XML file start
    //***********************************
    void extracted();
    bool createXml_STG1(QFile *xmlFile);
    void generateVariables(QDomElement*strucT,QDomDocument &doc);
    void generateIIPous(QDomElement*pous,QDomDocument &doc);
    void extracted(QDomDocument &doc, QDomText &text,
                   dataContainer *&curMessage);
    void generateIOPous(QDomElement *pous, QDomDocument &doc);
    void generateHandlers(QDomElement *pous, QDomDocument &doc);
    void generatePouFpd(QDomElement *pous, QDomDocument &doc);
    //***********************************
    //Generate XML file end
    //***********************************
    //Conversion Methods
    void generateIIST(QString *const ST, dataContainer * const curMessage);
    void generateIOST(QString *const ST,dataContainer *const curMessage);
    QString convTypeComtoApp(QString signalName,  unsigned short startbit, unsigned short length, QString converType);
    QString convTypeApptoCom(QString  signalName, unsigned short  startbit, unsigned short length, QString converType);


};
struct DBCHandler::structFbdBlock{
    //Datas can be impoerted from DBC file
    QString name;
    QList<QList<QString>> inputVars;// List of {name}
    QList<QList<QString>> outputVars;// List of {name}
};
#endif // ASCHANDLER_H
