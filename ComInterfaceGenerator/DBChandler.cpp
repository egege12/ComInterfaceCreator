#include "DBChandler.h"
#include "datacontainer.h"
#include "qforeach.h"
#include <QtGlobal>
#include <QRegularExpression>
#include <QUuid>
#include <QHostInfo>

unsigned long long DBCHandler::selectedMessageCounter = 0;
unsigned int DBCHandler::counterfbBYTETOWORD = 0;
unsigned int DBCHandler::counterfbBYTETODWORD = 0;
unsigned int DBCHandler::counterfbBYTETOLWORD = 0;
unsigned int DBCHandler::counterfb8BITTOBYTE = 0;
unsigned int DBCHandler::counterfbDWORDTOLWORD = 0;
unsigned int DBCHandler::counterfbLWORDTOBYTE = 0;
unsigned int DBCHandler::counterfbDWORDTOBYTE = 0;
unsigned int DBCHandler::counterfbWORDTOBYTE = 0;
unsigned int DBCHandler::counterfbBYTETO8BIT = 0;
bool DBCHandler::allSelected=false;


DBCHandler::DBCHandler(QObject *parent)
    : QObject{parent}
{
    this->isAllInserted = false;


    if(QFile::exists("log.txt")){
        QFile logFile("log.txt");
        if (!logFile.open(QIODevice::ReadOnly| QIODevice::Text )){
            dataContainer::setWarning("INFO","Kayıt defteri açılamadı");
        }else{
            QTextStream lines(&logFile);
            while (!lines.atEnd()) {
                QString curLine = lines.readLine();
                dataContainer::infoMessages.append(curLine);
            }
            dataContainer::setWarning("INFO","Kayıt defteri okundu");
            logFile.close();
        }

    }else{
            dataContainer::setWarning("INFO","Kayıt defteri bulunamadı");
    }

    this->canLine = "GVL.IC.Obj_CAN0";
    this->enableTest=false;

    dataContainer::setWarning("INFO","Program başlatıldı");
}

DBCHandler::~DBCHandler()
{
        dataContainer::setWarning("INFO",dbcPath+"dosyası kapatıldı");
        QFile logFile("log.txt");
        if (!logFile.open(QIODevice::WriteOnly| QIODevice::Truncate )){
            setErrCode("Yapılan değişiklikler kayıt defterine işlenemedi");
        }else{
            QTextStream out(&logFile);
            foreach(QString logLine , dataContainer::infoMessages){
                out<<logLine<<Qt::endl;
            }
            dataContainer::setWarning("INFO","Kayıt defteri yazıldı.");
            logFile.close();
        }

    isAllInserted = false;
    for(dataContainer * curValue : comInterface){
        delete curValue;
    }
    comInterface.clear();
    setErrCode("Tablo temizlendi");
    dataContainer::warningMessages.clear();
    dataContainer::setWarning("INFO","Tablo temizlendi");

}

QString DBCHandler::errCode() const
{
    return m_errCode;
}

void DBCHandler::setErrCode(const QString &newErrCode)
{
    this->m_errCode= newErrCode;
    emit errCodeChanged();
    qInfo()<<"Error: "<<m_errCode;
}

QList<QList<QString>> DBCHandler::messagesList()
{
    QList<QList<QString>> data;
    if (isAllInserted){
        QList<QList<QString>> data;
        data.append({"  ","İsim","ID(HEX)","DLC","Çevrim Pryd.[ms]","Zaman Aşımı[ms]"});
        foreach(dataContainer *const curValue , comInterface){
            data.append({curValue->getIfSelected() ? "X" : "O" ,curValue->getName(),curValue->getID(),QString::number(curValue->getDLC()),curValue->getMsCycleTime(),curValue->getMsTimeOut()});
        }
        return data;
    }
}

QList<QList<QString> > DBCHandler::signalsList()
{
     QList<QList<QString>> dataSignal;
    if (isAllInserted){
        dataSignal.append({"İsim","Başlangıç","Boyut","Ölçek","Ofset","Minimum","Maksimum","Varsayılan","J1939","Uyg. Veri Tipi","Hbr.  Veri Tipi","Birim","Yorum"});
        for ( const dataContainer::signal *data : *comInterface.value(this->displayReqSignalID)->getSignalList()){
            dataSignal.append({data->name,QString::number(data->startBit),QString::number(data->length),QString::number(data->resolution),QString::number(data->offset),QString::number(data->minValue,'g',(data->length>32)? 20:15),QString::number(data->maxValue,'g',(data->length>32)? 20:15),QString::number(data->defValue,'g',(data->length>32)? 20:15),data->isJ1939?"+":"-",data->appDataType,data->comDataType,data->unit.toUtf8(),data->comment.toUtf8()});
        }
        return dataSignal;
    }
}

void DBCHandler::update()
{
    isAllInserted = false;
    foreach(dataContainer * curValue , comInterface){
        delete curValue;
    }
    comInterface.clear();
    qInfo()<<"Updating";
    openFile();
}

void DBCHandler::clearData()
{
    isAllInserted = false;
    dataContainer::setWarning("INFO",dbcPath+"dosyası kapatıldı");
    isAllInserted = false;
    for(dataContainer * curValue : comInterface){
        delete curValue;
    }
    comInterface.clear();
    setErrCode("Tablo temizlendi");
    dataContainer::setWarning("INFO","Tablo temizlendi");
    for(structFbdBlock * curValue : fbdBlocks){
        delete curValue;
    }
    fbdBlocks.clear();
    dutName="null";
    dutHeader="null";
    IOType="null";
    fbNameandObjId.clear();
    dutObjID="null";
    pouObjID="null";
    DBCHandler::selectedMessageCounter=0;

}

void DBCHandler::readFile(QString fileLocation)
{
    dbcPath = fileLocation;
    if(this->isAllInserted){
        this->setErrCode("DBC zaten içeri aktarılmış, programı yeniden başlatın");
        qInfo()<<"alreadyinserted";
    }else{
        openFile();
    }
}

void DBCHandler::openFile()
{
    try {
        if (dbcPath.isEmpty()){
            throw QString("Dosya konumu boş olamaz!");
        }else if(!dbcPath.contains(".dbc")){
            throw QString("Lütfen \".dbc\" uzantılı bir dosya seçin!");
        }
        else{
            QFile *ascFile = new QFile(dbcPath);
            if(!ascFile->open(QIODevice::ReadOnly | QIODevice::Text)){
                throw QString("Dosya açılamadı, lütfen konumu kontrol edin!");
            }
            else{
                dataContainer::setWarning("INFO",dbcPath+"dosyası açıldı");
                emit fileandLockOk();
                if (!parseMessages(ascFile)){
                    ascFile->close();
                    throw QString("Arayüzü okurken bir şeyler yanlış gitti!");
                }else{
                    setTmOutCycleTmWarnings();
                    emit interfaceReady();
                    this->setDisplayReqSignal(comInterface.firstKey());
                }
                ascFile->close();
            }

        }
    } catch (QString text) {
        this->setErrCode(text);
    }

}
///******************************************************************************
/// MESSAGE AND APPLICATION DATA HANDLERS
///******************************************************************************
const dataContainer *DBCHandler::getMessage(QString messageID)
{

    return comInterface.value(messageID);
}

void DBCHandler::setSelected(QString messageID)
{
    if(!comInterface.value(messageID)->getIfNotSelectable()){
        comInterface.value(messageID)->setSelected();
        qInfo()<<"Selection status changed to "<<QString::number(comInterface.value(messageID)->getIfSelected()) <<"for message ID:"<<displayReqSignalID;
        if(comInterface.value(messageID)->getIfSelected()){
            DBCHandler::selectedMessageCounter++;
        }else{
            DBCHandler::selectedMessageCounter--;
        }
        emit selectedStatChanged();
    }else{
        emit selectedStatChanged();
        setErrCode(messageID+" seçime uygun değil, uyarıları kontrol et");
    }
}

void DBCHandler::setAllSelected()
{
    for(dataContainer *const curValue : comInterface){
      if(!curValue->getIfNotSelectable()){
        if(DBCHandler::allSelected && curValue->getIfSelected()){
           curValue->setSelected();
           DBCHandler::selectedMessageCounter++;
        }
        if(!DBCHandler::allSelected && !curValue->getIfSelected()){
           curValue->setSelected();
           DBCHandler::selectedMessageCounter--;
        }
        emit selectedStatChanged();
      }else{
          if(!DBCHandler::allSelected){
          dataContainer::setWarning("INFO",curValue->getID() + "mesajı seçime uygun değil,tümünü seçime dahil edilmedi.");
          setErrCode("Seçilemeyen mesaj/mesajlar var");
          }
        emit selectedStatChanged();
      }

    }
    DBCHandler::allSelected=!DBCHandler::allSelected;
    emit allSelectedChanged();
}

void DBCHandler::setDisplayReqSignal(QString signalID)
{
    this->displayReqSignalID = signalID;
    qInfo()<<"Signal set to display"<<displayReqSignalID;
    emit selectedViewChanged();
}

void DBCHandler::setFolderLoc(QString folderLoc)
{
    this->folderLoc = folderLoc;
}

void DBCHandler::setDutName(QString dutName)
{
    this->dutName = dutName;
    this->dutHeader = dutName.remove("_T");

    qInfo()<<"Dut name set to :"<<dutName;
}

void DBCHandler::setIOType(QString setIOType)
{
    this->IOType = setIOType;
}

void DBCHandler::setCANLine(QString canName){
    this->canLine = "GVL.IC.Obj_"+canName;
}

void DBCHandler::setTestMode(bool checkStat)
{
    this->enableTest = checkStat;
}

void DBCHandler::setFrcVar(bool checkStat)
{
    this->enableFrc = checkStat;
}

