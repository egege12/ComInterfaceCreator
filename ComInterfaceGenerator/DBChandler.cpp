#include "DBChandler.h"
#include "datacontainer.h"
#include "qforeach.h"
#include <QtGlobal>
#include <QRegularExpression>
#include <QUuid>
#include <QHostInfo>
#include <QStandardPaths>
#include <QDir>

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

    this->canLine = "GVL.IC.Obj_CAN0";
    enableMultiEnable = false;
    enableFrc=false;
    enableTest=false;
    dataContainer::setWarning("INFO","Program başlatıldı");
}

DBCHandler::~DBCHandler()
{
        QString path= QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        dataContainer::setWarning("INFO",dbcPath+"dosyası kapatıldı");
        QFile logFile(((m_ComType=="CAN")?(path+"/CIG_log/log.txt"):(path+"/CIG_log/logETH.txt")));
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
    openFile();
}

void DBCHandler::clearData()
{
    isAllInserted = false;
    dataContainer::setWarning("INFO",dbcPath+"dosyası kapatıldı");
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
    dataContainer::warningMessages.clear();
    DBCHandler::selectedMessageCounter=0;
    enableMultiEnable = false;
    enableFrc=false;
    enableTest=false;

}

void DBCHandler::readFile(QString fileLocation)
{
    dbcPath = fileLocation;
    if(this->isAllInserted){
        this->setErrCode("DBC zaten içeri aktarılmış, programı yeniden başlatın");
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

void DBCHandler::setMultiEnableMode(bool checkStat)
{
    this->enableMultiEnable =checkStat;
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
                msgCommentList.append({ID,msTimeout,msCycleTime,commentContainer.remove(configComment)});
                if (configComment.contains("j1939",Qt::CaseInsensitive)){
                    for( dataContainer::signal * curSignal : *comInterface.value(ID)->getSignalList()){
                        if(!curSignal->enJ1939){
                            dataContainer::setWarning(ID,curSignal->name+" sinyali sinyal yorumunda J1939 istenilmiş ancak uzunluk 2,4,8,16,32,64 olmadığı için mümkün değil");
                        }else{
                            curSignal->isJ1939 = true;
                        }
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
                            curSignal->isJ1939 = ((configComment.contains(QString("j1939"),Qt::CaseInsensitive)) && curSignal->enJ1939)? true : (curSignal->isJ1939 && curSignal->enJ1939) ;
                            curSignal->defValue= defValue;
                            if((configComment.contains(QString("j1939"),Qt::CaseInsensitive)) && !curSignal->enJ1939){
                                dataContainer::setWarning(targetID,curSignal->name+" sinyali mesaj yorumunda J1939 istenilmiş ancak uzunluk 2,4,8,16,32,64 olmadığı için mümkün değil");
                            }
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
                double rawdefValue=containerLine.at(5).toDouble();

                if(comInterface.contains(targetID)){
                    for( dataContainer::signal * curSignal : *comInterface.value(targetID)->getSignalList()){
                        if( curSignal->name.contains(containerLine.at(4))){
                            curSignal->defValue= rawdefValue * curSignal->resolution + curSignal->offset;
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
    checkRepatedSignal(false);
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
    if((isExtended) && m_ComType=="ETH"){
        newMessage->setNotSelectable();
        dataContainer::setWarning(messageID,"ethernet arayüzü için extended ID tipi seçilemez.");
    }
    comInterface.insert(messageID,newMessage);
    return true;
}

bool DBCHandler::createObjIds()
{
    this->dutObjID = this->getRandomID();
    this->pouObjID = this->getRandomID();
    this->ethernetSocketID= this->getRandomID();

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

void DBCHandler::checkRepatedSignal(bool doChange)
{
    interface::Iterator iteInterface1;
    interface::Iterator iteInterface2;
    if(doChange){
        for(iteInterface1=comInterface.begin();iteInterface1!=comInterface.end();iteInterface1++){
            if(iteInterface1.value()->getIfSelected()){
                for(dataContainer::signal * curSignal : *iteInterface1.value()->getSignalList()){
                    bool isDublicated=false;
                    for(const QString dubID : curSignal->dublicateIDs){
                        if(comInterface.contains(dubID)){
                            if(comInterface.value(dubID)->getIfSelected()){
                                isDublicated=true;
                            }
                        }
                    }
                    if(isDublicated){
                        curSignal->name = curSignal->name+"_"+iteInterface1.value()->getID();
                        curSignal->isDublicated=true;
                    }
                }
            }
        }

    }else{
        for(iteInterface1=comInterface.begin();iteInterface1!=comInterface.end();iteInterface1++){
            for(dataContainer::signal * curSignal1 : *iteInterface1.value()->getSignalList()){
                for(iteInterface2=comInterface.begin();iteInterface2!=comInterface.end();iteInterface2++){
                    for(dataContainer::signal * curSignal2 : *iteInterface2.value()->getSignalList()){
                        if((curSignal1->name == curSignal2->name) && (iteInterface2.value()->getID() != iteInterface1.value()->getID())){
                                dataContainer::setWarning(iteInterface2.value()->getID(),curSignal2->name+" sinyali "+iteInterface1.value()->getID()+" mesajında da yer aldığı için eğer her iki mesajı seçerseniz sonuna ID eklenerek etiketlenecek.");
                                curSignal1->dublicateIDs.append(iteInterface2.value()->getID());
                                curSignal2->dublicateIDs.append(iteInterface1.value()->getID());
                        }
                    }
                }
            }
        }
    }
}

void DBCHandler::cleanRepatedManipulation()
{
    interface::Iterator iteInterface1;
    for(iteInterface1=comInterface.begin();iteInterface1!=comInterface.end();iteInterface1++){
        if(iteInterface1.value()->getIfSelected()){
            for(dataContainer::signal * curSignal : *iteInterface1.value()->getSignalList()){
                if(curSignal->isDublicated){
                    curSignal->name.remove("_"+iteInterface1.value()->getID());
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
    }else if (!dutHeader.contains(this->IOType)){
        this->setErrCode("DUT adı II veya IO etiketini seçimle aynı şekilde içermeli");
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
                    if(m_ComType == "ETH"){
                        createGVLINFO();
                        this->setErrCode("GVL içeriğine kopyalayacağınız kod masaüstüne oluşturuldu.");
                    }
                    setProgress(100);
                    QString infoText;
                    infoText.append("| ");
                    for(dataContainer *const curValue : comInterface){
                        if(curValue->getIfSelected()){
                            infoText.append(curValue->getID()+" | ");
                        }
                    }
                    dataContainer::setWarning("INFO","**"+this->dutHeader+" mesajları : "+infoText);
                    dataContainer::setWarning("INFO",this->dutHeader+",oluşturma konfigurasyonu, [çoklu enable modu]: "+((this->enableMultiEnable)?("aktif"):("pasif"))+", [force değişkenleri]: "+((this->enableFrc)?("aktif"):("pasif"))+", [test modu]: "+((this->enableTest)?("aktif"):("pasif")));
                    dataContainer::setWarning("INFO",this->dutHeader+" oluşturuldu. Oluşturma süresi:"+QString::number((QDateTime::currentMSecsSinceEpoch())-startTimeMs)+"ms");

                    emit infoListChanged();
                    cleanRepatedManipulation();
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
QString DBCHandler::ComType() const
{
    return m_ComType;
}
void DBCHandler::setComType(const QString &newComType)
{
    if (m_ComType == newComType)
        return;
    m_ComType = newComType;
    emit comTypeChanged();
    QString path= QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir dir(path);
    dir.mkdir("CIG_log");

    if(QFile::exists(((m_ComType=="CAN")?(path+"/CIG_log/log.txt"):(path+"/CIG_log/logETH.txt")))){
        QFile logFile(((m_ComType=="CAN")?(path+"/CIG_log/log.txt"):(path+"/CIG_log/logETH.txt")));
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
}
void DBCHandler::setPastCreatedMessages(QString textLine)
{
    QStringList temp = textLine.split("mesajları");
    QStringList messages = temp.at(1).trimmed().split("|");

    unsigned long counterFind = 0;
    for(dataContainer *const curValue : comInterface){
        if(curValue->getIfSelected()){
            curValue->setSelected();
        }
    }
    emit selectedStatChanged();
    for(QString messageID : messages){
        messageID = messageID.remove(":").trimmed();
        if(comInterface.contains(messageID)){
            comInterface.value(messageID)->setSelected();
            counterFind++;
        }
    }

    if(counterFind != 0){
        setErrCode(QString::number(counterFind)+" adet mesaj seçildi.");
        emit selectedStatChanged();
    }else{
        setErrCode("Eşleşen hiçbir mesaj bulunamadı");
    }

}
bool DBCHandler::createXml_STG1(QFile *xmlFile)
{
//Change repeated signal names
    checkRepatedSignal(true);

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
        attr.setValue("CODESYS V3.5 SP16");
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
        {
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
        }

        setProgress(30);
        if(m_ComType == "CAN"){
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
        }else if(m_ComType == "ETH"){
            if((this->IOType == "II")){
                setProgress(40);
                this->generateIIETH(&pous,doc);
                this->generateETHPou(&pous,doc);
                setProgress(50);
            }
            else{
                setProgress(40);
                this->generateIOETH(&pous,doc);
                this->generateETHPou(&pous,doc);
                setProgress(50);
            }
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

                    {
                        QDomElement value = doc.createElement("value");
                        attr = doc.createAttribute("member");
                        attr.setValue("v");
                        value.setAttributeNode(attr);
                        QDomElement simpleValue = doc.createElement("simpleValue");
                        attr = doc.createAttribute("value");
                        attr.setValue("FALSE");
                        simpleValue.setAttributeNode(attr);
                        value.appendChild(simpleValue);
                        structValue.appendChild(value);
                    }

                    initialValue.appendChild(structValue);
                    variable.appendChild(initialValue);
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
    if((dutName.contains("II")||dutName.contains("ii")) && m_ComType == "CAN"){
        bool flagBlankSpace=true;
        foreach(dataContainer *const curValue , comInterface){
            if(curValue->getIfSelected()){
                {
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
                if(this->enableMultiEnable){
                QDomElement variable = doc.createElement("variable");
                attr = doc.createAttribute("name");
                attr.setValue("C_En_"+curValue->getName()+"_0X"+curValue->getID());
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
                            comment.append("\n\nMESSAGE ENABLE BITS\n");
                            flagBlankSpace=false;
                        }

                        comment.append("Message enable bit for message ID 16#"+curValue->getID());
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

    }else if ((dutName.contains("IO")||dutName.contains("io")) && m_ComType == "CAN"){
        bool flagBlankSpace=true;
        foreach(dataContainer *const curValue , comInterface){
            if(curValue->getIfSelected()){
                {
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
                if(this->enableMultiEnable){
                QDomElement variable = doc.createElement("variable");
                attr = doc.createAttribute("name");
                attr.setValue("C_En_"+curValue->getName()+"_0X"+curValue->getID());
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
                            comment.append("\n\nMESSAGE ENABLE BITS\n");
                            flagBlankSpace=false;
                        }

                        comment.append("Message enable bit for message ID 16#"+curValue->getID());
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
    }else if ((dutName.contains("II")||dutName.contains("ii")) && m_ComType == "ETH"){
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
    }
    // CAN_Init register for DUT
    if(!this->enableMultiEnable){
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

void DBCHandler::generateETHPou(QDomElement *pous, QDomDocument &doc)
{
    QDomAttr attr;
    QDomText text;

    QString namePou = "P_IC_EthSocket";
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
    //Generate local vars
    {
        QDomElement localVars= doc.createElement("localVars");
        /*Constant declaration*/

        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("init");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement BOOL = doc.createElement("BOOL");
            type.appendChild(BOOL);
            variable.appendChild(type);
            QDomElement initialValue = doc.createElement("initialValue");
            QDomElement simpleValue = doc.createElement("simpleValue");
            attr=doc.createAttribute("value");
            attr.setValue("TRUE");
            simpleValue.setAttributeNode(attr);
            initialValue.appendChild(simpleValue);
            variable.appendChild(initialValue);
            localVars.appendChild(variable);

        }
        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("sock");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue("UdpSocket");
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            localVars.appendChild(variable);
        }
        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("recSize");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue("__XINT");
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            QDomElement initialValue = doc.createElement("initialValue");
            QDomElement simpleValue = doc.createElement("simpleValue");
            attr=doc.createAttribute("value");
            attr.setValue("0");
            simpleValue.setAttributeNode(attr);
            initialValue.appendChild(simpleValue);
            variable.appendChild(initialValue);
            localVars.appendChild(variable);
        }
        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("sendSize");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue("__XINT");
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            QDomElement initialValue = doc.createElement("initialValue");
            QDomElement simpleValue = doc.createElement("simpleValue");
            attr=doc.createAttribute("value");
            attr.setValue("0");
            simpleValue.setAttributeNode(attr);
            initialValue.appendChild(simpleValue);
            variable.appendChild(initialValue);
            localVars.appendChild(variable);
        }
        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("createResult");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue("IoStandard.RTS_IEC_RESULT");
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            QDomElement initialValue = doc.createElement("initialValue");
            QDomElement simpleValue = doc.createElement("simpleValue");
            attr=doc.createAttribute("value");
            attr.setValue("0");
            simpleValue.setAttributeNode(attr);
            initialValue.appendChild(simpleValue);
            variable.appendChild(initialValue);
            localVars.appendChild(variable);
        }
        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("bindResult");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue("IoStandard.RTS_IEC_RESULT");
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            QDomElement initialValue = doc.createElement("initialValue");
            QDomElement simpleValue = doc.createElement("simpleValue");
            attr=doc.createAttribute("value");
            attr.setValue("0");
            simpleValue.setAttributeNode(attr);
            initialValue.appendChild(simpleValue);
            variable.appendChild(initialValue);
            localVars.appendChild(variable);
        }
        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("recResult");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue("IoStandard.RTS_IEC_RESULT");
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            QDomElement initialValue = doc.createElement("initialValue");
            QDomElement simpleValue = doc.createElement("simpleValue");
            attr=doc.createAttribute("value");
            attr.setValue("0");
            simpleValue.setAttributeNode(attr);
            initialValue.appendChild(simpleValue);
            variable.appendChild(initialValue);
            localVars.appendChild(variable);
        }
        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("sendResult");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue("IoStandard.RTS_IEC_RESULT");
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            QDomElement initialValue = doc.createElement("initialValue");
            QDomElement simpleValue = doc.createElement("simpleValue");
            attr=doc.createAttribute("value");
            attr.setValue("0");
            simpleValue.setAttributeNode(attr);
            initialValue.appendChild(simpleValue);
            variable.appendChild(initialValue);
            localVars.appendChild(variable);
        }
        {
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("sockAddress");
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue("SysSocket.SOCKADDRESS");
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            localVars.appendChild(variable);
        }

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
    QString STcode;
    QString container = "Datagram_"+this->dutHeader;
    QString receiveADR= container.replace("IO","II");
    QString sendADR= container.replace("II","IO");
    STcode.append("// ----------------------------------------------------------------------------"
              "// This socket binds port 5500 and IP: 10.100.30.200\n"
              "// This is a bi-directional UDP datagram protocol. ECU sends data when data received.\n"
              "// Each datagram has 40 bytes.  \n"
              "// GVL."+this->dutHeader+" first data pointer connected to "+((this-dutHeader.contains("II"))?(" receive "):(" send "))+"data port, you should connect the other data point if it is not correct\n"
              "// DATAGRAM order : (byte type must be used in every posibility)\n"
              "// 2 byte ID0 - 8 byte data- 2 byte ID1 - 8byte data- 2byte ID2- 8byte data- 2byte ID3- 8byte data\n"
              "// ----------------------------------------------------------------------------\n"
              "// check, if in init phase\n"
              "IF (init = TRUE) THEN\n"
              "	// configure socket type\n"
              "	sockAddress.sin_family := SysSocket.SOCKET_AF_INET;\n"
              "\n"
              "	// configure socket port\n"
              "	sockAddress.sin_port:= 5500;\n"
              "\n"
              "	// open a UDP socket\n"
              "	sock.Create(iAddressFamily:= SysSocket.SOCKET_AF_INET,\n"
              "		diType:= SysSocket.SOCKET_DGRAM,\n"
              "		diProtocol:= SysSocket.SOCKET_IPPROTO_UDP,\n"
              "		pResult:= ADR(createResult));\n"
              "\n"
              "	// check, if the socket could be created\n"
              "	IF (createResult = Errors.ERR_OK) THEN\n"
              "		// bind socket, to be able to receive\n"
              "		bindResult := sock.Bind(\n"
              "			pSockAddr:= ADR(sockAddress), \n"
              "			diSockAddrSize:= SIZEOF(SysSocket.SOCKADDRESS));\n"
              "			\n"
              "		// check, if the socket bind worked\n"
              "		IF (createResult = Errors.ERR_OK) THEN\n"
              "			// clear init flag\n"
              "			init := FALSE;\n"
              "		END_IF\n"
              "	END_IF\n"
              "END_IF\n"
              "\n"
              "IF (init = FALSE) THEN\n"
              "	// try to receive a packet\n"
              "	recSize := sock.RecvFrom(\n"
              "			pbyBuffer:= ADR(GVL."+receiveADR+"),\n"
              "			diBufferSize:= SIZEOF(GVL."+receiveADR+"),\n"
              "			diFlags:= 0,\n"
              "			pSockAddr:=ADR(sockAddress) ,\n"
              "			diSockAddrSize:= SIZEOF(SysSocket.SOCKADDRESS),\n"
              "			pResult:= ADR(recResult));\n"
              "	\n"
              "	// check, if a packet has been received\n"
              "	IF (recResult = Errors.ERR_OK) THEN\n"
              "		// copy the packet's content to the send data buffer\n"
              "	\n"
              "		// try to send the packet back\n"
              "		sendSize := sock.SendTo(\n"
              "			pbyBuffer := ADR(GVL."+sendADR+"),\n"
              "			diBufferSize := SIZEOF(GVL."+sendADR+"),\n"
              "			diFlags := 0,\n"
              "			pSockAddr := ADR(sockAddress),\n"
              "			diSockAddrSize := SIZEOF(SysSocket.SOCKADDRESS),\n"
              "			pResult := ADR(sendResult));\n"
              "	END_IF\n"
              "END_IF\n" );

    STcode.append("");
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
    text=doc.createTextNode(this->ethernetSocketID);
    ObjectId.appendChild(text);
    data.appendChild(ObjectId);
    addData.appendChild(data);
    pou.appendChild(addData);
    pous->appendChild(pou);


    fbdBlocks.append(newBlock);
    // append input variables to AND gate to check disturbance -> EXP format : GVL.IIXC.S_TmOut_Motor_Messages_3_0X512


}

/// ///******************************************************************************
/// ETH II GENERATION
///******************************************************************************
void DBCHandler::generateIIETH(QDomElement *pous, QDomDocument &doc)
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
                this->generateIIETHST(&STcode);
                //Generate local vars
                {
                QDomElement localVars= doc.createElement("localVars");
                /*Function block declaration*/
                foreach (dataContainer * curMessage , comInterface){
                    if(curMessage->getIfSelected()){
                        QDomElement variable=doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("MSG_ETH_"+curMessage->getID());
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement derived = doc.createElement("derived");
                        attr=doc.createAttribute("name");
                        attr.setValue(curMessage->getIfBitOperation()?("_FB_ETHRx_Message_Unpack"):("MSG_ETH_T"));
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
                        }
                    }
                }
                for(unsigned z=0;z<4;z++){
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_PACK_BYTE_TO_WORD_ID"+QString::number(z));
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



}
void DBCHandler::generateIIETHST(QString * const ST)
{
                ST->append("(*\n"
                           "**********************************************************************\n"
                           "Bozankaya A.Ş.\n"
                           "**********************************************************************\n"
                           "Name					: P_"+dutName+"\n"
                           "POU Type				: Program\n"
                           "Created by              : COMMUNICATION INTERFACE GENERATOR "+Version+"("+QHostInfo::localHostName()+") , BZK.\n"
                           "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
                           "Modifications			: see version description below\n"
                           "\n"
                           "\n"
                           "Program Description:"
                           "This program which is created by automatically by communication interface generator \n handles the RX (INPUT) ethernet messages as 40 byte custom UDP datagram\n"
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
                        QString nameFb = "MSG_ETH_"+curMessage->getID();
                        if(curMessage->getIfBitOperation()){
                            ST->append( "\n"+nameFb+"();\n");
                        }
                    }
                }

                ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
                ST->append("\n{region \"Selection Area\"}\n");
                ST->append(this->generateIIETHDatagramST());
                ST->append("\n{endregion}");
                ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");

                foreach (dataContainer * curMessage , comInterface){
                if(curMessage->getIfSelected()){
                    ST->append("\n{region \" MESSAGE AREA :"+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
                    QString nameFb = "MSG_ETH_"+curMessage->getID();

                    for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                        ST->append(convTypeComtoApp(curSignal,curMessage->getID(),curMessage->getName(),nameFb));
                    }
                    ST->append("\n{endregion}");


                }
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
                            //variable=doc.createElement("variable");
                            //attr=doc.createAttribute("name");
                            //attr.setValue("Cont_"+curSignal->name);
                            //variable.setAttributeNode(attr);
                            //type = doc.createElement("type");
                            //dataType = doc.createElement(curSignal->appDataType);
                            //type.appendChild(dataType);
                            //variable.appendChild(type);
                            //localVars.appendChild(variable);
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
               "Created by              : COMMUNICATION INTERFACE GENERATOR "+Version+"("+QHostInfo::localHostName()+") , BZK.\n"
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
            ST->append("\n{region \" MESSAGE AREA :"+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
            QString nameFb = (curMessage->getIfBitOperation())? ("_FB_CanRx_Message_Unpack_"+curMessage->getID()) : ("_FB_CanRx_Message_"+curMessage->getID());
            ST->append( "\n"+nameFb+"(\n"
                        "     C_Enable:= "+((this->enableMultiEnable)?("GVL."+dutHeader+".C_En_"+curMessage->getName()+"_0X"+curMessage->getID()):("GVL."+dutHeader+".S_CAN_Init"))+",\n"
                        "     Obj_CAN:= ADR("+this->canLine+"),\n"
                        "     X_MsgID:= 16#"+curMessage->getID()+",\n"
                        "     Tm_MsgTmOut:= TIME#"+curMessage->getMsTimeOut()+"ms,\n"
                        "     S_Extended:= "+QString::number(curMessage->getIfExtended()).toUpper()+",\n"
                        "     S_ER_TmOut=> GVL."+dutHeader+".S_TmOut_"+curMessage->getName()+"_0X"+curMessage->getID()+");\n");

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

QString DBCHandler::generateIIETHDatagramST()
{
    QString ST;

    for(unsigned i= 0;i<4;i++){
        QString datagramID = "ID"+ QString::number(i);
        ST.append("_FB_PACK_BYTE_TO_WORD_"+datagramID+"(X_BYTE_0:= GVL.Datagram_"+dutHeader+"."+datagramID+"_LS, X_BYTE_1:= GVL.Datagram_"+dutHeader+"."+datagramID+"_HS);\n");
        ST.append("CASE _FB_PACK_BYTE_TO_WORD_"+datagramID+".X_WORD_0 OF\n");
        foreach (dataContainer * curMessage , comInterface){
            if(curMessage->getIfSelected()){
                ST.append("16#"+curMessage->getID()+":\n");
                for(unsigned k=0 ;k<8;k++){
                            ST.append("MSG_ETH_"+curMessage->getID()+"."+((curMessage->getIfBitOperation())?("I"):("X"))+"_Byte_"+QString::number(k)+" := GVL.Datagram_"+dutHeader+"."+datagramID+"_Byte_"+QString::number(k)+";\n");
                }
            }
        }
        ST.append("\n");
        ST.append("ELSE\n"
                  "(*DO NOTHING*)\n"
                  "END_CASE;\n");

    }

    return ST;
}
QString DBCHandler::convTypeComtoApp(const dataContainer::signal * curSignal, QString idMessage, QString nameMessage, QString nameFb)
{
    QString ST="\n{region \" SIGNAL AREA : NAME->"+curSignal->name+" LENGTH->"+QString::number(curSignal->length)+" STARTBIT :"+QString::number(curSignal->startBit)+"\"}";
    if(curSignal->convMethod=="BOOL:BOOL"){
        if(this->enableFrc){
        ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".v\t\t:= NOT GVL."+dutHeader+((this->m_ComType=="ETH")?(".S_Com_Flt"):(".S_TmOut_"+nameMessage+"_0X"+idMessage))+" OR FrcVar."+curSignal->name+".v ;"
                  "\nIF NOT FrcVar."+curSignal->name+".v THEN;"
                  "\nGVL."+this->dutHeader+"."+curSignal->name+".x\t\t:= GVL."+this->dutHeader+"."+curSignal->name+".v AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+";"
                  "\nELSE "
                  "\nGVL."+this->dutHeader+"."+curSignal->name+".x\t\t:= FrcVar."+curSignal->name+".x ;"
                  "\nEND_IF;");
        }else{
            ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".v\t\t:= NOT GVL."+dutHeader+((this->m_ComType=="ETH")?(".S_Com_Flt"):(".S_TmOut_"+nameMessage+"_0X"+idMessage))+";"
                      "\nGVL."+this->dutHeader+"."+curSignal->name+".x\t\t:= GVL."+this->dutHeader+"."+curSignal->name+".v AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+";");
        }
    }else if(curSignal->convMethod=="2BOOL:BOOL"){
        if(this->enableFrc){
        ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".v\t\t:= (NOT GVL."+dutHeader+((this->m_ComType=="ETH")?(".S_Com_Flt"):(".S_TmOut_"+nameMessage+"_0X"+idMessage))+" AND NOT "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+") OR FrcVar."+curSignal->name+".v ;"
                   "\nIF NOT FrcVar."+curSignal->name+".v THEN;"
                   "\nGVL."+this->dutHeader+"."+curSignal->name+".x\t\t:= GVL."+this->dutHeader+"."+curSignal->name+".v AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+";"
                   "\nELSE "
                   "\nGVL."+this->dutHeader+"."+curSignal->name+".x\t\t:= FrcVar."+curSignal->name+".x ;"
                   "\nEND_IF;");
        }else{
            ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".v\t\t:= NOT GVL."+dutHeader+".S_TmOut_"+nameMessage+"_0X"+idMessage+" AND NOT "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+";"
                       "\nGVL."+this->dutHeader+"."+curSignal->name+".x\t\t:= GVL."+this->dutHeader+"."+curSignal->name+".v AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+";");
        }
    }else if((curSignal->convMethod=="xtoBYTE")||(curSignal->convMethod=="xtoUSINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==8))){
        ST.append("\nRaw_"+curSignal->name+"\t\t:= "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+";");
    }else if((curSignal->convMethod=="xtoWORD")||(curSignal->convMethod=="xtoUINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==16))){
        ST.append("\n_FB_PACK_BYTE_TO_WORD_"+QString::number(counterfbBYTETOWORD)+"(\n\t\tX_BYTE_0:= "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", \n\t\tX_BYTE_1:=  "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+", \n\t\tX_WORD_0 => Raw_"+curSignal->name+");");
        counterfbBYTETOWORD++;
    }else if((curSignal->convMethod=="xtoDWORD")||(curSignal->convMethod=="xtoUDINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==32))){
        ST.append("\n_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+"(\n\t\tX_BYTE_0:= "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", \n\t\tX_BYTE_1:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+", \n\t\tX_BYTE_2:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+2)+", \n\t\tX_BYTE_3:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+3)+" , \n\t\tX_DWORD_0 => Raw_"+curSignal->name+");");
        counterfbBYTETODWORD++;
    }else if((curSignal->convMethod=="xtoLWORD")||(curSignal->convMethod=="xtoULINT")||(curSignal->convMethod=="xtoLREAL") ){
        ST.append("\n_FB_PACK_BYTE_TO_LWORD_"+QString::number(counterfbBYTETOLWORD)+"(\n\t\tX_BYTE_0:= "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+", \n\t\tX_BYTE_1:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+", \n\t\tX_BYTE_2:="+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+2)+" , \n\t\tX_BYTE_3:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+3)+", \n\t\tX_BYTE_4:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+4)+", \n\t\tX_BYTE_5:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+5)+", \n\t\tX_BYTE_6:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+6)+", \n\t\tX_BYTE_7:= "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+7)+" , \n\t\tX_LWORD_0 => Raw_"+curSignal->name+");");
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
                    ST.append("\n\t\tS_BIT_"+QString::number(i%8)+":= "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+i)+"");
                    if(((i%8 != 7) &&(i%(curSignal->length-1) != 0)) || (i==0)) {
                        ST.append(",");
                    }
            }
            if(flagPack){
                if(((i%8 == 7)||(i%(curSignal->length-1) == 0) )&& (i>0)){
                    ST.append(" ,\n\t\tX_BYTE_0=>");
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
                ST.append("\n_FB_PACK_DWORD_TO_LWORD_"+QString::number(counterfbDWORDTOLWORD)+"(\n\t\t\tX_DWORD_0:=_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+".X_DWORD_0,\n\t\tX_DWORD_1:=_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD+1)+".X_DWORD_0,\n\t\tX_LWORD_0=> Raw_"+curSignal->name+");");
            counterfbDWORDTOLWORD++;
            counterfbBYTETODWORD++;
            counterfbBYTETODWORD++;
            }
        }
        counterfb8BITTOBYTE = counterfb8BITTOBYTE+ counterBITBYTE;
    }
    QString cont_text="";
    // TYPE CONVERTION AND E  NA CONTROL STARTS
    if(curSignal->length==1){
        //do nothing
    }
    else if((curSignal->length==2) && (curSignal->convMethod == "2BOOL:BOOL")){

        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e\t\t\t:= "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+" AND NOT "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+" ;") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na\t\t\t:= "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+" AND "+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+" ;") : (""));

    }else if((curSignal->length==4)){

        if((curSignal->convMethod == "toBYTE")|| (curSignal->convMethod == "xtoBYTE")){
            cont_text.append("Raw_"+curSignal->name);
        }
        else if ((curSignal->convMethod == "toUSINT")|| (curSignal->convMethod == "xtoUSINT")){
            cont_text.append("BYTE_TO_USINT(Raw_"+curSignal->name+")");
        }
        else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){
            cont_text.append("USINT_TO_REAL(BYTE_TO_USINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+("+QString::number(curSignal->offset,'g',15)+")")))+")";
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e\t\t\t:= (Raw_"+curSignal->name+" = 16#E);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na\t\t\t:= (Raw_"+curSignal->name+" = 16#F);") : (""));

    }else if((curSignal->length<9)){
        if((curSignal->convMethod == "toBYTE")|| (curSignal->convMethod == "xtoBYTE")){
            cont_text.append("Raw_"+curSignal->name);
        }
        else if ((curSignal->convMethod == "toUSINT")|| (curSignal->convMethod == "xtoUSINT")){
            cont_text.append("BYTE_TO_USINT(Raw_"+curSignal->name+")");
        }
        else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){
            cont_text.append("USINT_TO_REAL(BYTE_TO_USINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+("+QString::number(curSignal->offset,'g',15)+")")))+")";
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e\t\t\t:= (Raw_"+curSignal->name+" > 16#FD) AND (Raw_"+curSignal->name+" < 16#FF);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na\t\t\t:= (Raw_"+curSignal->name+" = 16#FF);") : (""));

    }else if((curSignal->length<17)){
        if((curSignal->convMethod == "toWORD")|| (curSignal->convMethod == "xtoWORD")){
            cont_text.append("Raw_"+curSignal->name);
        }
        else if ((curSignal->convMethod == "toUINT")|| (curSignal->convMethod == "xtoUINT")){
            cont_text.append("WORD_TO_UINT(Raw_"+curSignal->name+")");
        }
        else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){
            cont_text.append("UINT_TO_REAL(WORD_TO_UINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+("+QString::number(curSignal->offset,'g',15)+")")))+")";
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e\t\t\t:= (Raw_"+curSignal->name+" > 16#FDFF) AND (Raw_"+curSignal->name+" <= 16#FEFF);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na\t\t\t:= (Raw_"+curSignal->name+" > 16#FEFF);") : (""));
    }else if((curSignal->length<33)){
        if((curSignal->convMethod == "toDWORD")|| (curSignal->convMethod == "xtoDWORD")){
            cont_text.append("Raw_"+curSignal->name);
        }
        else if ((curSignal->convMethod == "toUDINT")|| (curSignal->convMethod == "xtoUDINT")){
            cont_text.append("DWORD_TO_UDINT(Raw_"+curSignal->name+")");
        }
        else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){
            cont_text.append("UDINT_TO_REAL(DWORD_TO_UDINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+("+QString::number(curSignal->offset,'g',15)+")")))+")";
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e\t\t\t:= (Raw_"+curSignal->name+" > 16#FDFFFFFF) AND (Raw_"+curSignal->name+" <= 16#FEFFFFFF);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na\t\t\t:= (Raw_"+curSignal->name+" > 16#FEFFFFFF);") : (""));

    }else if((curSignal->length<65)){
        if((curSignal->convMethod == "toLWORD")|| (curSignal->convMethod == "xtoLWORD")){
            cont_text.append("Raw_"+curSignal->name);
        }
        else if ((curSignal->convMethod == "toULINT")|| (curSignal->convMethod == "xtoULINT")){
            cont_text.append("LWORD_TO_ULINT(Raw_"+curSignal->name+")");
        }
        else if ((curSignal->convMethod == "toLREAL")|| (curSignal->convMethod == "xtoLREAL")){
            cont_text.append("ULINT_TO_LREAL(LWORD_TO_ULINT(Raw_"+curSignal->name+"))"+((curSignal->resolution == 1)? (""):("*"+QString::number(curSignal->resolution,'g',15)))+((curSignal->offset == 0)? (""):("+("+QString::number(curSignal->offset,'g',15)+")")))+")";
        }
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".e\t\t\t:= (Raw_"+curSignal->name+" > 16#FDFFFFFFFFFFFFFF) AND (Raw_"+curSignal->name+" <= 16#FEFFFFFFFFFFFFFF);") : (""));
        ST.append((curSignal->isJ1939==true)? ("\nGVL."+this->dutHeader+"."+curSignal->name+".na\t\t\t:= (Raw_"+curSignal->name+" > 16#FEFFFFFFFFFFFFFF);") : (""));

    }// TYPE CONVERTION AND E  NA CONTROL ENDS

    if((curSignal->convMethod != "BOOL:BOOL") && (curSignal->convMethod!="2BOOL:BOOL")){
            ST.append("\nGVL."+this->dutHeader+"."+curSignal->name+".RangeExcd\t:= NOT (("+cont_text+" >= "+QString::number(curSignal->minValue,'g',(curSignal->length>32)? 20:15)+") AND ("+cont_text+" <= "+QString::number(curSignal->maxValue,'g',(curSignal->length>32)? 20:15)+"));"
                  +((!this->enableFrc)?("\nGVL."+this->dutHeader+"."+curSignal->name+".v\t\t\t:= NOT( GVL."+dutHeader+((this->m_ComType=="ETH")?(".S_Com_Flt"):(".S_TmOut_"+nameMessage+"_0X"+idMessage))+" OR GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd "+((curSignal->isJ1939 ==true)?("OR GVL."+this->dutHeader+"."+curSignal->name+".e OR GVL."+this->dutHeader+"."+curSignal->name+".na) "):(") "))+";"):("\nGVL."+this->dutHeader+"."+curSignal->name+".v				:= NOT( GVL."+dutHeader+((this->m_ComType=="ETH")?(".S_Com_Flt"):(".S_TmOut_"+nameMessage+"_0X"+idMessage))+" OR GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd "+((curSignal->isJ1939 ==true)?("OR GVL."+this->dutHeader+"."+curSignal->name+".e OR GVL."+this->dutHeader+"."+curSignal->name+".na) OR "):(") OR "))+"FrcVar."+curSignal->name+".v ;"))+
            "\n");
            if(this->enableFrc){
                ST.append("GVL."+this->dutHeader+"."+curSignal->name+".x\t\t\t:= \n(*SELECT 1 : Is force active?*)\t\t\t\t\t\tSEL("+"FrcVar."+curSignal->name+".v"+","+"\n(*SELECT 2 : Is data valid ?*)\t\t\t\t\tSEL("+"GVL."+this->dutHeader+"."+curSignal->name+".v"+",(*DEFAULT value assignment*)\t\t\t\t\t\t"+QString::number(curSignal->defValue,'g',15)+",(*Transmission is VALID*)\t\t\t\t\t\t"+cont_text+"),(*Force is active*)\t\t\t\t\t\tFrcVar."+curSignal->name+".x);");
            }else{
                ST.append("GVL."+this->dutHeader+"."+curSignal->name+".x\t\t\t:= \n(*SELECT 1 : Is data valid ?*)\t\t\t\t\tSEL("+"GVL."+this->dutHeader+"."+curSignal->name+".v"+",\n(*DEFAULT value assigment*)\t\t\t\t\t\t"+QString::number(curSignal->defValue,'g',15)+",\n(*Transmission is VALID*)\t\t\t\t\t\t"+cont_text+");");
            }
    }


    ST.append("\n{endregion}");
    return ST;


}



///******************************************************************************
/// ETH IO GENERATION
///******************************************************************************
void DBCHandler::generateIOETH(QDomElement *pous, QDomDocument &doc)
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
    this->generateIOETHST(&STcode);
    //Generate local vars
    {
            QDomElement localVars= doc.createElement("localVars");
            /*Function block declaration*/
            foreach (dataContainer * curMessage , comInterface){
                if(curMessage->getIfSelected()){
            QDomElement variable=doc.createElement("variable");
            attr=doc.createAttribute("name");
            attr.setValue("MSG_ETH_"+curMessage->getID());
            variable.setAttributeNode(attr);
            QDomElement type = doc.createElement("type");
            QDomElement derived = doc.createElement("derived");
            attr=doc.createAttribute("name");
            attr.setValue(curMessage->getIfBitOperation()?("_FB_ETHTx_Message_Unpack"):("MSG_ETH_T"));
            derived.setAttributeNode(attr);
            type.appendChild(derived);
            variable.appendChild(type);
            localVars.appendChild(variable);
                }
            }

            //foreach (dataContainer * curMessage , comInterface){
            //    if(curMessage->getIfSelected()){
            //        for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
            //            QDomElement variable=doc.createElement("variable");
            //            attr=doc.createAttribute("name");
            //            attr.setValue("Cont_"+curSignal->name);
            //            variable.setAttributeNode(attr);
            //            QDomElement type = doc.createElement("type");
            //            QDomElement dataType = doc.createElement(curSignal->comDataType);
            //            type.appendChild(dataType);
            //            variable.appendChild(type);
            //            localVars.appendChild(variable);
            //        }
            //    }
            //}
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
            {
                QDomElement variable=doc.createElement("variable");
                attr=doc.createAttribute("name");
                attr.setValue("MessageCounter");
                variable.setAttributeNode(attr);
                QDomElement type = doc.createElement("type");
                QDomElement dataType = doc.createElement("UDINT");
                type.appendChild(dataType);
                variable.appendChild(type);
                localVars.appendChild(variable);
            }
            for(unsigned z=0;z<4;z++){
                QDomElement variable=doc.createElement("variable");
                attr=doc.createAttribute("name");
                attr.setValue("_FB_UNPACK_WORD_TO_BYTE_ID"+QString::number(z));
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

void DBCHandler::createGVLINFO()
{
    QFile logFile(this->folderLoc+"/"+(this->dutHeader)+"_GVLETH_README.txt");
    if (!logFile.open(QIODevice::WriteOnly| QIODevice::Truncate )){
            setErrCode("GVL kaydı oluşturulmadı.");
    }else{
            QTextStream out(&logFile);
            out<<"Aşağıdaki kodu GVL'e kopyalayabilirsiniz."<<Qt::endl;
            out<<this->dutHeader+" : "+this->dutName+";"<<Qt::endl;
            out<<"Datagram_"+this->dutHeader+" : DATAGRAM40_T;"<<Qt::endl;

            dataContainer::setWarning("INFO","GVL önerisi çıktı dosya konumunda oluşturuldu.");
            logFile.close();
    }
}
void DBCHandler::generateIOETHST(QString * const ST)
{       ST->append("(*\n"
               "**********************************************************************\n"
               "Bozankaya A.Ş.\n"
               "**********************************************************************\n"
               "Name					: P_"+dutHeader+"\n"
               "POU Type				: Program\n"
               "Created by              : COMMUNICATION INTERFACE GENERATOR "+Version+"("+QHostInfo::localHostName()+"), BZK.\n"
               "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
               "Modifications			: see version description below\n"
               "\n"
               "\n"
               "Program Description:"
               "This program which is created by automatically by communication interface generator \n handles the TX (OUTPUT) ethernet messages as custom UDP 40 byte datagram packs \n"
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
                ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
                ST->append("\n//MESSAGE------");
                ST->append("\n{region \" User Area : "+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
                for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
            ST->append("\n//-------SIGNAL -> "+curSignal->name+" Max: "+QString::number(curSignal->maxValue)+" Min: "+QString::number(curSignal->minValue)+" Def: "+QString::number(curSignal->defValue)+" Resolution: "+QString::number(curSignal->resolution)+" Offset: "+QString::number(curSignal->offset));
            ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".v\t\t\t\t\t:= "+((this->enableTest==true)?("FrcTest."+curSignal->name+".v;"):("ASSIGN HERE VALIDITY VARIABLE !!;")));
            ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".x\t\t\t\t\t:= "+((this->enableTest==true)?("FrcTest."+curSignal->name+".x;"):("ASSIGN HERE VALIDITY VARIABLE !!;")));
            if(curSignal->isJ1939){
                    ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".e\t\t\t\t\t:= "+((this->enableTest==true)?("FrcTest."+curSignal->name+".e;"):("ASSIGN HERE ERROR FLAG VARIABLE !!;")));
                    ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".na\t\t\t\t\t:= "+((this->enableTest==true)?("FrcTest."+curSignal->name+".na;"):("ASSIGN HERE NOT AVAILABLE FLAG VARIABLE !!;")));
            }
            if(curSignal->length > 3){
                    ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".RangeExcd\t\t\t:=  ("+QString::number(curSignal->maxValue,'g',(curSignal->length>32)? 20:15)+" < GVL."+this->dutHeader+"."+curSignal->name+".x )OR ("+QString::number(curSignal->minValue,'g',(curSignal->length>32)? 20:15)+" > GVL."+this->dutHeader+"."+curSignal->name+".x);");
            }

                }
                ST->append("\n{endregion}");
                ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
            }
    }
    foreach (dataContainer * curMessage , comInterface){
            if(curMessage->getIfSelected()){
                QString nameFb = "MSG_ETH_"+curMessage->getID();
                if(curMessage->getIfBitOperation()){
                    ST->append( "\n"+nameFb+"();\n");
                }
            }
    }

    foreach (dataContainer * curMessage , comInterface){
            if(curMessage->getIfSelected()){
                ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
                ST->append("\n{region \" MESSAGE AREA :"+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
                QString nameFb = "MSG_ETH_"+curMessage->getID();

                for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                    ST->append(convTypeApptoCom(curSignal,curMessage->getID(),curMessage->getName(),nameFb));
                }


                ST->append("\n{endregion}");
                ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
            }
    }
    ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
    ST->append("\n{region \"Selection Area\"}\n");
    ST->append(this->generateIOETHDatagramST());
    ST->append("\n{endregion}");
    ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
}

QString DBCHandler::generateIOETHDatagramST()
{
    QString ST;
    long counter=0;
    long quartercounter=0;
    QList<QString> IDlist;
    IDlist.clear();
    for(unsigned i= 0 ;i<4;i++){
        QString datagramID ="ID"+QString::number(i);
        ST.append("_FB_UNPACK_WORD_TO_BYTE_"+datagramID+"();\n");
    }

    ST.append("CASE MessageCounter OF\n");
    foreach (dataContainer * curMessage , comInterface){
            if(curMessage->getIfSelected()){
                quartercounter++;

                IDlist.append(curMessage->getID());
                if(quartercounter==4){
                    counter++;
                    ST.append(QString::number(counter)+":\n");
                    unsigned datagID = 0;
                    for(QString ID : IDlist){
                    QString datagramID ="ID"+QString::number(datagID);
                    ST.append("_FB_UNPACK_WORD_TO_BYTE_"+datagramID+".X_Word_0 := 16#"+ID+";\n");
                        for(unsigned k=0 ;k<8;k++){

                    ST.append("GVL.Datagram_"+dutHeader+"."+datagramID+"_Byte_"+QString::number(k)+" := "+"MSG_ETH_"+ID+"."+((comInterface[ID]->getIfBitOperation())?("O"):("X"))+"_Byte_"+QString::number(k)+";\n");
                        }
                        datagID++;
                    }
                    quartercounter=0;
                    IDlist.clear();
                }
            }
    }
    if(quartercounter !=0){
            unsigned needCounter = quartercounter;
            unsigned completer = 4 - needCounter;
            unsigned datagID = 0;
            counter++;
            ST.append(QString::number(counter)+":\n");
            for(QString ID : IDlist){
                QString datagramID ="ID"+QString::number(datagID);
                ST.append("_FB_UNPACK_WORD_TO_BYTE_"+datagramID+".X_Word_0 := 16#"+ID+";\n");
                for(unsigned k=0 ;k<8;k++){

                    ST.append("GVL.Datagram_"+dutHeader+"."+datagramID+"_Byte_"+QString::number(k)+" := "+"MSG_ETH_"+ID+"."+((comInterface[ID]->getIfBitOperation())?("O"):("X"))+"_Byte_"+QString::number(k)+";\n");
                }
                datagID++;
            }
            for(unsigned i = completer; i>0;i--){
                QString datagramID ="ID"+QString::number(datagID);
                ST.append("_FB_UNPACK_WORD_TO_BYTE_"+datagramID+".X_Word_0 := 0;\n");
                for(unsigned k=0 ;k<8;k++){
                ST.append("GVL.Datagram_"+dutHeader+"."+datagramID+"_Byte_"+QString::number(k)+" := 0;\n");
                }
                datagID++;
            }
            IDlist.clear();
    }
    ST.append("END_CASE;\n");
    for(unsigned i= 0 ;i<4;i++){
            QString datagramID ="ID"+QString::number(i);
            ST.append("GVL.Datagram_"+dutHeader+"."+datagramID+"_LS :=_FB_UNPACK_WORD_TO_BYTE_"+datagramID+"."+"X_Byte_0;\n");
            ST.append("GVL.Datagram_"+dutHeader+"."+datagramID+"_HS :=_FB_UNPACK_WORD_TO_BYTE_"+datagramID+"."+"X_Byte_1;\n");
    }
    ST.append("IF MessageCounter ="+QString::number(counter)+"THEN\n");
    ST.append("\t MessageCounter:=0 ; \n");
    ST.append("ELSE\n");
    ST.append("\t MessageCounter:=MessageCounter+1 ; \n");
    ST.append("END_IF;\n");
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

            //foreach (dataContainer * curMessage , comInterface){
            //    if(curMessage->getIfSelected()){
            //        for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
            //            QDomElement variable=doc.createElement("variable");
            //            attr=doc.createAttribute("name");
            //            attr.setValue("Cont_"+curSignal->name);
            //            variable.setAttributeNode(attr);
            //            QDomElement type = doc.createElement("type");
            //            QDomElement dataType = doc.createElement(curSignal->comDataType);
            //            type.appendChild(dataType);
            //            variable.appendChild(type);
            //            localVars.appendChild(variable);
            //        }
            //    }
            //}
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
                   "Created by              : COMMUNICATION INTERFACE GENERATOR "+Version+"("+QHostInfo::localHostName()+"), BZK.\n"
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
                 ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
                 ST->append("\n//MESSAGE------");
                 ST->append("\n{region \" User Area : "+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
                for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                        ST->append("\n//-------SIGNAL -> "+curSignal->name+" Max: "+QString::number(curSignal->maxValue)+" Min: "+QString::number(curSignal->minValue)+" Def: "+QString::number(curSignal->defValue)+" Resolution: "+QString::number(curSignal->resolution)+" Offset: "+QString::number(curSignal->offset));
                 ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".v\t\t\t\t\t:= "+((this->enableTest==true)?("FrcTest."+curSignal->name+".v;"):("ASSIGN HERE VALIDITY VARIABLE !!;")));
                 ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".x\t\t\t\t\t:= "+((this->enableTest==true)?("FrcTest."+curSignal->name+".x;"):("ASSIGN HERE VALIDITY VARIABLE !!;")));
                 if(curSignal->isJ1939){
                    ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".e\t\t\t\t\t:= "+((this->enableTest==true)?("FrcTest."+curSignal->name+".e;"):("ASSIGN HERE ERROR FLAG VARIABLE !!;")));
                    ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".na\t\t\t\t\t:= "+((this->enableTest==true)?("FrcTest."+curSignal->name+".na;"):("ASSIGN HERE NOT AVAILABLE FLAG VARIABLE !!;")));
                 }
                 if(curSignal->length > 3){
                    ST->append("\nGVL."+this->dutHeader+"."+curSignal->name+".RangeExcd\t\t\t:=  ("+QString::number(curSignal->maxValue,'g',(curSignal->length>32)? 20:15)+" < GVL."+this->dutHeader+"."+curSignal->name+".x )OR ("+QString::number(curSignal->minValue,'g',(curSignal->length>32)? 20:15)+" > GVL."+this->dutHeader+"."+curSignal->name+".x);");
                    }

                 }
                ST->append("\n{endregion}");
                ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
            }
        }
    foreach (dataContainer * curMessage , comInterface){
        if(curMessage->getIfSelected()){
            ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
            ST->append("\n{region \" MESSAGE AREA :"+curMessage->getName()+"- ID:"+curMessage->getID()+"\"}\n");
            QString nameFb = (curMessage->getIfBitOperation())? ("_FB_CanTx_Message_Unpack_"+curMessage->getID()) : ("_FB_CanTx_Message_"+curMessage->getID());
            bool arr[64] ={false};
            for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){

                ST->append(convTypeApptoCom(curSignal,curMessage->getID(),curMessage->getName(),nameFb));
                for(unsigned i=curSignal->startBit ; i<(curSignal->startBit + curSignal->length) ; i++){
                    arr[i]=true;
                }
                
            }
			for(unsigned i =0 ; i<8 ; i++){
                    bool flag = false;
                    for(unsigned k=0 ; k<8 ; k++){
                        if(arr[(i*8)+k]==true){
                            flag=true;
                        }
                    }
                    if(!flag){
                        ST->append("\n"+nameFb+".X_Byte_"+QString::number(i)+":=16#FF;");
                    }
                }
            ST->append("\n"+nameFb+"("
                                     "\n     C_Enable:= "+((this->enableMultiEnable)?("GVL."+dutHeader+".C_En_"+curMessage->getName()+"_0X"+curMessage->getID()):("GVL."+dutHeader+".S_CAN_Init"))+","
                                     "\n     Obj_CAN:= ADR("+this->canLine+"),"
                                     "\n     X_MsgID:= 16#"+curMessage->getID()+","
                                     "\n     Tm_CycleTime:= TIME#"+curMessage->getMsCycleTime()+"ms,"
                                     "\n     N_Msg_Dlc:="+QString::number(curMessage->getDLC())+","
                                     "\n     S_Extended:= "+QString::number(curMessage->getIfExtended()).toUpper()+","
                                     "\n     S_Sent_Ok   => GVL."+dutHeader+".S_SentOk_"+curMessage->getName()+"_0X"+curMessage->getID()+");");

            ST->append("\n{endregion}");
            ST->append("\n//-----------------------------------------------------------------------------------------------------------------------------");
        }
    }

}
QString DBCHandler::convTypeApptoCom (const dataContainer::signal *curSignal, QString idMessage, QString nameMessage,  QString nameFb)
{
    QString ST="\n{region \" SIGNAL AREA : NAME->"+curSignal->name+" LENGTH->"+QString::number(curSignal->length)+" STARTBIT :"+QString::number(curSignal->startBit)+"\"}";
    bool flagConversion =false;
    QString stat1,stat2,stat3,stat4,stat5 = "";

        // TYPE CONVERTION AND E  NA CONTROL STARTS
        if ((curSignal->length==4)){
            if((curSignal->convMethod == "toBYTE")|| (curSignal->convMethod == "xtoBYTE")){

                stat1 = "FrcVar."+curSignal->name+".x";
                stat2 = "GVL."+this->dutHeader+"."+curSignal->name+".x";
                stat3 = "16#F";
                stat4 = "16#E";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toUSINT")|| (curSignal->convMethod == "xtoUSINT")){

                stat1 = "USINT_TO_BYTE(FrcVar."+curSignal->name+".x)";
                stat2 = "USINT_TO_BYTE(GVL."+this->dutHeader+"."+curSignal->name+".x )";
                stat3 = "16#F";
                stat4 = "16#E";
                stat5 = QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15);
                flagConversion=true;
            }
            else if ((curSignal->convMethod == "toREAL")|| (curSignal->convMethod == "xtoREAL")){

                stat1 = "USINT_TO_BYTE(REAL_TO_USINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + ("+QString::number((-1*curSignal->offset),'g',15)+")"))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat2 = "USINT_TO_BYTE(REAL_TO_USINT((GVL."+this->dutHeader+"."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat3 = "16#F";
                stat4 = "16#E";
                stat5 = (((curSignal->offset == 0) && (curSignal->defValue==0))? (" 0 "): ("USINT_TO_BYTE(REAL_TO_USINT(("+((curSignal->defValue==0) ? "" : (QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15)))+(((!(curSignal->offset == 0) && !(curSignal->defValue==0))?("+"):(""))+((curSignal->offset ==0 )? (""): (QString::number((-1*curSignal->offset),'g',15))))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))"));
                flagConversion=true;
            }
        }else if((curSignal->length<9)){
            if((curSignal->convMethod == "toBYTE")|| (curSignal->convMethod == "xtoBYTE")){

                stat1 = "FrcVar."+curSignal->name+".x";
                stat2 = "GVL."+this->dutHeader+"."+curSignal->name+".x";
                stat3 = "16#F";
                stat4 = "16#E";
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

                stat1 = "USINT_TO_BYTE(REAL_TO_USINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + ("+QString::number((-1*curSignal->offset),'g',15)+")"))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
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

                stat1 = "UINT_TO_WORD(REAL_TO_UINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + ("+QString::number((-1*curSignal->offset),'g',15)+")"))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
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

                stat1 = "UDINT_TO_DWORD(REAL_TO_UDINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + ("+QString::number((-1*curSignal->offset),'g',15)+")"))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
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

                stat1 = "ULINT_TO_LWORD(LREAL_TO_ULINT((FrcVar."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + ("+QString::number((-1*curSignal->offset),'g',15)+")"))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat2 = "ULINT_TO_LWORD(LREAL_TO_ULINT((GVL."+this->dutHeader+"."+curSignal->name+".x"+((curSignal->offset ==0 )? (""): (" + "+QString::number((-1*curSignal->offset),'g',15)))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))";
                stat3 = "16#FFFFFFFFFFFFFFFF";
                stat4 = "16#FEFEFEFEFEFEFEFE";
                stat5 = (((curSignal->offset == 0) && (curSignal->defValue==0))? (" 0 "): ("ULINT_TO_LWORD(LREAL_TO_ULINT(("+((curSignal->defValue==0) ? "" : (QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15)))+(((!(curSignal->offset == 0) && !(curSignal->defValue==0))?("+"):(""))+((curSignal->offset ==0 )? (""): (QString::number((-1*curSignal->offset),'g',15))))+")"+((curSignal->resolution == 1)?(""):("/"+QString::number(curSignal->resolution,'g',15)))+"))"));
                flagConversion=true;
            }

        }
        QString cont_text="";
        if (flagConversion){

            if ((this->enableFrc) && (curSignal->isJ1939)){
                cont_text.append(
                "\n(*SELECT 1 : Is force active?*)\t\t\t\t\t\t\tSEL(FrcVar."+curSignal->name+".v,\n(*SELECT 2 : Is data valid and range is not exceed?*)\t\t\t\t\t\tSEL((GVL."+this->dutHeader+"."+curSignal->name+".v AND NOT GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd),\n(*SELECT 3 : Is NOT AVAILABLE ?*)\t\t\t\t\t\tSEL(GVL."+this->dutHeader+"."+curSignal->name+".na,\n(*SELECT 4 : Is ERROR active ?*)\t\t\t\t\t\tSEL(GVL."+this->dutHeader+"."+curSignal->name+".e,\n(*DEFAULT VALUE assignment*)\t\t\t\t\t\t\t\t"+stat5+",\n(*ERROR is active*)\t\t\t\t\t\t\t\t\t\t"+stat4+"),\n(*NOT AVAILABLE is active*)\t\t\t\t\t\t\t\t"+stat3+"),\n(*Transmission is VALID*)\t\t\t\t\t\t\t\t"+stat2+"),\n(*FORCE is active*)\t\t\t\t\t\t\t\t\t\t"+stat1+")"
                );
            }else if((!this->enableFrc) && (curSignal->isJ1939)){
                cont_text.append(
                "\n(*SELECT 1 : Is data valid and range is not exceed?*)\tSEL((GVL."+this->dutHeader+"."+curSignal->name+".v AND NOT GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd),\n(*SELECT 2 : Is NOT AVAILABLE ?*)\t\t\t\t\t\tSEL(GVL."+this->dutHeader+"."+curSignal->name+".na,\n(*SELECT 3 : Is ERROR active ?*)\t\t\t\t\t\tSEL(GVL."+this->dutHeader+"."+curSignal->name+".e,\n(*DEFAULT VALUE assignment*)\t\t\t\t\t\t\t"+stat5+",\n(*ERROR is active*)\t\t\t\t\t\t\t\t\t\t"+stat4+"),\n(*NOT AVAILABLE is active*)\t\t\t\t\t\t\t\t"+stat3+"),\n(*Transmission is VALID*)\t\t\t\t\t\t\t\t"+stat2+")"
                );
            }else if((this->enableFrc) && (!curSignal->isJ1939)){
                cont_text.append(
                "\n(*SELECT 1 : Is force active?*)\t\t\t\t\t\t\tSEL(FrcVar."+curSignal->name+".v,\n(*SELECT 1 : Is data valid and range is not exceed?*)\tSEL((GVL."+this->dutHeader+"."+curSignal->name+".v AND NOT GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd),\n(*DEFAULT VALUE assignment*)\t\t\t\t\t\t\t"+stat5+",\n(*Transmission is VALID*)\t\t\t\t\t\t\t\t"+stat2+"),\n(*FORCE is active*)\t\t\t\t\t\t\t\t\t\t"+stat1+")"
                );
            }else{
                cont_text.append(
                "\n(*SELECT 1 : Is force active?*)\t\t\t\t\t\t\tSEL((GVL."+this->dutHeader+"."+curSignal->name+".v AND NOT GVL."+this->dutHeader+"."+curSignal->name+".RangeExcd),\n(*DEFAULT VALUE assignment*)\t\t\t\t\t\t\t"+stat5+",\n(*FORCE is active*)\t\t\t\t\t\t\t\t\t\t"+stat2+")"
                );
            }
        }

                if(curSignal->convMethod=="BOOL:BOOL"){
                    ST.append("\n"+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+"\t\t:=	GVL."+this->dutHeader+"."+curSignal->name+".x AND GVL."+this->dutHeader+"."+curSignal->name+".v ;");
                }else if(curSignal->convMethod=="2BOOL:BOOL"){
                    ST.append("\n"+nameFb+".S_Bit_"+QString::number(curSignal->startBit)+"\t\t:=	 (GVL."+this->dutHeader+"."+curSignal->name+".x OR GVL."+this->dutHeader+"."+curSignal->name+".na) AND NOT GVL."+this->dutHeader+"."+curSignal->name+".e; "
                              "\n"+nameFb+".S_Bit_"+QString::number(curSignal->startBit+1)+"\t\t:=	 NOT GVL."+this->dutHeader+"."+curSignal->name+".v OR GVL."+this->dutHeader+"."+curSignal->name+".na OR GVL."+this->dutHeader+"."+curSignal->name+".e; ");
                }else if((curSignal->convMethod=="xtoBYTE")||(curSignal->convMethod=="xtoUSINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==8))){
                    ST.append("\n"+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+"\t\t:="+cont_text+";");
                }else if((curSignal->convMethod=="xtoWORD")||(curSignal->convMethod=="xtoUINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==16))){
                    ST.append("\n_FB_UNPACK_WORD_TO_BYTE_"+QString::number(counterfbWORDTOBYTE)+"(\n\t\tX_WORD_0:= "+cont_text+",\n\t\tX_BYTE_0=> "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+",\n\t\tX_BYTE_1=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+");");
                    counterfbWORDTOBYTE++;
                }else if((curSignal->convMethod=="xtoDWORD")||(curSignal->convMethod=="xtoUDINT")||((curSignal->convMethod=="xtoREAL") && (curSignal->length==32))){
                    ST.append("\n_FB_UNPACK_DWORD_TO_BYTE_"+QString::number(counterfbDWORDTOBYTE)+"(\n\t\tX_DWORD_0:= "+cont_text+",\n\t\tX_BYTE_0=> "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+",\n\t\tX_BYTE_1=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+",\n\t\tX_BYTE_2=>"+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+2)+",\n\t\tX_BYTE_3=>"+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+3)+");");
                    counterfbDWORDTOBYTE++;
                }else if((curSignal->convMethod=="xtoLWORD")||(curSignal->convMethod=="xtoULINT")||(curSignal->convMethod=="xtoLREAL") ){
                    ST.append("\n_FB_UNPACK_LWORD_TO_BYTE_"+QString::number(counterfbLWORDTOBYTE)+"(\n\t\tX_LWORD_0:= "+cont_text+",\n\t\tX_BYTE_0=> "+nameFb+".X_Byte_"+QString::number(curSignal->startBit/8)+",\n\t\tX_BYTE_1=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+1)+",\n\t\tX_BYTE_2=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+2)+",\n\t\tX_BYTE_3=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+3)+",\n\t\tX_BYTE_4=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+4)+",\n\t\tX_BYTE_5=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+5)+",\n\t\tX_BYTE_6=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+6)+",\n\t\tX_BYTE_7=> "+nameFb+".X_Byte_"+QString::number((curSignal->startBit/8)+7)+");");
                    counterfbLWORDTOBYTE++;

                }else{
                    bool flagPack = false;
                    if((curSignal->length>8)){
                        if(curSignal->length<17){
                        ST.append("\n_FB_UNPACK_WORD_TO_BYTE_"+QString::number(counterfbWORDTOBYTE)+"(\n\t\tX_WORD_0:="+cont_text+");");
                        counterfbWORDTOBYTE++;
                        }else if(curSignal->length <33){
                         ST.append("_FB_UNPACK_DWORD_TO_BYTE_"+QString::number(counterfbDWORDTOBYTE)+"(\n\t\tX_DWORD_0:="+cont_text+");");
                         counterfbDWORDTOBYTE++;
                        }else {
                          ST.append("\n_FB_UNPACK_LWORD_TO_BYTE_"+QString::number(counterfbLWORDTOBYTE)+"(\n\t\tX_LWORD_0:="+cont_text+");");
                          counterfbLWORDTOBYTE++;

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
                                ST.append("\n\t\tS_Bit_"+QString::number(i%8)+"=> "+nameFb+".S_Bit_"+QString::number(curSignal->startBit+i)+"");
                                if(((i%8 != 7) &&(i%(curSignal->length-1) != 0)) || (i==0)) {
                                    ST.append(",");
                                }
                        }
                        if(flagPack){
                            if(((i%8 == 7)||(i%(curSignal->length-1) == 0) )&& (i>0)){
                                ST.append(" ,\n\t\tX_BYTE_0:=");
                                if(curSignal->length<9){
                                ST.append(""+cont_text);
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

        ST.append("\n\n{endregion}\n");
        return ST;

}