bool DBCHandler::parseMessages(QFile *ascFile)
{
    QTextStream  *lines = new QTextStream(ascFile);
    bool inlineOfMessage=false;
    bool inlineOfMessageOld=false;
    QString messageID;
    QString messageName ;
    unsigned short  messageDLC ;
    QList<QList<QString>> msgCommentList;
    unsigned long debugcounter=0;
    bool isExtended = 0;
    bool secLineFlagCmMes = false;
    bool secLineFlagCmSig = false;
    QString secLineContainer = "";
    while (!lines->atEnd()) {
        QString curLine = lines->readLine();

        /*Message parse - split message line to items and rise message block flasg (inlineOfMessage)*/
        if(curLine.contains("BO_ ") && curLine.indexOf("BO_") < 2 && !(curLine.contains("INDEPENDENT")&&curLine.contains("VECTOR"))){
            inlineOfMessage = true;
            debugcounter++;

            QStringList messageList = curLine.split(" ");
            if(messageList.at(1).toUInt()>2047){
                messageID = QString::number((messageList.at(1).toUInt())-2147483648,16).toUpper();
                isExtended=true;
            }else{
                messageID = QString::number(messageList.at(1).toUInt(),16).toUpper();
                isExtended=false;
            }

            messageName= messageList.at(2);
            messageName.remove(QChar(':'),Qt::CaseInsensitive).replace(" ","_");;
            messageDLC = messageList.at(3).toUShort();
            if (!comInterface.contains(messageID)){
                generateNewMessage(messageID,messageName,messageDLC,isExtended);
            }
            /*Signal parse - split signal lines to items*/
        }else if(inlineOfMessage && curLine.contains("SG_")){

            dataContainer::signal curSignal;
            curSignal.name = getBetween("SG_",":",curLine).trimmed().replace(" ","_");
            curSignal.startBit = parseStartBit(getBetween(":","(",curLine));
            curSignal.length = parseLength(getBetween(":","(",curLine));
            curSignal.resolution = parseResolution(getBetween("(",")",curLine));
            curSignal.offset = parseOffset(getBetween("(",")",curLine));
            curSignal.minValue = parseMinValue(getBetween("[","]",curLine));
            curSignal.maxValue = parseMaxValue(getBetween("[","]",curLine));
            QString commentContainer = parseComment(curLine);
            curSignal.unit = commentContainer.toUtf8();
            curSignal.isJ1939 = false; // get default , it set on comment
            curSignal.defValue=0.0; // get default , it set on comment
            curSignal.isMotorola=curLine.contains("@0+");//Signal motorola or intel
            addSignalToMessage (messageID,curSignal);
 /*Append and manupulate message comments*/
/************************************************/
        }else if(((curLine.contains("CM_"))&& curLine.contains("BO_")) || secLineFlagCmMes ){
            unsigned i;
            if(!secLineFlagCmMes && curLine.contains("\";")){
                secLineContainer = curLine;
                i=10;
            }else if (secLineFlagCmMes && curLine.contains("\";")){
                secLineFlagCmMes = false;
                secLineContainer.append(curLine);
                i=20;
            }else if (secLineFlagCmMes && !curLine.contains("\";")){
                secLineContainer.append(curLine);
                i=30;
            }else{
                i=40;
                secLineContainer = curLine;
                secLineFlagCmMes = true;
            }

            if((i == 10) || (i ==20)){
                QStringList commentLine = secLineContainer.split(" ");
                QString commentContainer =  secLineContainer.contains("\"") && secLineContainer.contains("\";")? getBetween("\"","\";",secLineContainer):"";
                QString configComment = secLineContainer.contains("[*") && secLineContainer.contains("*]")? getBetween("[*","*]",secLineContainer) : "";
                QString ID ="";
                if(commentLine.at(2).toUInt()>2047){
                    ID = QString::number((commentLine.at(2).toUInt())-2147483648,16).toUpper();
                }else{
                    ID = QString::number(commentLine.at(2).toUInt(),16).toUpper();
                }
                QString msTimeout="";
                QString msCycleTime="";
                if(configComment.contains("timeout",Qt::CaseInsensitive)){
                    msTimeout = this->getBetween("timeout","ms",configComment);
                    if(comInterface.contains(ID))
                        comInterface.value(ID)->isTmOutSet = true;
                }
                if(configComment.contains("cycletime",Qt::CaseInsensitive)){
                    msCycleTime = this->getBetween("cycletime","ms",configComment);
                    if(comInterface.contains(ID))
                        comInterface.value(ID)->isCycleTmSet = true;
                }
                qInfo()<<configComment;
                msgCommentList.append({ID,msTimeout,msCycleTime,commentContainer.remove(configComment)});
                if (configComment.contains("j1939",Qt::CaseInsensitive)){
                    for( dataContainer::signal * curSignal : *comInterface.value(ID)->getSignalList()){
                        curSignal->isJ1939 = true;
                    }
                }
                secLineContainer="";
                i=0;
            }

        }else if(((curLine.contains("CM_"))&& curLine.contains("SG_")) || secLineFlagCmSig ){

            unsigned i;
            if(!secLineFlagCmSig && curLine.contains("\";")){
                secLineContainer = curLine;
                i=10;
            }else if (secLineFlagCmSig && curLine.contains("\";")){
                secLineFlagCmSig = false;
                secLineContainer.append(curLine);
                i=20;
            }else if (secLineFlagCmSig && !curLine.contains("\";")){
                secLineContainer.append(curLine);
                i=30;
            }else{
                i=30;
                secLineContainer = curLine;
                secLineFlagCmSig = true;
            }
            if((i == 10) || (i ==20)){
                QStringList commentLine = secLineContainer.split(" ");
                QString commentContainer =  secLineContainer.contains("\"") && secLineContainer.contains("\";")? getBetween("\"","\";",secLineContainer):"";
                QString configComment = secLineContainer.contains("[*") && secLineContainer.contains("*]")? getBetween("[*","*]",secLineContainer) : "";
                QString targetID;
                if(commentLine.at(2).toUInt()>2047){
                    targetID = QString::number((commentLine.at(2).toUInt())-2147483648,16).toUpper();
                }else{
                    targetID = QString::number(commentLine.at(2).toUInt(),16).toUpper();
                }
                double defValue;
                if(configComment.contains(QString("defValue"),Qt::CaseInsensitive)){
                    defValue=this->getBetween("sDefValue","eDefValue",configComment.remove("=").remove(":")).toDouble();
                }else{
                    defValue=0.0;
                }
                if(comInterface.contains(targetID)){
                    for( dataContainer::signal * curSignal : *comInterface.value(targetID)->getSignalList()){
                        if( curSignal->name.contains(commentLine.at(3))){
                            curSignal->comment=commentContainer.remove(configComment);
                            curSignal->isJ1939 = (configComment.contains(QString("j1939"),Qt::CaseInsensitive))? true: curSignal->isJ1939 ;
                            curSignal->defValue= defValue;
                        }
                    }
                }
                secLineFlagCmSig=false;
            }

        }else if((curLine.contains("BA_")) && curLine.contains("SG_")){   //Catch BA_ SA_  signal parameters

            QStringList containerLine = curLine.remove(";").split(" ");
            if(containerLine.at(1).contains(QString("GenSigStartValue"),Qt::CaseInsensitive)){
                QString targetID;
                if(containerLine.at(3).toUInt()>2047){
                    targetID = QString::number((containerLine.at(3).toUInt())-2147483648,16).toUpper();
                }else{
                    targetID = QString::number(containerLine.at(3).toUInt(),16).toUpper();
                }
                double defValue=containerLine.at(5).toDouble();

                if(comInterface.contains(targetID)){
                    for( dataContainer::signal * curSignal : *comInterface.value(targetID)->getSignalList()){
                        if( curSignal->name.contains(containerLine.at(4))){
                            curSignal->defValue= defValue;
                        }
                    }
                }
            }

        }else if((curLine.contains("BA_")) && curLine.contains("BO_")){   //Catch BA_ BO_  signal parameters

            QStringList containerLine = curLine.remove(";").split(" ");
            if(containerLine.at(1).contains(QString("GenMsgCycleTime"),Qt::CaseInsensitive)){
                QString targetID;
                if(containerLine.at(3).toUInt()>2047){
                    targetID = QString::number((containerLine.at(3).toUInt())-2147483648,16).toUpper();
                }else{
                    targetID = QString::number(containerLine.at(3).toUInt(),16).toUpper();
                }

                if(comInterface.contains(targetID)){
                    comInterface.value(targetID)->setMsCycleTime(containerLine.at(4));
                    comInterface.value(targetID)->isCycleTmSet = true;
                }
            }

        }else{
            inlineOfMessage = false;
        }
        if (inlineOfMessageOld && !inlineOfMessage){
            comInterface.value(messageID)->setInserted();
        }
        inlineOfMessageOld = inlineOfMessage;

    }
    delete lines;
    /*Assing the message comments on the list*/
    for(dataContainer *const curValue : comInterface){
        for (QList<QString> contMessage: msgCommentList){
            if(contMessage.at(0)==curValue->getID()){
                if(!contMessage.at(1).isEmpty()){
                    curValue->setMsTimeOut(contMessage.at(1));
                }
                if(!contMessage.at(2).isEmpty()){
                    curValue->setMsCycleTime(contMessage.at(2));
                }
                curValue->setComment(contMessage.at(3));
            }
        }
    }

    this->isAllInserted = true;
    checkRepatedSignal();
    return true;
}
//BO_ <ID> <Message_name>: <DLC> Vector__XXX -> for messages
//SG_ <Name> : <Bit order> | <Bit length>@1+ (<resolution>,<offset>) [<min_value>|max_value] "comment" -> for signals

bool DBCHandler::generateNewMessage(QString messageID, QString messageName , unsigned short messageDLC, bool isExtended)
{
    dataContainer* newMessage = new dataContainer();
    newMessage->setName(messageName);
    newMessage->setmessageID(messageID);
    newMessage->setDLC(messageDLC);
    newMessage->setExtended(isExtended);
    comInterface.insert(messageID,newMessage);
    return true;
}

bool DBCHandler::createObjIds()
{
    this->dutObjID = this->getRandomID();
    this->pouObjID = this->getRandomID();

    return true;
}


QString DBCHandler::getRandomID()
{
    QString str;
    // random hex string generator
        for (int i = 0; i < 10; i++)
        {
            str = QUuid::createUuid().toString();
            str.remove(QRegularExpression("{|}|")); // only hex numbers

        }
    return str;

}

bool DBCHandler::addSignalToMessage(QString messageID,dataContainer::signal curSignal)
{
    comInterface.value(messageID)->addSignal(curSignal);
    return true;
}
///******************************************************************************
/// STRING MANUPULATORS
///******************************************************************************
unsigned short DBCHandler::parseLength(QString splitedPart)
{
    splitedPart.remove("@1+");
    splitedPart.remove("@1-");
    QStringList container = splitedPart.trimmed().split("|");
    return container.at(1).toUShort();
}

unsigned short DBCHandler::parseStartBit(QString  splitedPart)
{
    splitedPart.remove("@1+");
    splitedPart.remove("@1-");
    QStringList container = splitedPart.trimmed().split("|");
    return container.at(0).toUShort();
}

double DBCHandler::parseResolution(QString  splitedPart)
{
    splitedPart.remove("(");
    splitedPart.remove(")");
    QStringList container = splitedPart.trimmed().split(",");
    return container.at(0).toDouble();
}

double DBCHandler::parseOffset(QString  splitedPart)
{
    splitedPart.remove("(");
    splitedPart.remove(")");
    QStringList container = splitedPart.trimmed().split(",");
    return container.at(1).toDouble();
}

double DBCHandler::parseMaxValue(QString  splitedPart)
{
    splitedPart.remove("[");
    splitedPart.remove("]");
    QStringList container = splitedPart.trimmed().split("|");
    return container.at(1).toDouble();
}

double DBCHandler::parseMinValue(QString  splitedPart)
{
    splitedPart.remove("[");
    splitedPart.remove("]");
    QStringList container = splitedPart.trimmed().split("|");
    return container.at(0).toDouble();
}

QString DBCHandler::parseComment(QString splitedPart)
{
    QString comment = splitedPart.mid(splitedPart.indexOf("]")+1,((splitedPart.indexOf("Vector__XXX")))-(splitedPart.indexOf("]")+1));
    comment.remove("\"");
    comment.remove("Vector__XXX");
    return comment.trimmed();
}

QString DBCHandler::getBetween(QString first, QString second, QString fullText)
{
    if(fullText.contains(first,Qt::CaseInsensitive)){
        qsizetype indexCont1 = fullText.indexOf(first,Qt::CaseInsensitive);
        fullText= fullText.sliced(indexCont1);
        indexCont1 = fullText.indexOf(first,Qt::CaseInsensitive);
        if(fullText.contains(QString(second),Qt::CaseInsensitive)){
            qsizetype indexCont2 = fullText.indexOf(second,Qt::CaseInsensitive);
            return fullText.mid(indexCont1,(indexCont2-indexCont1)).trimmed().remove("=").remove(":").remove(first).trimmed();
        }else
            return "(*ERROR*)";
    }return "(*ERROR*)";
}

void DBCHandler::setTmOutCycleTmWarnings()
{
    foreach (dataContainer * curMessage , comInterface){
        if(!curMessage->isCycleTmSet){
            dataContainer::setWarning(curMessage->getID(),"mesajı için cycletime belirtilmediği için 100ms atandı");
        }
        if((!curMessage->isTmOutSet)&& (!curMessage->isCycleTmSet)){
            dataContainer::setWarning(curMessage->getID(),"mesajı için timeout belirtilmediği için 2500ms atandı");
        }else if((curMessage->isCycleTmSet)&&(!curMessage->isTmOutSet)){
            curMessage->setMsTimeOut(QString::number(curMessage->getMsCycleTime().toUInt()*3));
            dataContainer::setWarning(curMessage->getID(),"mesajı için timeout belirtilmediği ancak cycletime belirtildiği için cycletime*3 yapıldı");
        }
    }

}

void DBCHandler::checkRepatedSignal()
{
    interface::Iterator iteInterface1;
    interface::Iterator iteInterface2;

    for(iteInterface1=comInterface.begin();iteInterface1!=comInterface.end();iteInterface1++){
        for(dataContainer::signal * curSignal1 : *iteInterface1.value()->getSignalList()){
            for(iteInterface2=comInterface.begin();iteInterface2!=comInterface.end();iteInterface2++){
                for(dataContainer::signal * curSignal2 : *iteInterface2.value()->getSignalList()){
                    if((curSignal1->name.contains(curSignal2->name,Qt::CaseInsensitive)) && (iteInterface2.value()->getID() != iteInterface1.value()->getID())){
                        dataContainer::setWarning(iteInterface2.value()->getID(),curSignal2->name+" sinyali "+iteInterface1.value()->getID()+" mesajında da yer aldığı için etiketlendi.");
                        curSignal2->name= curSignal2->name+"_"+iteInterface2.value()->getID();
                    }
                }
            }
        }
    }
}


qreal DBCHandler::progress() const
{
    return m_progress;
}

void DBCHandler::setProgress(qreal newProgress)
{
    if (m_progress == newProgress)
        return;
    m_progress = newProgress;
    if(m_progress == 100)
        emit progressCompleted();
    else
        emit progressChanged();


}

///******************************************************************************
/// XML GENERATION
/// -Open XML
/// -Generate general attributes
/// -Generate DUTs
/// -Check II or IO
/// -Generate POUS
/// -Generate handlers if IO type
///******************************************************************************
void DBCHandler::startToGenerate()
{
    setProgress(0);
    qint64 startTimeMs = QDateTime::currentMSecsSinceEpoch();
    if(!DBCHandler::selectedMessageCounter){
        this->setErrCode("En az bir mesaj seçmelisin");
    }else if(!isAllInserted){
        this->setErrCode("Veri tabanı okunamadı");
    }else if(!(dutHeader.contains("II") || dutHeader.contains("IO") || dutHeader.contains("io")|| dutHeader.contains("ii"))){
        this->setErrCode("DUT adı II veya IO etiketini içermeli");
    }else{
        emit this->procesStarted();
        try {
            if (this->folderLoc.isEmpty()){
                throw QString("Dosya konumu boş olamaz!");
            }
            else{
                QFile *xmlFile = new QFile(QString(this->folderLoc+"/"+(this->dutHeader)+"_Chain.xml"));
                if(!xmlFile->open(QIODevice::WriteOnly | QIODevice::Truncate)){
                    throw QString("Dosya açılamadı, lütfen konumu kontrol edin!");
                }
                else{
                    emit progressStarted();
                    creationDate = QDateTime::currentDateTime();
                    if (!createXml_STG1(xmlFile)){
                        throw QString("XML oluşturulurken bir şeyler yanlış gitti!");
                    }else
                        xmlFile->close();
                    setProgress(100);
                    QString infoText;
                    for(dataContainer *const curValue : comInterface){
                        if(curValue->getIfSelected()){
                            infoText.append(curValue->getID()+"|");
                        }
                    }
                    dataContainer::setWarning("INFO",this->dutHeader+" mesajları : "+infoText);
                    dataContainer::setWarning("INFO",this->dutHeader+" oluşturuldu. Oluşturma süresi:"+QString::number((QDateTime::currentMSecsSinceEpoch())-startTimeMs)+"ms");
                    emit infoListChanged();
                }

            }
        } catch (QString text) {
            this->setErrCode(text);
        }
    }
    fbNameandObjId.clear();
    //do here
}
QList<QString> DBCHandler::getWarningList()
{
    return dataContainer::getWarningList();
}
QList<QString> DBCHandler::getMsgWarningList()
{
    return dataContainer::getMsgWarningList(displayReqSignalID);
}
QList<QString> DBCHandler::getInfoList()
{
    return dataContainer::getInfoList();
}
bool DBCHandler::getAllSelected()
{
    return DBCHandler::allSelected;
}
bool DBCHandler::createXml_STG1(QFile *xmlFile)
{
    QTextStream out(xmlFile);
    QDomDocument doc;
    QDomText text;
    QDomElement element;
    QDomAttr attr;
    //Generate obj ids
    this->createObjIds();
    //<?xml version="1.0" encoding="utf-8"?>
    QDomProcessingInstruction instruction;
    instruction = doc.createProcessingInstruction( "xml", "version = \'1.0\' encoding=\'utf-8\'" );
    doc.appendChild( instruction );
    //****************************************************
    setProgress(10);
    //<project xmlns="http://www.plcopen.org/xml/tc6_0200">
    QDomElement elemProject = doc.createElement( "project" );
    attr = doc.createAttribute( "xmlns" );//Add the text NAME in the USERINFO element
    attr.setValue("http://www.plcopen.org/xml/tc6_0200");//Add value for text NAME
    elemProject.setAttributeNode(attr);
    //Append child to document
    doc.appendChild(elemProject);
    //****************************************************
    //START OF "fileHeader"
    //<fileHeader companyName="" productName="" productVersion="" creationDateTime="" />
    {
        QDomElement fileHeader = doc.createElement( "fileHeader" );
        attr =doc.createAttribute("companyName");
        attr.setValue("BZK");
        fileHeader.setAttributeNode(attr);
        attr =doc.createAttribute("productName");
        attr.setValue("CODESYS");
        fileHeader.setAttributeNode(attr);
        attr =doc.createAttribute("productVersion");
        attr.setValue("CODESYS V3.5 SP17");
        fileHeader.setAttributeNode(attr);
        attr =doc.createAttribute("creationDateTime");
        attr.setValue(creationDate.toString(Qt::DateFormat::ISODate));
        fileHeader.setAttributeNode(attr);
        //Append child to project
        elemProject.appendChild(fileHeader);
    }
    //****************************************************
    //END OF "fileHeader"
    //START OF "ContentHeader"
    //<contentHeader name="" modificationDateTime="">
    {
        QDomElement contentHeader = doc.createElement( "contentHeader" );
        attr =doc.createAttribute("name");
        attr.setValue("AUTOMATIC_INTERFACE_GENERATOR.project");
        contentHeader.setAttributeNode(attr);
        attr =doc.createAttribute("modificationDateTime");
        attr.setValue("2023-01-01T01:01:01.0000001");
        contentHeader.setAttributeNode(attr);
        //Append child to project
        elemProject.appendChild(contentHeader);
        //****************************************************
        //<coordinateInfo>
        QDomElement coordinateInfo = doc.createElement("coordinateInfo");
        contentHeader.appendChild(coordinateInfo);
        QDomElement scaling1 = doc.createElement("scaling"); // Make Child4 "scaling"
        attr =doc.createAttribute("y");
        attr.setValue("1");
        scaling1.setAttributeNode(attr);         //<scaling x="1"/>
        attr =doc.createAttribute("x");
        attr.setValue("1");
        scaling1.setAttributeNode(attr);         //<scaling Y="1"/>

        QDomElement fbd = doc.createElement("fbd");
        fbd.appendChild(scaling1);       //Append "scaling" to "fbd"
        coordinateInfo.appendChild(fbd);       //Append "fpd" to "coordinateInfo"
        scaling1 = doc.createElement("scaling"); // Make Child4 "scaling"
        attr =doc.createAttribute("y");
        attr.setValue("1");
        scaling1.setAttributeNode(attr);         //<scaling x="1"/>
        attr =doc.createAttribute("x");
        attr.setValue("1");
        scaling1.setAttributeNode(attr);
        QDomElement ld = doc.createElement("ld");      //Make Child3 "ld"
        ld.appendChild(scaling1);       //Append "scaling" to "ld"
        coordinateInfo.appendChild(ld);       //Append "ld" to "coordinateInfo"
        scaling1 = doc.createElement("scaling"); // Make Child4 "scaling"
        attr =doc.createAttribute("y");
        attr.setValue("1");
        scaling1.setAttributeNode(attr);         //<scaling x="1"/>
        attr =doc.createAttribute("x");
        attr.setValue("1");
        scaling1.setAttributeNode(attr);
        QDomElement sfc = doc.createElement("sfc");     //Make Child3 "sfc"
        sfc.appendChild(scaling1);       //Append "scaling" to "sfc"
        coordinateInfo.appendChild(sfc);       //Append "sfc" to "coordinateInfo"

        {
            //****************************************************
            //<addData>
            QDomElement addData = doc.createElement("addData");
            contentHeader.appendChild(addData);
            QDomElement data = doc.createElement("data");
            attr = doc.createAttribute("name");
            attr.setValue("http://www.3s-software.com/plcopenxml/projectinformation");
            data.setAttributeNode(attr);
            attr = doc.createAttribute("handleUnknown");
            attr.setValue("implementation");
            data.setAttributeNode(attr);
            addData.appendChild(data);
            QDomElement ProjectInformation = doc.createElement("ProjectInformation");
            data.appendChild(ProjectInformation);
        }
        //END OF "ContentHeader"
    }
    setProgress(20);
    {
        //START OF "types"
        QDomElement types = doc.createElement("types");
        QDomElement pous = doc.createElement("pous");
        QDomElement dataTypes = doc.createElement("dataTypes");
        QDomElement dataType = doc.createElement("dataType");
        QDomElement baseType = doc.createElement("baseType");
        QDomElement strucT = doc.createElement("struct");
        QDomElement addData = doc.createElement("addData");
        QDomElement data = doc.createElement("data");
        QDomElement ObjectId = doc.createElement("ObjectId");
        attr = doc.createAttribute("name");
        attr.setValue(this->dutName);
        dataType.setAttributeNode(attr);
        this->generateVariables(&strucT,doc);
        baseType.appendChild(strucT);
        dataType.appendChild(baseType);
        attr = doc.createAttribute("name");
        attr.setValue("http://www.3s-software.com/plcopenxml/objectid");
        data.setAttributeNode(attr);
        attr = doc.createAttribute("handleUnknown");
        attr.setValue("discard");
        data.setAttributeNode(attr);
        text=doc.createTextNode(dutObjID);
        ObjectId.appendChild(text);
        data.appendChild(ObjectId);
        addData.appendChild(data);
        dataType.appendChild(addData);
        dataTypes.appendChild(dataType);
        types.appendChild(dataTypes);
        setProgress(30);
        if((this->IOType == "II")){
            setProgress(40);
            this->generateIIPous(&pous,doc);
            setProgress(50);
        }
        else{
            setProgress(40);
            this->generateIOPous(&pous,doc);
            setProgress(50);
        }
        types.appendChild(pous);
        elemProject.appendChild(types);

        //END OF "types"
    }
    {
        //START OF "instances"
        QDomElement instances = doc.createElement("instances");
        elemProject.appendChild(instances);
        QDomElement configurations= doc.createElement("configurations");
        instances.appendChild(configurations);
        //END OF "instances"
    }
    setProgress(60);
    {
        //START OF "addData"
        QDomElement addData = doc.createElement("addData");
        elemProject.appendChild(addData);
        QDomElement data= doc.createElement("data");
        attr = doc.createAttribute("name");
        attr.setValue("http://www.3s-software.com/plcopenxml/projectstructure");
        data.setAttributeNode(attr);
        attr = doc.createAttribute("handleUnknown");
        attr.setValue("discard");
        data.setAttributeNode(attr);
        addData.appendChild(data);
        //fbs
        QDomElement ProjectStructure = doc.createElement("ProjectStructure");
        data.appendChild(ProjectStructure);
        QDomElement folder1;
        QDomElement folder2;

        setProgress(70);
        //DUTs
        ProjectStructure.appendChild(folder1);
        folder1 = doc.createElement("Folder");
        attr= doc.createAttribute("Name");
        attr.setValue("FD_COMDUT"); // Folder name for Function blocks
        folder1.setAttributeNode(attr);
        folder2 = doc.createElement("Folder");
        attr = doc.createAttribute("Name");
        attr.setValue(this->dutName);
        folder2.setAttributeNode(attr);
        attr = doc.createAttribute("ObjectId");
        attr.setValue(this->dutObjID);
        folder2.setAttributeNode(attr);
        folder1.appendChild(folder2);
        ProjectStructure.appendChild(folder1);
         setProgress(80);
        //Pous
        folder1 = doc.createElement("Folder");
        attr= doc.createAttribute("Name");
        attr.setValue("FD_POUs"); // Folder name for Function blocks
        folder1.setAttributeNode(attr);
        folder2 = doc.createElement("Folder");
        attr = doc.createAttribute("Name");
        attr.setValue("P_"+this->dutHeader);
        folder2.setAttributeNode(attr);
        attr = doc.createAttribute("ObjectId");
        attr.setValue(this->pouObjID);
        folder2.setAttributeNode(attr);
        folder1.appendChild(folder2);
        ProjectStructure.appendChild(folder1);
        setProgress(90);
        //END OF "addData"
    }

    doc.save(out, 4);
    return true;
}
void DBCHandler::generateVariables(QDomElement * strucT, QDomDocument &doc)
{
    QDomAttr attr;
    QDomText text;
    bool flagNewMessage=false;
    for(dataContainer *const curValue : comInterface){
        if(curValue->getIfSelected()){
            flagNewMessage=true;
            for(const dataContainer::signal * curSignal : *curValue->getSignalList()){
                QDomElement variable = doc.createElement("variable");
                attr = doc.createAttribute("name");
                attr.setValue(curSignal->name);
                variable.setAttributeNode(attr);
                { //type and derived element with name attribute
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue(curSignal->appDataType+"_T");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                }
                {//initialValue
                    QDomElement initialValue = doc.createElement("initialValue");
                    QDomElement structValue = doc.createElement("structValue");
                    /*
                     * This part deleted after v1.000.037
                     *
                     */
                    //if((curSignal->appDataType)!="BOOL"){
                    //    QDomElement value = doc.createElement("value");
                    //    attr = doc.createAttribute("member");
                    //    attr.setValue("Param_Max");
                    //    value.setAttributeNode(attr);
                    //    QDomElement simpleValue = doc.createElement("simpleValue");
                    //    attr = doc.createAttribute("value");
                    //    attr.setValue(QString::number(curSignal->maxValue,'g',(curSignal->length>32)? 20:15));
                    //    simpleValue.setAttributeNode(attr);
                    //    value.appendChild(simpleValue);
                    //    structValue.appendChild(value);
                    //}
                    //if((curSignal->appDataType)!="BOOL"){
                    //    QDomElement value = doc.createElement("value");
                    //    attr = doc.createAttribute("member");
                    //    attr.setValue("Param_Min");
                    //    value.setAttributeNode(attr);
                    //    QDomElement simpleValue = doc.createElement("simpleValue");
                    //    attr = doc.createAttribute("value");
                    //    attr.setValue(QString::number(curSignal->minValue,'g',(curSignal->length>32)? 20:15));
                    //    simpleValue.setAttributeNode(attr);
                    //    value.appendChild(simpleValue);
                    //    structValue.appendChild(value);
                    //}
                    //if(((curSignal->appDataType)=="REAL")||((curSignal->appDataType)=="LREAL")){
                    //    QDomElement value = doc.createElement("value");
                    //    attr = doc.createAttribute("member");
                    //    attr.setValue("Param_Res");
                    //    value.setAttributeNode(attr);
                    //    QDomElement simpleValue = doc.createElement("simpleValue");
                    //    attr = doc.createAttribute("value");
                    //    attr.setValue(QString::number(curSignal->resolution));
                    //    simpleValue.setAttributeNode(attr);
                    //    value.appendChild(simpleValue);
                    //    structValue.appendChild(value);
                    //}
                    //if(((curSignal->appDataType)=="REAL")||((curSignal->appDataType)=="LREAL")){
                    //    QDomElement value = doc.createElement("value");
                    //    attr = doc.createAttribute("member");
                    //    attr.setValue("Param_Off");
                    //    value.setAttributeNode(attr);
                    //    QDomElement simpleValue = doc.createElement("simpleValue");
                    //    attr = doc.createAttribute("value");
                    //    attr.setValue(QString::number(curSignal->offset));
                    //    simpleValue.setAttributeNode(attr);
                    //    value.appendChild(simpleValue);
                    //    structValue.appendChild(value);
                    //}
                    //if((curSignal->appDataType)!="BOOL"){
                    //    QDomElement value = doc.createElement("value");
                    //    attr = doc.createAttribute("member");
                    //    attr.setValue("Param_Def");
                    //    value.setAttributeNode(attr);
                    //    QDomElement simpleValue = doc.createElement("simpleValue");
                    //    attr = doc.createAttribute("value");
                    //    attr.setValue(QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15));
                    //    simpleValue.setAttributeNode(attr);
                    //    value.appendChild(simpleValue);
                    //    structValue.appendChild(value);
                    //}
                    /*{
                        QDomElement value = doc.createElement("value");
                        attr = doc.createAttribute("member");
                        attr.setValue("x");
                        value.setAttributeNode(attr);
                        QDomElement simpleValue = doc.createElement("simpleValue");
                        attr = doc.createAttribute("value");
                        attr.setValue(QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15));
                        simpleValue.setAttributeNode(attr);
                        value.appendChild(simpleValue);
                        structValue.appendChild(value);
                    }
                    if((curSignal->appDataType)!="BOOL"){
                        QDomElement value = doc.createElement("value");
                        attr = doc.createAttribute("member");
                        attr.setValue("J1939");
                        value.setAttributeNode(attr);
                        QDomElement simpleValue = doc.createElement("simpleValue");
                        attr = doc.createAttribute("value");
                        attr.setValue(QString::number(curSignal->isJ1939));
                        simpleValue.setAttributeNode(attr);
                        value.appendChild(simpleValue);
                        structValue.appendChild(value);
                    }
                    initialValue.appendChild(structValue);
                    variable.appendChild(initialValue);*/
                }
                QDomElement documentation=doc.createElement("documentation");
                QDomElement xhtml = doc.createElement("xhtml");
                attr=doc.createAttribute("xmlns");
                attr.setValue("http://www.w3.org/1999/xhtml");
                xhtml.setAttributeNode(attr);
                {
                    QString comment;
                if (flagNewMessage){
                    comment.append("\n\n****************************************** MESSAGE: "+curValue->getName()+"  ID: "+curValue->getID()+" ******************************************\n");
                    flagNewMessage=false;
                }
                   comment.append(curSignal->name+"/StartBit:"+QString::number(curSignal->startBit)+"/Length:"+QString::number(curSignal->length)+"/J1939:"+((curSignal->isJ1939)?"YES":"NO"));
                text =doc.createTextNode(comment);
                xhtml.appendChild(text);
                }
                documentation.appendChild(xhtml);
                variable.appendChild(documentation);
                strucT->appendChild(variable);
            }


        }
    }
    if(dutName.contains("II")||dutName.contains("ii")){
        bool flagBlankSpace=true;
        foreach(dataContainer *const curValue , comInterface){
            if(curValue->getIfSelected()){

                QDomElement variable = doc.createElement("variable");
                attr = doc.createAttribute("name");
                attr.setValue("S_TmOut_"+curValue->getName()+"_0X"+curValue->getID());
                variable.setAttributeNode(attr);
                { //type and derived element with name attribute
                    QDomElement type = doc.createElement("type");
                    QDomElement BOOL = doc.createElement("BOOL");
                    type.appendChild(BOOL);
                    variable.appendChild(type);
                }
                {//Documentation

                    QDomElement documentation=doc.createElement("documentation");
                    QDomElement xhtml = doc.createElement("xhtml");
                    attr=doc.createAttribute("xmlns");
                    attr.setValue("http://www.w3.org/1999/xhtml");
                    xhtml.setAttributeNode(attr);
                    {
                        QString comment;
                        if(flagBlankSpace){
                            comment.append("\n\nTIMEOUT SIGNALS\n");
                            flagBlankSpace=false;
                        }

                        comment.append("Signal timeout status. Timeout parameter for signal :"+curValue->getMsTimeOut()+"ms");
                        text =doc.createTextNode(comment);
                        xhtml.appendChild(text);
                    }
                    documentation.appendChild(xhtml);
                    variable.appendChild(documentation);
                }
                strucT->appendChild(variable);
            }
        }
        {
            QDomElement variable = doc.createElement("variable");
            attr = doc.createAttribute("name");
            attr.setValue("S_Com_Flt");
            variable.setAttributeNode(attr);
            { //type and derived element with name attribute
                QDomElement type = doc.createElement("type");
                QDomElement BOOL = doc.createElement("BOOL");
                type.appendChild(BOOL);
                variable.appendChild(type);
            }
            {//Documentation

                QDomElement documentation=doc.createElement("documentation");
                QDomElement xhtml = doc.createElement("xhtml");
                attr=doc.createAttribute("xmlns");
                attr.setValue("http://www.w3.org/1999/xhtml");
                xhtml.setAttributeNode(attr);
                {
                    QString comment;
                    comment.append("All of the timeout states are TRUE, no communication with unit");
                    text =doc.createTextNode(comment);
                    xhtml.appendChild(text);
                }
                documentation.appendChild(xhtml);
                variable.appendChild(documentation);
            }
            strucT->appendChild(variable);
        }
        {
            QDomElement variable = doc.createElement("variable");
            attr = doc.createAttribute("name");
            attr.setValue("S_Com_Distrb");
            variable.setAttributeNode(attr);
            { //type and derived element with name attribute
                QDomElement type = doc.createElement("type");
                QDomElement BOOL = doc.createElement("BOOL");
                type.appendChild(BOOL);
                variable.appendChild(type);
            }
            {//Documentation

                QDomElement documentation=doc.createElement("documentation");
                QDomElement xhtml = doc.createElement("xhtml");
                attr=doc.createAttribute("xmlns");
                attr.setValue("http://www.w3.org/1999/xhtml");
                xhtml.setAttributeNode(attr);
                {
                    QString comment;
                    comment.append("One of the messages is not receiving from that unit, communication disturbed with unit");
                    text =doc.createTextNode(comment);
                    xhtml.appendChild(text);
                }
                documentation.appendChild(xhtml);
                variable.appendChild(documentation);
            }
            strucT->appendChild(variable);
        }

    }else if (dutName.contains("IO")||dutName.contains("io")){
        bool flagBlankSpace=true;
        foreach(dataContainer *const curValue , comInterface){
            if(curValue->getIfSelected()){

                QDomElement variable = doc.createElement("variable");
                attr = doc.createAttribute("name");
                attr.setValue("S_SentOk_"+curValue->getName()+"_0X"+curValue->getID());
                variable.setAttributeNode(attr);
                { //type and derived element with name attribute
                    QDomElement type = doc.createElement("type");
                    QDomElement BOOL = doc.createElement("BOOL");
                    type.appendChild(BOOL);
                    variable.appendChild(type);
                }
                {//Documentation

                    QDomElement documentation=doc.createElement("documentation");
                    QDomElement xhtml = doc.createElement("xhtml");
                    attr=doc.createAttribute("xmlns");
                    attr.setValue("http://www.w3.org/1999/xhtml");
                    xhtml.setAttributeNode(attr);
                    {
                        QString comment;
                        if(flagBlankSpace){
                            comment.append("\n\nMessage sent SIGNALS\n");
                            flagBlankSpace=false;
                        }

                        comment.append("Message sent status provided by LIB500 ");
                        text =doc.createTextNode(comment);
                        xhtml.appendChild(text);
                    }
                    documentation.appendChild(xhtml);
                    variable.appendChild(documentation);
                }
                strucT->appendChild(variable);
            }
        }
    }
    // CAN_Init register for DUT
    {
        QDomElement variable = doc.createElement("variable");
        attr = doc.createAttribute("name");
        attr.setValue("S_CAN_Init");
        variable.setAttributeNode(attr);
        { //type and derived element with name attribute
            QDomElement type = doc.createElement("type");
            QDomElement BOOL = doc.createElement("BOOL");
            type.appendChild(BOOL);
            variable.appendChild(type);
        }
        {//Documentation

            QDomElement documentation=doc.createElement("documentation");
            QDomElement xhtml = doc.createElement("xhtml");
            attr=doc.createAttribute("xmlns");
            attr.setValue("http://www.w3.org/1999/xhtml");
            xhtml.setAttributeNode(attr);
            {
                QString comment;
                comment.append("CAN Initilizer variable");
                text =doc.createTextNode(comment);
                xhtml.appendChild(text);
            }
            documentation.appendChild(xhtml);
            variable.appendChild(documentation);
        }
        strucT->appendChild(variable);
    }
}
///******************************************************************************
/// RECEIVE MESSAGES FUNCTION BLOCK ST GENERATOR
///******************************************************************************
void DBCHandler::generateIIPous(QDomElement * pous, QDomDocument &doc)
{
    QDomAttr attr;
    QDomText text;

    QString namePou = "P_"+this->dutHeader;
     //For AND and OR gate to manipulate timeout and disturbance flags

            /*generate block struct type new block*/

            structFbdBlock* newBlock = new structFbdBlock;
            QDomElement pou = doc.createElement("pou");
            /*set pou name*/
            attr=doc.createAttribute("name");
            attr.setValue(namePou);
            pou.setAttributeNode(attr);
            /*set pouType*/
            attr=doc.createAttribute("pouType");
            attr.setValue("program");
            pou.setAttributeNode(attr);

            /*Interface*/
            QDomElement interface = doc.createElement("interface");

            /*Generate Local Variables - localVars*/
            QString STcode;
            this->generateIIST(&STcode);
//Generate local vars
            {
                QDomElement localVars= doc.createElement("localVars");
                /*Function block declaration*/
                foreach (dataContainer * curMessage , comInterface){
                    if(curMessage->getIfSelected()){
                        QDomElement variable=doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue(curMessage->getIfBitOperation()?("_FB_CanRx_Message_Unpack_"+curMessage->getID()):("_FB_CanRx_Message_"+curMessage->getID()));
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement derived = doc.createElement("derived");
                        attr=doc.createAttribute("name");
                        attr.setValue(curMessage->getIfBitOperation()?("_FB_CanRx_Message_Unpack"):("_FB_CanRx_Message"));
                        derived.setAttributeNode(attr);
                        type.appendChild(derived);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                }

                foreach (dataContainer * curMessage , comInterface){
                    if(curMessage->getIfSelected()){
                        for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                            QDomElement variable=doc.createElement("variable");
                            attr=doc.createAttribute("name");
                            attr.setValue("Raw_"+curSignal->name);
                            variable.setAttributeNode(attr);
                            QDomElement type = doc.createElement("type");
                            QDomElement dataType = doc.createElement(curSignal->comDataType);
                            type.appendChild(dataType);
                            variable.appendChild(type);
                            localVars.appendChild(variable);
                            variable=doc.createElement("variable");
                            attr=doc.createAttribute("name");
                            attr.setValue("Cont_"+curSignal->name);
                            variable.setAttributeNode(attr);
                            type = doc.createElement("type");
                            dataType = doc.createElement(curSignal->appDataType);
                            type.appendChild(dataType);
                            variable.appendChild(type);
                            localVars.appendChild(variable);
                        }
                    }
                }

                if(this->enableFrc){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("FrcVar");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue(this->dutName);
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }

                for (unsigned i =0; i<counterfbBYTETOWORD; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_PACK_BYTE_TO_WORD_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_PACK_BYTE_TO_WORD");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }
                for (unsigned i =0; i<counterfbBYTETODWORD; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_PACK_BYTE_TO_DWORD_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_PACK_BYTE_TO_DWORD");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);

                }
                for (unsigned i =0; i<counterfbBYTETOLWORD; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_PACK_BYTE_TO_LWORD_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_PACK_BYTE_TO_LWORD");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }
                for (unsigned i =0; i<counterfb8BITTOBYTE; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_PACK_8BITS_TO_BYTE_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_PACK_8BITS_TO_BYTE");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }
                for (unsigned i =0; i<counterfbDWORDTOLWORD; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_PACK_DWORD_TO_LWORD_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_PACK_DWORD_TO_LWORD");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }

                /*Function Block declaratoins*/
                DBCHandler::counterfbBYTETOWORD=0;
                DBCHandler::counterfbBYTETODWORD=0;
                DBCHandler::counterfbBYTETOLWORD=0;
                DBCHandler::counterfb8BITTOBYTE=0;
                DBCHandler::counterfbDWORDTOLWORD=0;


                interface.appendChild(localVars);
            }
            pou.appendChild(interface);

            /*Create Body*/
            QDomElement body = doc.createElement("body");
            QDomElement ST = doc.createElement("ST");
            QDomElement xhtml = doc.createElement("xhtml");
            attr=doc.createAttribute("xmlns");
            attr.setValue("http://www.w3.org/1999/xhtml");
            xhtml.setAttributeNode(attr);
            text=doc.createTextNode(STcode);
            xhtml.appendChild(text);
            ST.appendChild(xhtml);
            body.appendChild(ST);
            pou.appendChild(body);

            /*Create addData*/
            QDomElement addData = doc.createElement("addData");
            QDomElement data = doc.createElement("data");
            attr=doc.createAttribute("name");
            attr.setValue("http://www.3s-software.com/plcopenxml/objectid");
            data.setAttributeNode(attr);
            attr=doc.createAttribute("handleUnknown");
            attr.setValue("discard");
            data.setAttributeNode(attr);
            QDomElement ObjectId = doc.createElement("ObjectId");
            text=doc.createTextNode(this->pouObjID);
            ObjectId.appendChild(text);
            data.appendChild(ObjectId);
            addData.appendChild(data);
            pou.appendChild(addData);
            pous->appendChild(pou);


           fbdBlocks.append(newBlock);
           // append input variables to AND gate to check disturbance -> EXP format : GVL.IIXC.S_TmOut_Motor_Messages_3_0X512


}
void DBCHandler::generateIIST(QString *const ST)
{
    ST->append("(*\n"
               "**********************************************************************\n"
               "Bozankaya A.Ş.\n"
               "**********************************************************************\n"
               "Name					: P_"+dutName+"\n"
               "POU Type				: Program\n"
               "Created by              : COMMUNICATION INTERFACE GENERATOR("+QHostInfo::localHostName()+") , BZK.\n"
               "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
               "Modifications			: see version description below\n"
               "\n"
               "\n"
               "Program Description:"
               "This program which is created by automatically by communication interface generator \n handles the RX (INPUT) can messages according to CAN_500_LIB\n"
               "*********************************************************************\n"
               "\n"
               "Version 1	\n"
               "*********************************************************************\n"
               "Version Description:\n"
               "- initial version\n"
               "*********************************************************************\n"
               "*)");
    foreach (dataContainer * curMessage , comInterface){
        if(curMessage->getIfSelected()){
            ST->append("\n{region \""+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
            QString nameFb = (curMessage->getIfBitOperation())? ("_FB_CanRx_Message_Unpack_"+curMessage->getID()) : ("_FB_CanRx_Message_"+curMessage->getID());
            ST->append( "\n"+nameFb+"(\n"
                        "   C_Enable:= GVL."+dutHeader+".S_CAN_Init,\n"
                        "   Obj_CAN:= ADR("+this->canLine+"),\n"
                        "   X_MsgID:= 16#"+curMessage->getID()+",\n"
                        "   Tm_MsgTmOut:= TIME#"+curMessage->getMsTimeOut()+"ms,\n"
                        "   S_Extended:= "+QString::number(curMessage->getIfExtended()).toUpper()+",\n"
                        "   S_ER_TmOut=> GVL."+dutHeader+".S_TmOut_"+curMessage->getName()+"_0X"+curMessage->getID()+");\n");

            for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                ST->append(convTypeComtoApp(curSignal,curMessage->getID(),curMessage->getName(),nameFb));
            }
            ST->append("\n{endregion}");

        }
    }
    //Place communication fault flag that belongs to interface
    ST->append("\n GVL."+dutHeader+".S_Com_Flt:=");
    foreach (dataContainer * curMessage , comInterface){
        if(curMessage->getIfSelected()){
            for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                ST->append("GVL."+dutHeader+".S_TmOut_"+curMessage->getName()+"_0X"+curMessage->getID()+" AND ");
            }
        }
    }
    ST->append(" TRUE;");
    //Place communication disturbance flag that belongs to interface
    ST->append("\n GVL."+dutHeader+".S_Com_Distrb:=");
    foreach (dataContainer * curMessage , comInterface){
        if(curMessage->getIfSelected()){
            for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                ST->append("GVL."+dutHeader+".S_TmOut_"+curMessage->getName()+"_0X"+curMessage->getID()+" OR ");
            }
        }
    }
    ST->append(" FALSE;");
}
QString DBCHandler::convTypeComtoApp(const dataContainer::signal * curSignal, QString idMessage, QString nameMessage, QString nameFb)
{
    QString ST="\n{region \""+curSignal->name+"\"}";
    if(curSignal->convMethod=="BOOL:BOOL"){
        if(this->enableFrc){
        ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".v               := NOT GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+" OR FrcVar."+curSignal->name+".v ;"
                  "\nIF NOT FrcVar."+curSignal->name+".v THEN;"
                  "\nGVL."+this->dutHeader+"."+curSignal->name+".x               := GVL."+this->dutHeader+"."+curSignal->name+".v AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+";"
                  "\nELSE "
                  "\nGVL."+this->dutHeader+"."+curSignal->name+".x               := FrcVar."+curSignal->name+".x ;"
                  "\nEND_IF;");
        }else{
            ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".v               := NOT GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+";"
                      "\nGVL."+this->dutHeader+"."+curSignal->name+".x               := GVL."+this->dutHeader+"."+curSignal->name+".v AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+";");
        }
    }else if(curSignal->convMethod=="2BOOL:BOOL"){
        if(this->enableFrc){
        ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".v               := (NOT GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+" AND NOT "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+") OR FrcVar."+curSignal->name+".v ;"
                   "\nIF NOT FrcVar."+curSignal->name+".v THEN;"
                   "\nGVL."+this->dutHeader+"."+curSignal->name+".x              := GVL."+this->dutHeader+"."+curSignal->name+".v AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+";"
                   "\nELSE "
                   "\nGVL."+this->dutHeader+"."+curSignal->name+".x              := FrcVar."+curSignal->name+".x ;"
                   "\nEND_IF;");
        }else{
            ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".v               := NOT GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+" AND NOT "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+";"
                       "\nGVL."+this->dutHeader+"."+curSignal->name+".x              := GVL."+this->dutHeader+"."+curSignal->name+".v AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+";");
        }
    }else if((curSignal->convMethod=="xtoBYTE")||(curSignal->convMethod=="xtoUSINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==8))){
        ST.append("\nRaw_"+curSignal->name+"             := "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+";");
    }else if((curSignal->convMethod=="xtoWORD")||(curSignal->convMethod=="xtoUINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==16))){
        ST.append("\n_FB_PACK_BYTE_TO_WORD_"+QString::number(counterfbBYTETOWORD)+"(X_BYTE_0:= "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", X_BYTE_1:=  "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+", X_WORD_0 => Raw_"+curSignal->name+");");
        counterfbBYTETOWORD++;
    }else if((curSignal->convMethod=="xtoDWORD")||(curSignal->convMethod=="xtoUDINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==32))){
        ST.append("\n_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+"(X_BYTE_0:= "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", X_BYTE_1:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+", X_BYTE_2:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+2)+", X_BYTE_3:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+3)+" , X_DWORD_0 => Raw_"+curSignal->name+");");
        counterfbBYTETODWORD++;
    }else if((curSignal->convMethod=="xtoLWORD")||(curSignal->convMethod=="xtoULINT")||(curSignal->convMethod=="xtoLREAL") ){
        ST.append("\n_FB_PACK_BYTE_TO_LWORD_"+QString::number(counterfbBYTETOLWORD)+"(X_BYTE_0:= "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", X_BYTE_1:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+", X_BYTE_2:="+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+2)+" , X_BYTE_3:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+3)+", X_BYTE_4:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+4)+", X_BYTE_5:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+5)+", X_BYTE_6:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+6)+", X_BYTE_7:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+7)+" , X_LWORD_0 => Raw_"+curSignal->name+");");
        counterfbBYTETOLWORD++;
    }else{
        bool flagPack = false;
        if((curSignal->length>8)){
            if(curSignal->length<17){
                ST.append("\n_FB_PACK_BYTE_TO_WORD_"+QString::number(counterfbBYTETOWORD)+"();");

            }else if(curSignal->length <33){
                ST.append("\n_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+"();");

            }else {
                ST.append("\n_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+"();");
                ST.append("\n_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD+1)+"();\n");

            }
        }
        unsigned packID=0;
        unsigned packByteID=0;
        unsigned counterBITBYTE=0;
        unsigned counterBYTEWORD=0;
        unsigned counterBYTEDWORD=0;
        for(unsigned i =0; i<curSignal->length;i++){

            if((i%32 == 0) && (i>0) && curSignal->length>16){
                packID++;
                packByteID=0;
            }
            if((i%16 == 0) && (i>0) && curSignal->length<=16){
                packID++;
                packByteID=0;
            }

            if(((i%8 == 0) && ((i/8)<8) )&& (!flagPack)){
                ST.append("\n_FB_PACK_8BITS_TO_BYTE_"+QString::number(counterfb8BITTOBYTE+qFloor(i/8.0))+"(");
                counterBITBYTE++;
                flagPack=true;
            }
            if(flagPack){
                    ST.append("S_BIT_"+QString::number(i%8)+":= "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+i)+"");
                    if(((i%8 != 7) &&(i%(curSignal->length-1) != 0)) || (i==0)) {
                        ST.append(",");
                    }
            }
            if(flagPack){
                if(((i%8 == 7)||(i%(curSignal->length-1) == 0) )&& (i>0)){
                    ST.append(" ,X_BYTE_0=>");
                    if(curSignal->length<9){
                    ST.append("Raw_"+curSignal->name);
                    ST.append(");");
                    }else if (curSignal->length <17){
                    ST.append("_FB_PACK_BYTE_TO_WORD_"+QString::number(counterfbBYTETOWORD+packID));
                    ST.append(".X_BYTE_"+QString::number(packByteID)+");");
                    }else{
                    ST.append("_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD+packID)+"");
                    ST.append(".X_BYTE_"+QString::number(packByteID)+");");
                    }

                    flagPack=false;
                    packByteID++;
                }
            }

        }

        if((curSignal->length>8)){
            if(curSignal->length<17){
                ST.append("\nRaw_"+curSignal->name+"            :=_FB_PACK_BYTE_TO_WORD_"+QString::number(counterfbBYTETOWORD)+".X_WORD_0;");
                counterfbBYTETOWORD++;
            }else if(curSignal->length <33){
                ST.append("\nRaw_"+curSignal->name+"            :=_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+".X_DWORD_0;");
                counterfbBYTETODWORD++;
            }else {
                ST.append("\n_FB_PACK_DWORD_TO_LWORD_"+QString::number(counterfbDWORDTOLWORD)+"(X_DWORD_0:=_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+".X_DWORD_0,X_DWORD_1:=_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD+1)+".X_DWORD_0,X_LWORD_0=> Raw_"+curSignal->name+");");
            counterfbDWORDTOLWORD++;
            counterfbBYTETODWORD++;
            counterfbBYTETODWORD++;
            }
        }
        counterfb8BITTOBYTE = counterfb8BITTOBYTE+ counterBITBYTE;
    }
    // TYPE CONVERTION AND E  NA CONTROL STARTS
    if(curSignal->length==1){
        //do nothing
    }
    else if((curSignal->length==2) && (curSignal->convMethod != "toBYTE")){

        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e				    := "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+" AND NOT "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+" ;") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na				:= "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+" AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+" ;") : (""));

    }
    else if((curSignal->length<9)){
        if((curSignal->convMethod == "toBYTE")|| (curSignal->convMethod == "xtoBYTE")){
            ST.append("\nCont_"+curSignal->name+"				:= Raw_"+curSignal->name+" ;");
        }
        else if ((curSignal->convMethod == "toUSINT")|| (curSignal->convMethod == "xtoUSINT")){
            ST.append("\nCont_"+curSignal->name+"				:= BYTE_TO_USINT(Raw_"+curSignal->name+") ;");
        }
        else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){
            ST.append("\nCont_"+curSignal->name+"				:= USINT_TO_REAL(BYTE_TO_USINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+"+QString::number(curSignal->offset,'g',15)))+";");
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e              := (Raw_"+curSignal->name+" > 16#FD);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na             := (Raw_"+curSignal->name+" = 16#FF);") : (""));

    }else if((curSignal->length<17)){
        if((curSignal->convMethod == "toWORD")|| (curSignal->convMethod == "xtoWORD")){
            ST.append("\nCont_"+curSignal->name+"				:= Raw_"+curSignal->name+" ;");
        }
        else if ((curSignal->convMethod == "toUINT")|| (curSignal->convMethod == "xtoUINT")){
            ST.append("\nCont_"+curSignal->name+"				:= WORD_TO_UINT(Raw_"+curSignal->name+") ;");
        }
        else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){
            ST.append("\n Cont_"+curSignal->name+"				:= UINT_TO_REAL(WORD_TO_UINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+"+QString::number(curSignal->offset,'g',15)))+";");
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e              := (Raw_"+curSignal->name+" > 16#FDFF);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na             := (Raw_"+curSignal->name+" > 16#FEFF);") : (""));
    }else if((curSignal->length<33)){
        if((curSignal->convMethod == "toDWORD")|| (curSignal->convMethod == "xtoDWORD")){
            ST.append("\nCont_"+curSignal->name+"               := Raw_"+curSignal->name+" ;");
        }
        else if ((curSignal->convMethod == "toUDINT")|| (curSignal->convMethod == "xtoUDINT")){
            ST.append("\nCont_"+curSignal->name+"				:= DWORD_TO_UDINT(Raw_"+curSignal->name+") ;");
        }
        else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){
            ST.append("\nCont_"+curSignal->name+"				:= UDINT_TO_REAL(DWORD_TO_UDINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+"+QString::number(curSignal->offset,'g',15)))+";");
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e              := (Raw_"+curSignal->name+" > 16#FDFFFFFF);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na             := (Raw_"+curSignal->name+" > 16#FEFFFFFF);") : (""));

    }else if((curSignal->length<65)){
        if((curSignal->convMethod == "toLWORD")|| (curSignal->convMethod == "xtoLWORD")){
            ST.append("\nCont_"+curSignal->name+"               := Raw_"+curSignal->name+" ;");
        }
        else if ((curSignal->convMethod == "toULINT")|| (curSignal->convMethod == "xtoULINT")){
            ST.append("\nCont_"+curSignal->name+"				:= LWORD_TO_ULINT(Raw_"+curSignal->name+") ;");
        }
        else if ((curSignal->convMethod == "toLREAL")|| (curSignal->convMethod == "xtoLREAL")){
            ST.append("\nCont_"+curSignal->name+"				:= ULINT_TO_LREAL(LWORD_TO_ULINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+"+QString::number(curSignal->offset,'g',15)))+";");
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e              := (Raw_"+curSignal->name+" > 16#FDFFFFFFFFFFFFFF);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na             := (Raw_"+curSignal->name+" > 16#FEFFFFFFFFFFFFFF);") : (""));

    }// TYPE CONVERTION AND E  NA CONTROL ENDS
// ADD COMMON PART EXCEPT BOOL AND 2XBOOLS
    //if((curSignal->length!= 1) && (!(curSignal->length==2 && (curSignal->convMethod !="toByte")))){
    //    ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".RangeExcd 		:= NOT ((Cont_"+curSignal->name+" >= "+QString::number(curSignal->minValue,'g',(curSignal->length>32)? 20:15)+") AND (Cont_"+curSignal->name+" <= "+QString::number(curSignal->maxValue,'g',(curSignal->length>32)? 20:15)+"));"
    //    +((!this->enableFrc)?("\nGVL."+this->dutHeader+"."+curSignal->name+".v				:= NOT( GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+" OR GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd "+((curSignal->isJ1939 ==true)?("OR GVL."+this->dutHeader+"."+curSignal->name+".e OR GVL."+this->dutHeader+"."+curSignal->name+".na) "):(") "))+";"):("\nGVL."+this->dutHeader+"."+curSignal->name+".v				:= NOT( GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+" OR GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd "+((curSignal->isJ1939 ==true)?("OR GVL."+this->dutHeader+"."+curSignal->name+".e OR GVL."+this->dutHeader+"."+curSignal->name+".na) OR "):(") OR "))+"FrcVar."+curSignal->name+".v ;"))+
    //    "\n"
    //    +((!this->enableFrc)?(""):("\nIF FrcVar."+curSignal->name+".v  THEN"))+
    //    ((!this->enableFrc)?(""):("\n	GVL."+this->dutHeader+"."+curSignal->name+".x 		   	:= FrcVar."+curSignal->name+".x ;\nELS"))+
    //    "IF GVL."+this->dutHeader+"."+curSignal->name+".v THEN"
    //    "\n	GVL."+this->dutHeader+"."+curSignal->name+".x 		   	:= Cont_"+curSignal->name+";"
    //    "\nELSE"
    //    "\n	GVL."+this->dutHeader+"."+curSignal->name+".x 		   	:= "+QString::number(curSignal->defValue,'g',15)+";"
    //    "\nEND_IF;\n");
    //}
    //SEL version
    if((curSignal->length!= 1) && (!(curSignal->length==2 && (curSignal->convMethod !="toByte")))){
        ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".RangeExcd 		:= NOT ((Cont_"+curSignal->name+" >= "+QString::number(curSignal->minValue,'g',(curSignal->length>32)? 20:15)+") AND (Cont_"+curSignal->name+" <= "+QString::number(curSignal->maxValue,'g',(curSignal->length>32)? 20:15)+"));"
        +((!this->enableFrc)?("\nGVL."+this->dutHeader+"."+curSignal->name+".v				:= NOT( GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+" OR GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd "+((curSignal->isJ1939 ==true)?("OR GVL."+this->dutHeader+"."+curSignal->name+".e OR GVL."+this->dutHeader+"."+curSignal->name+".na) "):(") "))+";"):("\nGVL."+this->dutHeader+"."+curSignal->name+".v				:= NOT( GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+" OR GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd "+((curSignal->isJ1939 ==true)?("OR GVL."+this->dutHeader+"."+curSignal->name+".e OR GVL."+this->dutHeader+"."+curSignal->name+".na) OR "):(") OR "))+"FrcVar."+curSignal->name+".v ;"))+
        "\n");
        if(this->enableFrc){
            ST.append("GVL."+this->dutHeader+"."+curSignal->name+".x 		   	:= SEL("+"FrcVar."+curSignal->name+".v"+","+"FrcVar."+curSignal->name+".x"+",SEL("+"GVL."+this->dutHeader+"."+curSignal->name+".v"+","+"Cont_"+curSignal->name+","+QString::number(curSignal->defValue,'g',15)+");");
        }else{
            ST.append("GVL."+this->dutHeader+"."+curSignal->name+".x 		   	:= SEL("+"GVL."+this->dutHeader+"."+curSignal->name+".v"+","+"Cont_"+curSignal->name+","+QString::number(curSignal->defValue,'g',15)+");");
        }

    }


    ST.append("\n{endregion}");
    return ST;


}
///******************************************************************************
/// TRANSMIT MESSAGES FUNCTION BLOCK ST GENERATOR
///******************************************************************************
void DBCHandler::generateIOPous(QDomElement * pous, QDomDocument &doc)
{
    QDomAttr attr;
    QDomText text;

    QString namePou = "P_"+this->dutHeader;
     //For AND and OR gate to manipulate timeout and disturbance flags

            /*generate block struct type new block*/

            structFbdBlock* newBlock = new structFbdBlock;
            QDomElement pou = doc.createElement("pou");
            /*set pou name*/
            attr=doc.createAttribute("name");
            attr.setValue(namePou);
            pou.setAttributeNode(attr);
            /*set pouType*/
            attr=doc.createAttribute("pouType");
            attr.setValue("program");
            pou.setAttributeNode(attr);

            /*Interface*/
            QDomElement interface = doc.createElement("interface");

            /*Generate Local Variables - localVars*/
            QString STcode;
            this->generateIOST(&STcode);
//Generate local vars
            {
                QDomElement localVars= doc.createElement("localVars");
                /*Function block declaration*/
                foreach (dataContainer * curMessage , comInterface){
                    if(curMessage->getIfSelected()){
                        QDomElement variable=doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue(curMessage->getIfBitOperation()?("_FB_CanTx_Message_Unpack_"+curMessage->getID()):("_FB_CanTx_Message_"+curMessage->getID()));
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement derived = doc.createElement("derived");
                        attr=doc.createAttribute("name");
                        attr.setValue(curMessage->getIfBitOperation()?("_FB_CanTx_Message_Unpack"):("_FB_CanTx_Message"));
                        derived.setAttributeNode(attr);
                        type.appendChild(derived);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                }

                foreach (dataContainer * curMessage , comInterface){
                    if(curMessage->getIfSelected()){
                        for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                            QDomElement variable=doc.createElement("variable");
                            attr=doc.createAttribute("name");
                            attr.setValue("Cont_"+curSignal->name);
                            variable.setAttributeNode(attr);
                            QDomElement type = doc.createElement("type");
                            QDomElement dataType = doc.createElement(curSignal->comDataType);
                            type.appendChild(dataType);
                            variable.appendChild(type);
                            localVars.appendChild(variable);
                        }
                    }
                }
                if(this->enableFrc){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("FrcVar");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue(this->dutName);
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }

                if(this->enableTest){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("FrcTest");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue(this->dutName);
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }
                for (unsigned i =0; i<counterfbLWORDTOBYTE; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_UNPACK_LWORD_TO_BYTE_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_UNPACK_LWORD_TO_BYTE");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);

                }
                for (unsigned i =0; i<counterfbDWORDTOBYTE; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_UNPACK_DWORD_TO_BYTE_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_UNPACK_DWORD_TO_BYTE");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }
                for (unsigned i =0; i<counterfbWORDTOBYTE; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_UNPACK_WORD_TO_BYTE_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_UNPACK_WORD_TO_BYTE");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }
                for (unsigned i =0; i<counterfbBYTETO8BIT; i++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_UNPACK_BYTE_TO_8BITS_"+QString::number(i));
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr = doc.createAttribute("name");
                    attr.setValue("_FB_UNPACK_BYTE_TO_8BITS");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }

                counterfbLWORDTOBYTE = 0;
                counterfbDWORDTOBYTE = 0;
                counterfbWORDTOBYTE = 0;
                counterfbBYTETO8BIT = 0;


                interface.appendChild(localVars);
            }
            pou.appendChild(interface);

            /*Create Body*/
            QDomElement body = doc.createElement("body");
            QDomElement ST = doc.createElement("ST");
            QDomElement xhtml = doc.createElement("xhtml");
            attr=doc.createAttribute("xmlns");
            attr.setValue("http://www.w3.org/1999/xhtml");
            xhtml.setAttributeNode(attr);
            text=doc.createTextNode(STcode);
            xhtml.appendChild(text);
            ST.appendChild(xhtml);
            body.appendChild(ST);
            pou.appendChild(body);

            /*Create addData*/
            QDomElement addData = doc.createElement("addData");
            QDomElement data = doc.createElement("data");
            attr=doc.createAttribute("name");
            attr.setValue("http://www.3s-software.com/plcopenxml/objectid");
            data.setAttributeNode(attr);
            attr=doc.createAttribute("handleUnknown");
            attr.setValue("discard");
            data.setAttributeNode(attr);
            QDomElement ObjectId = doc.createElement("ObjectId");
            text=doc.createTextNode(this->pouObjID);
            ObjectId.appendChild(text);
            data.appendChild(ObjectId);
            addData.appendChild(data);
            pou.appendChild(addData);
            pous->appendChild(pou);


           fbdBlocks.append(newBlock);
           // append input variables to AND gate to check disturbance -> EXP format : GVL.IIXC.S_TmOut_Motor_Messages_3_0X512


}
void DBCHandler::generateIOST(QString *const ST)
{       ST->append("(*\n"
                   "**********************************************************************\n"
                   "Bozankaya A.Ş.\n"
                   "**********************************************************************\n"
                   "Name					: P_"+dutHeader+"\n"
                   "POU Type				: Program\n"
                   "Created by              : COMMUNICATION INTERFACE GENERATOR("+QHostInfo::localHostName()+"), BZK.\n"
                   "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
                   "Modifications			: see version description below\n"
                   "\n"
                   "\n"
                   "Program Description:"
                   "This program which is created by automatically by communication interface generator \n handles the TX (OUTPUT) can messages according to CAN_500_LIB \n"
                   "*********************************************************************\n"
                   "\n"
                   "Version 1	\n"
                   "*********************************************************************\n"
                   "Version Description:\n"
                   "- initial version\n"
                   "*********************************************************************\n"
                   "*)");
        foreach (dataContainer * curMessage , comInterface){
            if(curMessage->getIfSelected()){
                 ST->append("\n{region \" User Area : "+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
                for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                 ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".v := "+((this->enableTest==true)?("FrcTest."+curSignal->name+".v;"):("ASSIGN HERE VALIDITY VARIABLE !!;")));
                 ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".x := "+((this->enableTest==true)?("FrcTest."+curSignal->name+".x;"):("ASSIGN HERE VALIDITY VARIABLE !!;")));
                 if(curSignal->isJ1939){
                    ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".e := "+((this->enableTest==true)?("FrcTest."+curSignal->name+".e;"):("ASSIGN HERE ERROR FLAG VARIABLE !!;")));
                    ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".na := "+((this->enableTest==true)?("FrcTest."+curSignal->name+".na;"):("ASSIGN HERE NOT AVAILABLE FLAG VARIABLE !!;")));
                 }
                }
                ST->append("\n{endregion}");
            }
        }
    foreach (dataContainer * curMessage , comInterface){
        if(curMessage->getIfSelected()){
            ST->append("\n{region \""+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
            QString nameFb = (curMessage->getIfBitOperation())? ("_FB_CanTx_Message_Unpack_"+curMessage->getID()) : ("_FB_CanTx_Message_"+curMessage->getID());
            ST->append("\n"+nameFb+"("
                       "\n   C_Enable:= GVL."+dutHeader+".S_CAN_Init,"
                       "\n   Obj_CAN:= ADR("+this->canLine+"),"
                       "\n   X_MsgID:= 16#"+curMessage->getID()+","
                       "\n   Tm_CycleTime:= TIME#"+curMessage->getMsCycleTime()+"ms,"
                       "\n   N_Msg_Dlc:="+QString::number(curMessage->getDLC())+","
                       "\n   S_Extended:= "+QString::number(curMessage->getIfExtended()).toUpper()+","
                       "\n   S_Sent_Ok   => GVL."+dutHeader+".S_SentOk_"+curMessage->getName()+"_0X"+curMessage->getID()+");");

            for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                ST->append(convTypeApptoCom(curSignal,curMessage->getID(),curMessage->getName(),nameFb));
            }
            ST->append("\n{endregion}");
        }
    }

}
QString DBCHandler::convTypeApptoCom (const dataContainer::signal *curSignal, QString idMessage, QString nameMessage,  QString nameFb)
{
    QString ST="\n{region \""+curSignal->name+"\"}";
    bool flagConversion =false;
    QString stat1,stat2,stat3,stat4,stat5 = "";

        // TYPE CONVERTION AND E  NA CONTROL STARTS
        if((curSignal->length<9)){
            if((curSignal->convMethod == "toBYTE")|| (curSignal->convMethod == "xtoBYTE")){

                stat1 = "FrcVar."+curSignal->name+".x";
                stat2 = "GVL."+this->dutHeader+"."+curSignal->name+".x";
                stat3 = "16#FF";
                stat4 = "16#FE";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toUSINT")|| (curSignal->convMethod == "xtoUSINT")){

                stat1 = "USINT_TO_BYTE(FrcVar."+curSignal->name+".x)";
                stat2 = "USINT_TO_BYTE(GVL."+this->dutHeader+"."+curSignal->name+".x )";
                stat3 = "16#FF";
                stat4 = "16#FE";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){

                stat1 = "USINT_TO_BYTE(REAL_TO_USINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat2 = "USINT_TO_BYTE(REAL_TO_USINT((GVL."+this->dutHeader+"."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat3 = "16#FF";
                stat4 = "16#FE";
                stat5 = (((curSignal->offset == 0) && (curSignal->defValue==0))? (" 0 "): ("USINT_TO_BYTE(REAL_TO_USINT(("+((curSignal->defValue==0) ? "" : (QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15)))+(((!(curSignal->offset == 0) && !(curSignal->defValue==0))?("+"):(""))+((curSignal->offset ==0 )? (""): (QString::number((-1*curSignal->offset),'g',15))))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))"));
                flagConversion=true;
            }
        }else if((curSignal->length<17)){
            if((curSignal->convMethod == "toWORD")|| (curSignal->convMethod == "xtoWORD")){
                stat1 = "FrcVar."+curSignal->name+".x";
                stat2 = "GVL."+this->dutHeader+"."+curSignal->name+".x";
                stat3 = "16#FFFF";
                stat4 = "16#FEFE";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toUINT")|| (curSignal->convMethod == "xtoUINT")){

                stat1 = "UINT_TO_WORD(FrcVar."+curSignal->name+".x)";
                stat2 = "UINT_TO_WORD(GVL."+this->dutHeader+"."+curSignal->name+".x )";
                stat3 = "16#FFFF";
                stat4 = "16#FEFE";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){

                stat1 = "UINT_TO_WORD(REAL_TO_UINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat2 = "UINT_TO_WORD(REAL_TO_UINT((GVL."+this->dutHeader+"."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat3 = "16#FFFF";
                stat4 = "16#FEFE";
                stat5 = (((curSignal->offset == 0) && (curSignal->defValue==0))? (" 0 "): ("UINT_TO_WORD(REAL_TO_UINT(("+((curSignal->defValue==0) ? "" : (QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15)))+(((!(curSignal->offset == 0) && !(curSignal->defValue==0))?("+"):(""))+((curSignal->offset ==0 )? (""): (QString::number((-1*curSignal->offset),'g',15))))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))"));
                flagConversion=true;
            }
        }else if((curSignal->length<33)){
            if((curSignal->convMethod == "toDWORD")|| (curSignal->convMethod == "xtoDWORD")){

                stat1 = "FrcVar."+curSignal->name+".x";
                stat2 = "GVL."+this->dutHeader+"."+curSignal->name+".x";
                stat3 = "16#FFFFFFFF";
                stat4 = "16#FEFEFEFE";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toUDINT")|| (curSignal->convMethod == "xtoUDINT")){


                stat1 = "UDINT_TO_DWORD(FrcVar."+curSignal->name+".x)";
                stat2 = "UDINT_TO_DWORD(GVL."+this->dutHeader+"."+curSignal->name+".x )";
                stat3 = "16#FFFFFFFF";
                stat4 = "16#FEFEFEFE";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;

            }
            else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){

                stat1 = "UDINT_TO_DWORD(REAL_TO_UDINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat2 = "UDINT_TO_DWORD(REAL_TO_UDINT((GVL."+this->dutHeader+"."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat3 = "16#FFFFFFFF";
                stat4 = "16#FEFEFEFE";
                stat5 = (((curSignal->offset == 0) && (curSignal->defValue==0))? (" 0 "): ("UDINT_TO_DWORD(REAL_TO_UDINT(("+((curSignal->defValue==0) ? "" : (QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15)))+(((!(curSignal->offset == 0) && !(curSignal->defValue==0))?("+"):(""))+((curSignal->offset ==0 )? (""): (QString::number((-1*curSignal->offset),'g',15))))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))"));
                flagConversion=true;
            }

        }else if((curSignal->length<65)){
            if((curSignal->convMethod == "toLWORD")|| (curSignal->convMethod == "xtoLWORD")){

                stat1 = "FrcVar."+curSignal->name+".x";
                stat2 = "GVL."+this->dutHeader+"."+curSignal->name+".x";
                stat3 = "16#FFFFFFFFFFFFFFFF";
                stat4 = "16#FEFEFEFEFEFEFEFE";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toULINT")|| (curSignal->convMethod == "xtoULINT")){

                stat1 = "ULINT_TO_LWORD(FrcVar."+curSignal->name+".x)";
                stat2 = "ULINT_TO_LWORD(GVL."+this->dutHeader+"."+curSignal->name+".x )";
                stat3 = "16#FFFFFFFFFFFFFFFF";
                stat4 = "16#FEFEFEFEFEFEFEFE";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toLREAL")|| (curSignal->convMethod == "xtoLREAL")){

                stat1 = "ULINT_TO_LWORD(LREAL_TO_ULINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat2 = "ULINT_TO_LWORD(LREAL_TO_ULINT((GVL."+this->dutHeader+"."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat3 = "16#FFFFFFFFFFFFFFFF";
                stat4 = "16#FEFEFEFEFEFEFEFE";
                stat5 = (((curSignal->offset == 0) && (curSignal->defValue==0))? (" 0 "): ("ULINT_TO_LWORD(LREAL_TO_ULINT(("+((curSignal->defValue==0) ? "" : (QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15)))+(((!(curSignal->offset == 0) && !(curSignal->defValue==0))?("+"):(""))+((curSignal->offset ==0 )? (""): (QString::number((-1*curSignal->offset),'g',15))))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))"));
                flagConversion=true;
            }

        }

        if (flagConversion){
            ST.append("GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd :=  ("+QString::number(curSignal->maxValue,'g',(curSignal->length>32)? 20:15)+" < GVL."+this->dutHeader+"."+curSignal->name+".x )OR ("+QString::number(curSignal->minValue,'g',(curSignal->length>32)? 20:15)+" > GVL."+this->dutHeader+"."+curSignal->name+".x);"
                      "\n //Transmit Data : Range Control End \n");
            if ((this->enableFrc) && (curSignal->isJ1939)){
                ST.append(
                "Cont_"+curSignal->name+":= SEL ("+"USINT_TO_BYTE(FrcVar."+curSignal->name+".x)"+","+stat1+","+"SEL ("+"(GVL."+this->dutHeader+"."+curSignal->name+".v AND NOT GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd)"+","+stat2+","+"SEL ("+"GVL."+this->dutHeader+"."+curSignal->name+".na"+","+stat3+","+"SEL ("+"GVL."+this->dutHeader+"."+curSignal->name+".e"+","+stat4+","+stat5+")"+")"+")"+");"
                );
            }else if((!this->enableFrc) && (curSignal->isJ1939)){
                ST.append(
                "Cont_"+curSignal->name+" := SEL ("+"(GVL."+this->dutHeader+"."+curSignal->name+".v AND NOT GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd)"+","+stat2+","+"SEL ("+"GVL."+this->dutHeader+"."+curSignal->name+".na"+","+stat3+","+"SEL ("+"GVL."+this->dutHeader+"."+curSignal->name+".e"+","+stat4+","+stat5+")"+")"+");"
                );
            }else if((this->enableFrc) && (!curSignal->isJ1939)){
                ST.append(
                "Cont_"+curSignal->name+" := SEL ("+"USINT_TO_BYTE(FrcVar."+curSignal->name+".x)"+","+stat1+","+"SEL ("+"(GVL."+this->dutHeader+"."+curSignal->name+".v AND NOT GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd)"+","+stat2+","+stat5+")"+");"
                );
            }else{
                ST.append(
                "Cont_"+curSignal->name+" := SEL ("+"(GVL."+this->dutHeader+"."+curSignal->name+".v AND NOT GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd)"+","+stat2+","+stat5+");"
                );
            }
            flagConversion = false;
        }

        if(curSignal->convMethod=="BOOL:BOOL"){
            ST.append("\n"+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+"               :=	GVL."+this->dutHeader+"."+curSignal->name+".x AND GVL."+this->dutHeader+"."+curSignal->name+".v ;");
        }else if(curSignal->convMethod=="2BOOL:BOOL"){
            ST.append("\n"+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+"               :=	 GVL."+this->dutHeader+"."+curSignal->name+".x ;"
                      "\n"+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+"             :=	 NOT GVL."+this->dutHeader+"."+curSignal->name+".v ; ");
        }else if((curSignal->convMethod=="xtoBYTE")||(curSignal->convMethod=="xtoUSINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==8))){
            ST.append("\n"+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+"            :=Cont_"+curSignal->name+";");
        }else if((curSignal->convMethod=="xtoWORD")||(curSignal->convMethod=="xtoUINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==16))){
            ST.append("\n_FB_UNPACK_WORD_TO_BYTE_"+QString::number(counterfbWORDTOBYTE)+"(X_WORD_0:= Cont_"+curSignal->name+", X_BYTE_0=> "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", X_BYTE_1=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+");");
            counterfbWORDTOBYTE++;
        }else if((curSignal->convMethod=="xtoDWORD")||(curSignal->convMethod=="xtoUDINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==32))){
            ST.append("\n_FB_UNPACK_DWORD_TO_BYTE_"+QString::number(counterfbDWORDTOBYTE)+"(X_DWORD_0:= Cont_"+curSignal->name+", X_BYTE_0=> "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", X_BYTE_1=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+", X_BYTE_2=>"+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+2)+", X_BYTE_3=>"+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+3)+");");
            counterfbDWORDTOBYTE++;
        }else if((curSignal->convMethod=="xtoLWORD")||(curSignal->convMethod=="xtoULINT")||(curSignal->convMethod=="xtoLREAL") ){
            ST.append("\n_FB_UNPACK_LWORD_TO_BYTE_"+QString::number(counterfbLWORDTOBYTE)+"(X_LWORD_0:= Cont_"+curSignal->name+", X_BYTE_0=> "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", X_BYTE_1=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+", X_BYTE_2=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+2)+", X_BYTE_3=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+3)+", X_BYTE_4=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+4)+", X_BYTE_5=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+5)+", X_BYTE_6=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+6)+", X_BYTE_7=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+7)+");");
            counterfbLWORDTOBYTE++;
            qInfo()<<"counter: "+QString::number(counterfbLWORDTOBYTE);
        }else{
            bool flagPack = false;
            if((curSignal->length>8)){
                if(curSignal->length<17){
                ST.append("\n_FB_UNPACK_WORD_TO_BYTE_"+QString::number(counterfbWORDTOBYTE)+"(X_WORD_0:=Cont_"+curSignal->name+");");
                counterfbWORDTOBYTE++;
                }else if(curSignal->length <33){
                 ST.append("_FB_UNPACK_DWORD_TO_BYTE_"+QString::number(counterfbDWORDTOBYTE)+"(X_DWORD_0:=Cont_"+curSignal->name+");");
                 counterfbDWORDTOBYTE++;
                }else {
                  ST.append("\n_FB_UNPACK_LWORD_TO_BYTE_"+QString::number(counterfbLWORDTOBYTE)+"(X_LWORD_0:=Cont_"+curSignal->name+");");
                  counterfbLWORDTOBYTE++;
                  qInfo()<<"counter: "+QString::number(counterfbLWORDTOBYTE);
               }
            }
            unsigned packID=0;
            unsigned packByteID=0;
            unsigned counterBITBYTE=0;

            for(unsigned i =0; i<curSignal->length;i++){

                if(((i%8 == 0) && ((i/8)<8) )&& (!flagPack)){
                    ST.append("\n_FB_UNPACK_BYTE_TO_8BITS_"+QString::number(counterfbBYTETO8BIT+qFloor(i/8.0))+"(");
                    counterBITBYTE++;
                    flagPack=true;
                }
                if(flagPack){
                        ST.append("S_Bit_"+QString::number(i%8)+"=> "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+i)+"");
                        if(((i%8 != 7) &&(i%(curSignal->length-1) != 0)) || (i==0)) {
                            ST.append(",");
                        }
                }
                if(flagPack){
                    if(((i%8 == 7)||(i%(curSignal->length-1) == 0) )&& (i>0)){
                        ST.append(" ,X_BYTE_0:=");
                        if(curSignal->length<9){
                        ST.append("Cont_"+curSignal->name);
                        ST.append(");");
                        }else if (curSignal->length <17){
                        ST.append("_FB_UNPACK_WORD_TO_BYTE_"+QString::number(counterfbWORDTOBYTE-1));
                        ST.append(".X_BYTE_"+QString::number(packByteID)+");");
                        }else if (curSignal->length <33){
                        ST.append("_FB_UNPACK_DWORD_TO_BYTE_"+QString::number(counterfbDWORDTOBYTE-1)+"");
                        ST.append(".X_BYTE_"+QString::number(packByteID)+");");
                        }else{
                        ST.append("_FB_UNPACK_LWORD_TO_BYTE_"+QString::number(counterfbLWORDTOBYTE-1)+"");
                        ST.append(".X_BYTE_"+QString::number(packByteID)+");");
                        }

                        flagPack=false;
                        packByteID++;
                    }
                }

            }


            counterfbBYTETO8BIT = counterfbBYTETO8BIT+ counterBITBYTE;
        }

        ST.append("\n{endregion}");
        return ST;

}
