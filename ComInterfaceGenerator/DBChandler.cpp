#include "DBChandler.h"
#include "datacontainer.h"
#include "qforeach.h"
#include <QtGlobal>
#include <QRegularExpression>
#include <QUuid>
#include <QHostInfo>

unsigned long long DBCHandler::selectedMessageCounter = 0;
unsigned DBCHandler::counterfbBYTETOWORD = 0;
unsigned DBCHandler::counterfbBYTETODWORD = 0;
unsigned DBCHandler::counterfbBYTETOLWORD = 0;
unsigned DBCHandler::counterfb8BITTOBYTE = 0;
unsigned DBCHandler::counterfbDWORDTOLWORD = 0;
unsigned DBCHandler::counterfbLWORDTOBYTE = 0;
unsigned DBCHandler::counterfbDWORDTOBYTE = 0;
unsigned DBCHandler::counterfbWORDTOBYTE = 0;
unsigned DBCHandler::counterfbBYTETO8BIT = 0;
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

    this->canLine = "CAN0";
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
    qInfo()<< ((this->enableTest)?"Test mode on":"Test mode off");
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
            addSignalToMessage (messageID,curSignal);
 /*Append and manupulate message comments*/
/************************************************/
        }else if((curLine.contains("CM_"))&& curLine.contains("BO_")){

            QStringList messageList = curLine.split(" ");
            QString commentContainer =  getBetween("\"","\";",curLine);
            QString configComment = getBetween("[*","*]",curLine);
            QString ID ="";
            if(messageList.at(2).toUInt()>2047){
                ID = QString::number((messageList.at(2).toUInt())-2147483648,16).toUpper();
            }else{
                ID = QString::number(messageList.at(2).toUInt(),16).toUpper();
            }
            QString msTimeout="";
            QString msCycleTime="";
            if(configComment.contains("timeout",Qt::CaseInsensitive)){
                msTimeout = this->getBetween("timeout","ms",configComment);
            }else{
                dataContainer::setWarning(ID,"Mesaj için timeout hatalı yazılmış, <timeout : XXXX ms> etiketiyle yoruma ekleyin.");
            }
            if(configComment.contains("cycletime",Qt::CaseInsensitive)){
                msCycleTime = this->getBetween("cycletime","ms",configComment);
            }else{
                dataContainer::setWarning(ID,"Mesaj için cycletime hatalı yazılmış, <cycletime : XXXX ms> etiketiyle yoruma ekleyin.");
            }
            qInfo()<<configComment;
            msgCommentList.append({ID,msTimeout,msCycleTime,commentContainer});
            if (configComment.contains("j1939",Qt::CaseInsensitive)){
                      for( dataContainer::signal * curSignal : *comInterface.value(ID)->getSignalList()){
                        curSignal->isJ1939 = true;
                      }
            }
        }else if((curLine.contains("CM_"))&& curLine.contains("SG_")){

            QString commentContainer = getBetween("\"","\";",curLine);
            QString configComment = getBetween("[*","*]",curLine);
            QStringList commentLine = curLine.split(" ");
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
    QString name,id;

    foreach(dataContainer *const curValue , comInterface){
        if(curValue->getIfSelected()){
            name = "_FB_"+this->dutHeader+"_"+curValue->getName()+"_0X"+curValue->getID();
            id = getRandomID();
            this->fbNameandObjId.append({name,id});
        }
    }
    if(this->IOType == "IO"){
        name = "_FB_"+this->dutHeader+"_NA_Handler";
        this->fbNameandObjId.append({name,getRandomID()});
        name = "_FB_"+this->dutHeader+"_ERR_Handler";
        this->fbNameandObjId.append({name,getRandomID()});
        name = "_FB_"+this->dutHeader+"_Validity_Handler";
        this->fbNameandObjId.append({name,getRandomID()});
        name = "_FB_"+this->dutHeader+"_Output_Handler";
        this->fbNameandObjId.append({name,getRandomID()});
    }
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
        if(!curMessage->isTmOutSet){
            dataContainer::setWarning(curMessage->getID(),"mesajı için timeout belirtilmediği için 2500ms atandı");
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
            this->generateIIPous(&pous,doc);
            setProgress(40);
            this->generatePouFpd(&pous,doc);
            setProgress(60);
        }
        else{
            this->generateHandlers(&pous,doc);
            setProgress(40);
            this->generateIOPous(&pous,doc);
            setProgress(50);
            this->generatePouFpd(&pous,doc);
            setProgress(60);
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
        QDomElement folder1 = doc.createElement("Folder");
        attr= doc.createAttribute("Name");
        attr.setValue("FD_Library_Application"); // Folder name for Function blocks
        folder1.setAttributeNode(attr);
        QDomElement folder2 = doc.createElement("Folder");
        attr= doc.createAttribute("Name");
        attr.setValue("_FB_"+this->dutHeader+"_MessageBlocks"); // Folder name for Function blocks
        folder2.setAttributeNode(attr);
        folder1.appendChild(folder2);

        for(QList<QString> const &data : fbNameandObjId ){
            QDomElement object = doc.createElement("Object");
            attr=doc.createAttribute("Name");
            attr.setValue(data.at(0));
            object.setAttributeNode(attr);
            attr=doc.createAttribute("ObjectId");
            attr.setValue(data.at(1));
            object.setAttributeNode(attr);
            folder2.appendChild(object);
        }
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
                    if((curSignal->appDataType)!="BOOL"){
                        QDomElement value = doc.createElement("value");
                        attr = doc.createAttribute("member");
                        attr.setValue("Param_Max");
                        value.setAttributeNode(attr);
                        QDomElement simpleValue = doc.createElement("simpleValue");
                        attr = doc.createAttribute("value");
                        attr.setValue(QString::number(curSignal->maxValue,'g',(curSignal->length>32)? 20:15));
                        simpleValue.setAttributeNode(attr);
                        value.appendChild(simpleValue);
                        structValue.appendChild(value);
                    }
                    if((curSignal->appDataType)!="BOOL"){
                        QDomElement value = doc.createElement("value");
                        attr = doc.createAttribute("member");
                        attr.setValue("Param_Min");
                        value.setAttributeNode(attr);
                        QDomElement simpleValue = doc.createElement("simpleValue");
                        attr = doc.createAttribute("value");
                        attr.setValue(QString::number(curSignal->minValue,'g',(curSignal->length>32)? 20:15));
                        simpleValue.setAttributeNode(attr);
                        value.appendChild(simpleValue);
                        structValue.appendChild(value);
                    }
                    if(((curSignal->appDataType)=="REAL")||((curSignal->appDataType)=="LREAL")){
                        QDomElement value = doc.createElement("value");
                        attr = doc.createAttribute("member");
                        attr.setValue("Param_Res");
                        value.setAttributeNode(attr);
                        QDomElement simpleValue = doc.createElement("simpleValue");
                        attr = doc.createAttribute("value");
                        attr.setValue(QString::number(curSignal->resolution));
                        simpleValue.setAttributeNode(attr);
                        value.appendChild(simpleValue);
                        structValue.appendChild(value);
                    }
                    if(((curSignal->appDataType)=="REAL")||((curSignal->appDataType)=="LREAL")){
                        QDomElement value = doc.createElement("value");
                        attr = doc.createAttribute("member");
                        attr.setValue("Param_Off");
                        value.setAttributeNode(attr);
                        QDomElement simpleValue = doc.createElement("simpleValue");
                        attr = doc.createAttribute("value");
                        attr.setValue(QString::number(curSignal->offset));
                        simpleValue.setAttributeNode(attr);
                        value.appendChild(simpleValue);
                        structValue.appendChild(value);
                    }
                    if((curSignal->appDataType)!="BOOL"){
                        QDomElement value = doc.createElement("value");
                        attr = doc.createAttribute("member");
                        attr.setValue("Param_Def");
                        value.setAttributeNode(attr);
                        QDomElement simpleValue = doc.createElement("simpleValue");
                        attr = doc.createAttribute("value");
                        attr.setValue(QString::number(curSignal->defValue,'g',(curSignal->length>32)? 20:15));
                        simpleValue.setAttributeNode(attr);
                        value.appendChild(simpleValue);
                        structValue.appendChild(value);
                    }
                    {
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
     //For AND and OR gate to manipulate timeout and disturbance flags
    structFbdBlock* gateAND = new structFbdBlock;
    structFbdBlock* gateOR = new structFbdBlock;
    unsigned short counterInANDOR = 1;
    gateAND->name="AND";
    gateOR->name="OR";

    foreach (dataContainer * curMessage , comInterface){
        if(curMessage->getIfSelected()){

            /*generate block struct type new block*/

            structFbdBlock* newBlock = new structFbdBlock;
            QDomElement pou = doc.createElement("pou");
            /*set pou name*/
            attr=doc.createAttribute("name");
            attr.setValue("_FB_"+this->dutHeader+"_"+curMessage->getName()+"_0X"+curMessage->getID());
            newBlock->name="_FB_"+this->dutHeader+"_"+curMessage->getName()+"_0X"+curMessage->getID();
            pou.setAttributeNode(attr);
            /*set pouType*/
            attr=doc.createAttribute("pouType");
            attr.setValue("functionBlock");
            pou.setAttributeNode(attr);

            /*Interface*/
            QDomElement interface = doc.createElement("interface");
            {
                QDomElement inputVars = doc.createElement("inputVars");

                /*Generate Input Variables - inputVars*/
                /*C_Init_Can*/
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("C_Init_Can");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement BOOL = doc.createElement("BOOL");
                    type.appendChild(BOOL);
                    variable.appendChild(type);
                    inputVars.appendChild(variable);
                    newBlock->inputVars.append({"C_Init_Can","GVL."+dutHeader+".S_CAN_Init","BOOL"," "});
                }
                /*Ptr_Obj_Can*/
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("Ptr_Obj_Can");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement pointer = doc.createElement("pointer");
                    QDomElement baseType = doc.createElement("baseType");
                    QDomElement derived =doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue("tCan");
                    derived.setAttributeNode(attr);
                    baseType.appendChild(derived);
                    pointer.appendChild(baseType);
                    type.appendChild(pointer);
                    variable.appendChild(type);
                    inputVars.appendChild(variable);
                    newBlock->inputVars.append({"Ptr_Obj_Can","ADR("+canLine+")","POINTER TO tCan"," "});

                }
                /*Start to generate variables*/
                for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                    /*Bool signals has FrcHi and FrcLo, others FrcEn FrcVal*/
                    if(curSignal->appDataType != "BOOL"){
                        /*Create FrcEn */
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("FrcEn_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"FrcEn_"+curSignal->name,"FrcHi."+curSignal->name+".v","BOOL"," "});
                        /*Create FrcVal */
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("FrcVal_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement signalDataType = doc.createElement(curSignal->appDataType);
                        type.appendChild(signalDataType);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"FrcVal_"+curSignal->name,"FrcHi."+curSignal->name+".x",curSignal->comDataType," "});
                    }
                    else {
                        /*Create FrcHi */
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("FrcHi_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"FrcHi_"+curSignal->name,"FrcHi."+curSignal->name+".x","BOOL"," "});
                        /*Create FrcLo */
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("FrcLo_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"FrcLo_"+curSignal->name,"FrcLo."+curSignal->name+".x","BOOL"," "});
                    }

                }
                interface.appendChild(inputVars);
            }
            /*Generate Output Variables - outputVars*/
            {
                QDomElement outputVars = doc.createElement("outputVars");;
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("S_Msg_TmOut");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement BOOL = doc.createElement("BOOL");
                    type.appendChild(BOOL);
                    variable.appendChild(type);
                    outputVars.appendChild(variable);
                    newBlock->outputVars.append({"S_Msg_TmOut","GVL."+dutHeader+".S_TmOut_"+curMessage->getName()+"_0X"+curMessage->getID(),"BOOL"," "});
                }
                interface.appendChild(outputVars);
            }
            /*Generate Output Input Variables - inoutVars*/
            {
                QDomElement inoutVars = doc.createElement("inOutVars");
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue(dutHeader);
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue(dutName);
                    dataVarType.setAttributeNode(attr);
                    type.appendChild(dataVarType);
                    variable.appendChild(type);
                    inoutVars.appendChild(variable);
                }
                interface.appendChild(inoutVars);
            }
            /*Generate Local Variables - inoutVars*/
            QString STcode;
            this->generateIIST(&STcode,curMessage);
            {
                QDomElement localVars= doc.createElement("localVars");
                /*Function block declaration*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_CanRx_Message_Unpack_0");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_CanRx_Message_Unpack");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }
                /*P_ID_Can*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("P_ID_Can");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("UDINT");
                    type.appendChild(dataVarType);
                    QDomElement initialValue = doc.createElement("initialValue");
                    QDomElement simpleValue = doc.createElement("simpleValue");
                    attr=doc.createAttribute("value");
                    attr.setValue("16#"+curMessage->getID());
                    simpleValue.setAttributeNode(attr);
                    initialValue.appendChild(simpleValue);
                    variable.appendChild(type);
                    variable.appendChild(initialValue);
                    localVars.appendChild(variable);
                }
                /*P_Tm_MsgTmOut*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("P_Tm_MsgTmOut");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("TIME");
                    type.appendChild(dataVarType);
                    QDomElement initialValue = doc.createElement("initialValue");
                    QDomElement simpleValue = doc.createElement("simpleValue");
                    attr=doc.createAttribute("value");
                    attr.setValue("TIME#"+curMessage->getMsTimeOut()+"ms");
                    simpleValue.setAttributeNode(attr);
                    initialValue.appendChild(simpleValue);
                    variable.appendChild(type);
                    variable.appendChild(initialValue);
                    localVars.appendChild(variable);
                }
                /*P_Msg_Extd*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("P_Msg_Extd");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("BOOL");
                    type.appendChild(dataVarType);
                    QDomElement initialValue = doc.createElement("initialValue");
                    QDomElement simpleValue = doc.createElement("simpleValue");
                    attr=doc.createAttribute("value");
                    attr.setValue(QString::number(curMessage->getIfExtended()).toUpper());
                    simpleValue.setAttributeNode(attr);
                    initialValue.appendChild(simpleValue);
                    variable.appendChild(type);
                    variable.appendChild(initialValue);
                    localVars.appendChild(variable);
                }

                for(unsigned i=0; i<64;i++){
                    if(i==9){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_BYTE_0");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }else if(i==17){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_BYTE_1");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_WORD_0");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement WORD = doc.createElement("WORD");
                        type.appendChild(WORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==25){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_BYTE_2");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==33){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_BYTE_3");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_WORD_1");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement WORD = doc.createElement("WORD");
                        type.appendChild(WORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_DWORD_0");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement DWORD = doc.createElement("DWORD");
                        type.appendChild(DWORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==41){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_BYTE_4");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==49){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_BYTE_5");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_WORD_2");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement WORD = doc.createElement("WORD");
                        type.appendChild(WORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==57){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_BYTE_6");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }

                    {
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("S_II_BIT_"+QString::number(i));
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }

                    if(i==63){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_BYTE_7");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_WORD_3");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement WORD = doc.createElement("WORD");
                        type.appendChild(WORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_II_DWORD_1");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement DWORD = doc.createElement("DWORD");
                        type.appendChild(DWORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                }
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
            foreach(QList<QString> curVal , this->fbNameandObjId){
                if(curVal.at(0)== ("_FB_"+this->dutHeader+"_"+curMessage->getName()+"_0X"+curMessage->getID())){
                    text=doc.createTextNode(curVal.at(1));
                }
            }
            ObjectId.appendChild(text);
            data.appendChild(ObjectId);
            addData.appendChild(data);
            pou.appendChild(addData);
            pous->appendChild(pou);


           fbdBlocks.append(newBlock);
           // append input variables to AND gate to check disturbance -> EXP format : GVL.IIXC.S_TmOut_Motor_Messages_3_0X512
           gateAND->inputVars.append({"In"+QString::number(counterInANDOR),"GVL."+dutHeader+".S_TmOut_"+curMessage->getName()+"_0X"+curMessage->getID(),"BOOL"," "});
           // append input variables to OR gate to check disturbance -> EXP format : GVL.IIXC.S_TmOut_Motor_Messages_3_0X512
           gateOR->inputVars.append({"In"+QString::number(counterInANDOR),"GVL."+dutHeader+".S_TmOut_"+curMessage->getName()+"_0X"+curMessage->getID(),"BOOL"," "});
           counterInANDOR++;
        }

    }
 // It only has one output so It is outside of loop
  // append input variables to AND gate to check disturbance -> EXP format : GVL.IIXC.S_Com_Distrb_IIXC
    gateAND->outputVars.append({"Out1","GVL."+dutHeader+".S_Com_Distrb","BOOL"," "});
  // append input variables to OR gate to check disturbance -> EXP format : GVL.IIXC.S_Com_Flt_IIXC
    gateOR->outputVars.append({"Out1","GVL."+dutHeader+".S_Com_Flt","BOOL"," "});

    fbdBlocks.append(gateAND);
    fbdBlocks.append(gateOR);



}
void DBCHandler::generateIIST(QString *const ST,dataContainer *const curMessage)
{
    ST->append("(*\n"
               "**********************************************************************\n"
               "Bozankaya A.Ş.\n"
               "**********************************************************************\n"
               "Name					: _FB_"+dutHeader+"_"+curMessage->getName()+"_"+curMessage->getID()+"\n"
               "POU Type				: FB\n"
               "Created by              : COMMUNICATION INTERFACE GENERATOR("+QHostInfo::localHostName()+") , BZK.\n"
               "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
               "Modifications			: see version description below\n"
               "\n"
               "\n"
               "FB Description:"
               "This FB which is created by automatically by communication interface generator \n handles the RX (INPUT) can messages according to CAN_500_LIB\n"
               "*********************************************************************\n"
               "\n"
               "Version 1	\n"
               "*********************************************************************\n"
               "Version Description:\n"
               "- initial version\n"
               "*********************************************************************\n"
               "*)");
    ST->append( "\n_FB_CanRx_Message_Unpack_0(\n"
                "   C_Enable:= C_Init_Can,\n"
                "   Obj_CAN:= Ptr_Obj_Can ,\n"
                "   X_MsgID:= P_ID_Can,\n"
                "   Tm_MsgTmOut:= P_Tm_MsgTmOut,\n"
                "   S_Extended:= P_Msg_Extd,\n"
                "   S_ER_TmOut=> S_Msg_TmOut,\n"
                "   S_Bit_0     =>  S_II_BIT_0	 ,\n"
                "   S_Bit_1     =>  S_II_BIT_1	 ,\n"
                "   S_Bit_2     =>  S_II_BIT_2	 ,\n"
                "   S_Bit_3     =>  S_II_BIT_3	 ,\n"
                "   S_Bit_4     =>  S_II_BIT_4	 ,\n"
                "   S_Bit_5     =>  S_II_BIT_5	 ,\n"
                "   S_Bit_6     =>  S_II_BIT_6	 ,\n"
                "   S_Bit_7     =>  S_II_BIT_7	 ,\n"
                "   X_Byte_0    => X_II_BYTE_0	 ,\n"
                "   S_Bit_8     =>  S_II_BIT_8	 ,\n"
                "   S_Bit_9     =>  S_II_BIT_9	 ,\n"
                "   S_Bit_10    => S_II_BIT_10	 ,\n"
                "   S_Bit_11    => S_II_BIT_11	 ,\n"
                "   S_Bit_12    => S_II_BIT_12	 ,\n"
                "   S_Bit_13    => S_II_BIT_13	 ,\n"
                "   S_Bit_14    => S_II_BIT_14	 ,\n"
                "   S_Bit_15    => S_II_BIT_15	 ,\n"
                "   X_Byte_1    => X_II_BYTE_1	 ,\n"
                "   X_Word_0    => X_II_WORD_0	 ,\n"
                "   S_Bit_16    => S_II_BIT_16	 ,\n"
                "   S_Bit_17    => S_II_BIT_17	 ,\n"
                "   S_Bit_18    => S_II_BIT_18	 ,\n"
                "   S_Bit_19    => S_II_BIT_19	 ,\n"
                "   S_Bit_20    => S_II_BIT_20	 ,\n"
                "   S_Bit_21    => S_II_BIT_21	 ,\n"
                "   S_Bit_22    => S_II_BIT_22	 ,\n"
                "   S_Bit_23    => S_II_BIT_23	 ,\n"
                "   X_Byte_2    => X_II_BYTE_2	 ,\n"
                "   S_Bit_24    => S_II_BIT_24	 ,\n"
                "   S_Bit_25    => S_II_BIT_25	 ,\n"
                "   S_Bit_26    => S_II_BIT_26	 ,\n"
                "   S_Bit_27    => S_II_BIT_27	 ,\n"
                "   S_Bit_28    => S_II_BIT_28	 ,\n"
                "   S_Bit_29    => S_II_BIT_29	 ,\n"
                "   S_Bit_30    => S_II_BIT_30	 ,\n"
                "   S_Bit_31    => S_II_BIT_31	 ,\n"
                "   X_Byte_3    => X_II_BYTE_3	 ,\n"
                "   X_Word_1    => X_II_WORD_1	 ,\n"
                "   X_DWord_0   => X_II_DWORD_0 ,\n"
                "   S_Bit_32    => S_II_BIT_32	 ,\n"
                "   S_Bit_33    => S_II_BIT_33	 ,\n"
                "   S_Bit_34    => S_II_BIT_34	 ,\n"
                "   S_Bit_35    => S_II_BIT_35	 ,\n"
                "   S_Bit_36    => S_II_BIT_36	 ,\n"
                "   S_Bit_37    => S_II_BIT_37	 ,\n"
                "   S_Bit_38    => S_II_BIT_38	 ,\n"
                "   S_Bit_39    => S_II_BIT_39	 ,\n"
                "   X_Byte_4    => X_II_BYTE_4	 ,\n"
                "   S_Bit_40    => S_II_BIT_40	 ,\n"
                "   S_Bit_41    => S_II_BIT_41	 ,\n"
                "   S_Bit_42    => S_II_BIT_42	 ,\n"
                "   S_Bit_43    => S_II_BIT_43	 ,\n"
                "   S_Bit_44    => S_II_BIT_44	 ,\n"
                "   S_Bit_45    => S_II_BIT_45	 ,\n"
                "   S_Bit_46    => S_II_BIT_46	 ,\n"
                "   S_Bit_47    => S_II_BIT_47	 ,\n"
                "   X_Byte_5    => X_II_BYTE_5	 ,\n"
                "   X_Word_2    => X_II_WORD_2	 ,\n"
                "   S_Bit_48    => S_II_BIT_48	 ,\n"
                "   S_Bit_49    => S_II_BIT_49	 ,\n"
                "   S_Bit_50    => S_II_BIT_50	 ,\n"
                "   S_Bit_51    => S_II_BIT_51	 ,\n"
                "   S_Bit_52    => S_II_BIT_52	 ,\n"
                "   S_Bit_53    => S_II_BIT_53	 ,\n"
                "   S_Bit_54    => S_II_BIT_54	 ,\n"
                "   S_Bit_55    => S_II_BIT_55	 ,\n"
                "   X_Byte_6    => X_II_BYTE_6	 ,\n"
                "   S_Bit_56    => S_II_BIT_56	 ,\n"
                "   S_Bit_57    => S_II_BIT_57	 ,\n"
                "   S_Bit_58    => S_II_BIT_58	 ,\n"
                "   S_Bit_59    => S_II_BIT_59	 ,\n"
                "   S_Bit_60    => S_II_BIT_60	 ,\n"
                "   S_Bit_61    => S_II_BIT_61	 ,\n"
                "   S_Bit_62    => S_II_BIT_62	 ,\n"
                "   S_Bit_63    => S_II_BIT_63	 ,\n"
                "   X_Byte_7    => X_II_BYTE_7	 ,\n"
                "   X_Word_3    => X_II_WORD_3	 ,\n"
                "   X_DWord_1   =>X_II_DWORD_1   );\n");

    for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
        ST->append(convTypeComtoApp(curSignal->name,curSignal->startBit,curSignal->length,curSignal->convMethod));
    }


}
QString DBCHandler::convTypeComtoApp(QString signalName, unsigned short startbit,unsigned short length,  QString converType)
{
    QString ST="\n\n\n{region \""+signalName+"\"}\n\n\n";
    if(converType=="BOOL:BOOL"){
        ST.append("\n"+this->dutHeader+"."+signalName+".v               := NOT S_Msg_TmOut OR FrcHi_"+signalName+" OR FrcLo_"+signalName+" ;"
                  "\n"+this->dutHeader+"."+signalName+".x                := "+this->dutHeader+"."+signalName+".v AND (S_II_BIT_"+QString::number(startbit)+" OR FrcHi_"+signalName+") AND NOT FrcLo_"+signalName+" ;");
    }else if(converType=="2BOOL:BOOL"){
        ST.append("\n"+this->dutHeader+"."+signalName+".v               := ((NOT S_Msg_TmOut AND NOT S_II_BIT_"+QString::number(startbit+1)+") OR FrcHi_"+signalName+" OR FrcLo_"+signalName+") ;"
                   "\n"+this->dutHeader+"."+signalName+".x               := "+this->dutHeader+"."+signalName+".v AND (S_II_BIT_"+QString::number(startbit+1)+" OR FrcHi_"+signalName+") AND NOT FrcLo_"+signalName+";");
    }else if((converType=="xtoBYTE")||(converType=="xtoUSINT")||((converType=="xtoREAL") && (length==8))){
        ST.append("\nRaw_"+signalName+"             := X_II_BYTE_"+QString::number(startbit/8)+";");
    }else if((converType=="xtoWORD")||(converType=="xtoUINT")||((converType=="xtoREAL") && (length==16))){
        ST.append("\n_FB_PACK_BYTE_TO_WORD_"+QString::number(counterfbBYTETOWORD)+"(X_BYTE_0:= X_II_BYTE_"+QString::number(startbit/8)+", X_BYTE_1:=  X_II_BYTE_"+QString::number((startbit/8)+1)+", X_WORD_0 => Raw_"+signalName+");");
        counterfbBYTETOWORD++;
    }else if((converType=="xtoDWORD")||(converType=="xtoUDINT")||((converType=="xtoREAL") && (length==32))){
        ST.append("\n_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+"(X_BYTE_0:= X_II_BYTE_"+QString::number(startbit/8)+", X_BYTE_1:= X_II_BYTE_"+QString::number((startbit/8)+1)+", X_BYTE_2:= X_II_BYTE_"+QString::number((startbit/8)+2)+", X_BYTE_3:= X_II_BYTE_"+QString::number((startbit/8)+3)+" , X_DWORD_0 => Raw_"+signalName+");");
        counterfbBYTETODWORD++;
    }else if((converType=="xtoLWORD")||(converType=="xtoULINT")||(converType=="xtoLREAL") ){
        ST.append("\n_FB_PACK_BYTE_TO_LWORD_"+QString::number(counterfbBYTETOLWORD)+"(X_BYTE_0:= X_II_BYTE_"+QString::number(startbit/8)+", X_BYTE_1:= X_II_BYTE_"+QString::number((startbit/8)+1)+", X_BYTE_2:=X_II_BYTE_"+QString::number((startbit/8)+2)+" , X_BYTE_3:= X_II_BYTE_"+QString::number((startbit/8)+3)+", X_BYTE_4:= X_II_BYTE_"+QString::number((startbit/8)+4)+", X_BYTE_5:= X_II_BYTE_"+QString::number((startbit/8)+5)+", X_BYTE_6:= X_II_BYTE_"+QString::number((startbit/8)+6)+", X_BYTE_7:= X_II_BYTE_"+QString::number((startbit/8)+7)+" , X_LWORD_0 => Raw_"+signalName+");");
        counterfbBYTETOLWORD++;
    }else{
        bool flagPack = false;
        if((length>8)){
            if(length<17){
                ST.append("\n_FB_PACK_BYTE_TO_WORD_"+QString::number(counterfbBYTETOWORD)+"();");

            }else if(length <33){
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
        for(unsigned i =0; i<length;i++){

            if((i%32 == 0) && (i>0) && length>16){
                packID++;
                packByteID=0;
            }
            if((i%16 == 0) && (i>0) && length<=16){
                packID++;
                packByteID=0;
            }

            if(((i%8 == 0) && ((i/8)<8) )&& (!flagPack)){
                ST.append("\n_FB_PACK_8BITS_TO_BYTE_"+QString::number(counterfb8BITTOBYTE+qFloor(i/8.0))+"(");
                counterBITBYTE++;
                flagPack=true;
            }
            if(flagPack){
                    ST.append("S_BIT_"+QString::number(i%8)+":= S_II_BIT_"+QString::number(startbit+i)+"");
                    if(((i%8 != 7) &&(i%(length-1) != 0)) || (i==0)) {
                        ST.append(",");
                    }
            }
            if(flagPack){
                if(((i%8 == 7)||(i%(length-1) == 0) )&& (i>0)){
                    ST.append(" ,X_BYTE_0=>");
                    if(length<9){
                    ST.append("Raw_"+signalName);
                    ST.append(");");
                    }else if (length <17){
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

        if((length>8)){
            if(length<17){
                ST.append("\nRaw_"+signalName+"            :=_FB_PACK_BYTE_TO_WORD_"+QString::number(counterfbBYTETOWORD)+".X_WORD_0;");
                counterfbBYTETOWORD++;
            }else if(length <33){
                ST.append("\nRaw_"+signalName+"            :=_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+".X_DWORD_0;");
                counterfbBYTETODWORD++;
            }else {
                ST.append("\n_FB_PACK_DWORD_TO_LWORD_"+QString::number(counterfbDWORDTOLWORD)+"(X_DWORD_0:=_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD)+".X_DWORD_0,X_DWORD_1:=_FB_PACK_BYTE_TO_DWORD_"+QString::number(counterfbBYTETODWORD+1)+".X_DWORD_0,X_LWORD_0=> Raw_"+signalName+");");
            counterfbDWORDTOLWORD++;
            counterfbBYTETODWORD++;
            counterfbBYTETODWORD++;
            }
        }
        counterfb8BITTOBYTE = counterfb8BITTOBYTE+ counterBITBYTE;
    }
    // TYPE CONVERTION AND E  NA CONTROL STARTS
    if(length==1){
        ST.append("\n"+this->dutHeader+"."+signalName+".e				:= FALSE;");
        ST.append("\n"+this->dutHeader+"."+signalName+".na				:= FALSE;");
    }
    else if((length==2) && (converType != "toBYTE")){

        ST.append("\n"+this->dutHeader+"."+signalName+".e				:= S_II_BIT_"+QString::number(startbit+1)+" AND NOT S_II_BIT_"+QString::number(startbit)+" ;");
        ST.append("\n"+this->dutHeader+"."+signalName+".na				:= S_II_BIT_"+QString::number(startbit+1)+" AND S_II_BIT_"+QString::number(startbit)+" ;");

    }
    else if((length<9)){
        if((converType == "toBYTE")|| (converType == "xtoBYTE")){
            ST.append("\nCont_"+signalName+"				:= Raw_"+signalName+" ;");
        }
        else if ((converType == "toUSINT")|| (converType == "xtoUSINT")){
            ST.append("\nCont_"+signalName+"				:= BYTE_TO_USINT(Raw_"+signalName+") ;");
        }
        else if ((converType == "toREAL")|| (converType == "xtoREAL")){
            ST.append("\nCont_"+signalName+"				:= USINT_TO_REAL(BYTE_TO_USINT(Raw_"+signalName+"))*"+this->dutHeader+"."+signalName+".Param_Res+"+this->dutHeader+"."+signalName+".Param_Off ;");
        }
        ST.append( "\n"+this->dutHeader+"."+signalName+".e              := (Raw_"+signalName+" > 16#FD);");
        ST.append("\n"+this->dutHeader+"."+signalName+".na              := (Raw_"+signalName+" = 16#FF) ;");

    }else if((length<17)){
        if((converType == "toWORD")|| (converType == "xtoWORD")){
            ST.append("\nCont_"+signalName+"				:= Raw_"+signalName+" ;");
        }
        else if ((converType == "toUINT")|| (converType == "xtoUINT")){
            ST.append("\nCont_"+signalName+"				:= WORD_TO_UINT(Raw_"+signalName+") ;");
        }
        else if ((converType == "toREAL")|| (converType == "xtoREAL")){
            ST.append("\n Cont_"+signalName+"				:= UINT_TO_REAL(WORD_TO_UINT(Raw_"+signalName+"))*"+this->dutHeader+"."+signalName+".Param_Res+"+this->dutHeader+"."+signalName+".Param_Off ;");
        }
        ST.append("\n"+this->dutHeader+"."+signalName+".e              := (Raw_"+signalName+" > 16#FDFF) AND (Raw_"+signalName+" < 16#FF00) ;");
        ST.append("\n"+this->dutHeader+"."+signalName+".na              := (Raw_"+signalName+" > 16#FEFF) ;");

    }else if((length<33)){
        if((converType == "toDWORD")|| (converType == "xtoDWORD")){
            ST.append("\nCont_"+signalName+"               := Raw_"+signalName+" ;");
        }
        else if ((converType == "toUDINT")|| (converType == "xtoUDINT")){
            ST.append("\nCont_"+signalName+"				:= DWORD_TO_UDINT(Raw_"+signalName+") ;");
        }
        else if ((converType == "toREAL")|| (converType == "xtoREAL")){
            ST.append("\nCont_"+signalName+"				:= UDINT_TO_REAL(DWORD_TO_UDINT(Raw_"+signalName+"))*"+this->dutHeader+"."+signalName+".Param_Res+"+this->dutHeader+"."+signalName+".Param_Off ;");
        }
        ST.append("\n"+this->dutHeader+"."+signalName+".e              := (Raw_"+signalName+" > 16#FDFFFFFF) AND (Raw_"+signalName+" < 16#FF000000) ;");
        ST.append("\n"+this->dutHeader+"."+signalName+".na              := (Raw_"+signalName+" > 16#FEFFFFFF) ;");

    }else if((length<65)){
        if((converType == "toLWORD")|| (converType == "xtoLWORD")){
            ST.append("\nCont_"+signalName+"               := Raw_"+signalName+" ;");
        }
        else if ((converType == "toULINT")|| (converType == "xtoULINT")){
            ST.append("\nCont_"+signalName+"				:= LWORD_TO_ULINT(Raw_"+signalName+") ;");
        }
        else if ((converType == "toLREAL")|| (converType == "xtoLREAL")){
            ST.append("\nCont_"+signalName+"				:= ULINT_TO_LREAL(LWORD_TO_ULINT(Raw_"+signalName+"))*"+this->dutHeader+"."+signalName+".Param_Res+"+this->dutHeader+"."+signalName+".Param_Off ;");
        }
        ST.append("\n"+this->dutHeader+"."+signalName+".e              := (Raw_"+signalName+" > 16#FDFFFFFFFFFFFFFF) AND (Raw_"+signalName+" < 16#FF00000000000000) ;");
        ST.append("\n"+this->dutHeader+"."+signalName+".na              := (Raw_"+signalName+" > 16#FEFFFFFFFFFFFFFF) ;");

    }// TYPE CONVERTION AND E  NA CONTROL ENDS
// ADD COMMON PART EXCEPT BOOL AND 2XBOOLS
    if((length!= 1) && (!(length==2 && (converType !="toByte")))){
        ST.append("\n"+this->dutHeader+"."+signalName+".RangeExcd 		:= NOT ((Cont_"+signalName+" >= "+this->dutHeader+"."+signalName+".Param_Min) AND ("+this->dutHeader+"."+signalName+".Param_Min <= "+this->dutHeader+"."+signalName+".Param_Max));"
        "\n"+this->dutHeader+"."+signalName+".v				:= NOT( S_Msg_TmOut OR "+this->dutHeader+"."+signalName+".RangeExcd OR (( "+this->dutHeader+"."+signalName+".e OR "+this->dutHeader+"."+signalName+".na) AND "+this->dutHeader+"."+signalName+".J1939)) OR FrcEn_"+signalName+";"
        "\n"
        "\nIF FrcEn_"+signalName+" THEN"
        "\n	"+this->dutHeader+"."+signalName+".x 		   	:= FrcVal_"+signalName+";"
        "\nELSIF "+this->dutHeader+"."+signalName+".v THEN"
        "\n	"+this->dutHeader+"."+signalName+".x 		   	:= Cont_"+signalName+";"
        "\nELSE"
        "\n	"+this->dutHeader+"."+signalName+".x 		   	:= "+this->dutHeader+"."+signalName+".Param_Def;"
        "\nEND_IF;\n");
    }
    ST.append("\n\n\n{endregion}\n\n\n");
    return ST;
}

///******************************************************************************
/// TRANSMIT MESSAGES FUNCTION BLOCK ST GENERATOR
///******************************************************************************

void DBCHandler::generateIOPous(QDomElement * pous, QDomDocument &doc)
{
    QDomAttr attr;
    QDomText text;

    foreach (dataContainer * curMessage , comInterface){
        if(curMessage->getIfSelected()){
            QDomElement pou = doc.createElement("pou");
            structFbdBlock* newBlock = new structFbdBlock;

            /*set pou name*/
            attr=doc.createAttribute("name");
            attr.setValue("_FB_"+this->dutHeader+"_"+curMessage->getName()+"_0X"+curMessage->getID());
            newBlock->name="_FB_"+this->dutHeader+"_"+curMessage->getName()+"_0X"+curMessage->getID();
            pou.setAttributeNode(attr);
            /*set pouType*/
            attr=doc.createAttribute("pouType");
            attr.setValue("functionBlock");
            pou.setAttributeNode(attr);
            /*Interface*/
            QDomElement interface = doc.createElement("interface");
            {
                QDomElement inputVars = doc.createElement("inputVars");

                /*Generate Input Variables - inputVars*/
                /*C_Init_Can*/
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("C_Init_Can");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement BOOL = doc.createElement("BOOL");
                    type.appendChild(BOOL);
                    variable.appendChild(type);
                    inputVars.appendChild(variable);
                    newBlock->inputVars.append({"C_Init_Can","GVL."+dutHeader+".S_CAN_Init","BOOL"," "});
                }
                /*Ptr_Obj_Can*/
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("Ptr_Obj_Can");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement pointer = doc.createElement("pointer");
                    QDomElement baseType = doc.createElement("baseType");
                    QDomElement derived =doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue("tCan");
                    derived.setAttributeNode(attr);
                    baseType.appendChild(derived);
                    pointer.appendChild(baseType);
                    type.appendChild(pointer);
                    variable.appendChild(type);
                    inputVars.appendChild(variable);
                    newBlock->inputVars.append({"Ptr_Obj_Can","ADR("+canLine+")","POINTER to tCan"," "});
                }
                /*Start to generate variables*/
                for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                    /*Bool signals has FrcHi and FrcLo, others FrcEn FrcVal*/
                    if(curSignal->appDataType != "BOOL"){
                        /*Create FrcEn */
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("FrcEn_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"FrcEn_"+curSignal->name,"FrcHi."+curSignal->name+".v","BOOL"," "});
                        /*Create FrcVal */
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("FrcVal_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement signalDataType = doc.createElement(curSignal->appDataType);
                        type.appendChild(signalDataType);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"FrcVal_"+curSignal->name,"FrcHi."+curSignal->name+".x",curSignal->comDataType," "});
                    }
                    else {
                        /*Create FrcHi */
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("FrcHi_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"FrcHi_"+curSignal->name,"FrcHi."+curSignal->name+".x","BOOL"," "});
                        /*Create FrcLo */
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("FrcLo_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"FrcLo_"+curSignal->name,"FrcLo."+curSignal->name+".x","BOOL"," "});
                    }

                }
                interface.appendChild(inputVars);
            }
            /*Generate Output Variables - outputVars*/
            {
                QDomElement outputVars = doc.createElement("outputVars");;
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("S_Msg_Snt_Ok");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement BOOL = doc.createElement("BOOL");
                    type.appendChild(BOOL);
                    variable.appendChild(type);
                    outputVars.appendChild(variable);
                    newBlock->outputVars.append({"S_Msg_Snt_Ok","GVL."+dutHeader+".S_SentOk_"+curMessage->getName()+"_0X"+curMessage->getID(),"BOOL"," "});
                }
                interface.appendChild(outputVars);
            }
            /*Generate Output Input Variables - inoutVars*/
            {
                QDomElement inoutVars = doc.createElement("inOutVars");
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue(dutHeader);
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue(dutName);
                    dataVarType.setAttributeNode(attr);
                    type.appendChild(dataVarType);
                    variable.appendChild(type);
                    inoutVars.appendChild(variable);
                }
                interface.appendChild(inoutVars);
            }
            QString STcode;
            this->generateIOST(&STcode,curMessage);
            /*Generate Local Variables - localVars*/
            {
                QDomElement localVars= doc.createElement("localVars");
                /*Function block declaration*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_CanTx_Message_Unpack_0");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement derived = doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue("_FB_CanTx_Message_Unpack");
                    derived.setAttributeNode(attr);
                    type.appendChild(derived);
                    variable.appendChild(type);
                    localVars.appendChild(variable);
                }
                /*P_ID_Can*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("P_ID_Can");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("UDINT");
                    type.appendChild(dataVarType);
                    QDomElement initialValue = doc.createElement("initialValue");
                    QDomElement simpleValue = doc.createElement("simpleValue");
                    attr=doc.createAttribute("value");
                    attr.setValue("16#"+curMessage->getID());
                    simpleValue.setAttributeNode(attr);
                    initialValue.appendChild(simpleValue);
                    variable.appendChild(type);
                    variable.appendChild(initialValue);
                    localVars.appendChild(variable);
                }
                /*P_Tm_CycleTime*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("P_Tm_CycleTime");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("TIME");
                    type.appendChild(dataVarType);
                    QDomElement initialValue = doc.createElement("initialValue");
                    QDomElement simpleValue = doc.createElement("simpleValue");
                    attr=doc.createAttribute("value");
                    attr.setValue("TIME#"+curMessage->getMsCycleTime()+"ms");
                    simpleValue.setAttributeNode(attr);
                    initialValue.appendChild(simpleValue);
                    variable.appendChild(type);
                    variable.appendChild(initialValue);
                    localVars.appendChild(variable);
                }
                /*P_Msg_Dlc*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("P_Msg_Dlc");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("BYTE");
                    type.appendChild(dataVarType);
                    QDomElement initialValue = doc.createElement("initialValue");
                    QDomElement simpleValue = doc.createElement("simpleValue");
                    attr=doc.createAttribute("value");
                    attr.setValue(QString::number(curMessage->getDLC()));
                    simpleValue.setAttributeNode(attr);
                    initialValue.appendChild(simpleValue);
                    variable.appendChild(type);
                    variable.appendChild(initialValue);
                    localVars.appendChild(variable);
                }
                /*P_Msg_Extd*/
                {
                    QDomElement variable=doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue("P_Msg_Extd");
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("BOOL");
                    type.appendChild(dataVarType);
                    QDomElement initialValue = doc.createElement("initialValue");
                    QDomElement simpleValue = doc.createElement("simpleValue");
                    attr=doc.createAttribute("value");
                    attr.setValue(QString::number(curMessage->getIfExtended()).toUpper());
                    simpleValue.setAttributeNode(attr);
                    initialValue.appendChild(simpleValue);
                    variable.appendChild(type);
                    variable.appendChild(initialValue);
                    localVars.appendChild(variable);
                }

                for(unsigned i=0; i<64;i++){
                    if(i==9){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_BYTE_0");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }else if(i==17){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_BYTE_1");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_WORD_0");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement WORD = doc.createElement("WORD");
                        type.appendChild(WORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==25){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_BYTE_2");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==33){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_BYTE_3");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_WORD_1");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement WORD = doc.createElement("WORD");
                        type.appendChild(WORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_DWORD_0");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement DWORD = doc.createElement("DWORD");
                        type.appendChild(DWORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==41){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_BYTE_4");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==49){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_BYTE_5");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_WORD_2");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement WORD = doc.createElement("WORD");
                        type.appendChild(WORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                    else if(i==57){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_BYTE_6");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }

                    {
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("S_IO_BIT_"+QString::number(i));
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }

                    if(i==63){
                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_BYTE_7");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BYTE = doc.createElement("BYTE");
                        type.appendChild(BYTE);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_WORD_3");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement WORD = doc.createElement("WORD");
                        type.appendChild(WORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                        variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("X_IO_DWORD_1");
                        variable.setAttributeNode(attr);
                        type = doc.createElement("type");
                        QDomElement DWORD = doc.createElement("DWORD");
                        type.appendChild(DWORD);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                }
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
            for(QList<QString> curVal : this->fbNameandObjId){
                if(curVal.at(0)== ("_FB_"+this->dutHeader+"_"+curMessage->getName()+"_0X"+curMessage->getID())){
                    text=doc.createTextNode(curVal.at(1));
                }
            }
            ObjectId.appendChild(text);
            data.appendChild(ObjectId);
            addData.appendChild(data);
            pou.appendChild(addData);
            pous->appendChild(pou);

            fbdBlocks.append(newBlock);
        }

    }


}
void DBCHandler::generateIOST(QString *const ST,dataContainer *const curMessage)
{       ST->append("(*\n"
                   "**********************************************************************\n"
                   "Bozankaya A.Ş.\n"
                   "**********************************************************************\n"
                   "Name					: _FB_"+dutHeader+"_"+curMessage->getName()+"_"+curMessage->getID()+"\n"
                   "POU Type				: FB\n"
                   "Created by              : COMMUNICATION INTERFACE GENERATOR , BZK.\n"
                   "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
                   "Modifications			: see version description below\n"
                   "\n"
                   "\n"
                   "FB Description:"
                   "This FB which is created by automatically by communication interface generator \n handles the TX (OUTPUT) can messages according to CAN_500_LIB \n"
                   "*********************************************************************\n"
                   "\n"
                   "Version 1	\n"
                   "*********************************************************************\n"
                   "Version Description:\n"
                   "- initial version\n"
                   "*********************************************************************\n"
                   "*)");
    for( const dataContainer::signal * curSignal : *curMessage->getSignalList()){
        ST->append(convTypeApptoCom(curSignal->name,curSignal->startBit,curSignal->length,curSignal->convMethod));
    }
    ST->append(    "_FB_CanTx_Message_Unpack_0(\n"
                   "    C_Enable:= C_Init_Can,\n"
                   "    Obj_CAN:= Ptr_Obj_Can,\n"
                   "    X_MsgID:= P_ID_Can,\n"
                   "    Tm_CycleTime:= P_Tm_CycleTime,\n"
                   "    N_Msg_Dlc:=P_Msg_Dlc,\n"
                   "    S_Extended:= P_Msg_Extd,\n"
                   "    S_Bit_0     :=	S_IO_BIT_0	,\n"
                   "    S_Bit_1     :=	S_IO_BIT_1	,\n"
                   "    S_Bit_2     :=	S_IO_BIT_2	,\n"
                   "    S_Bit_3     :=	S_IO_BIT_3	,\n"
                   "    S_Bit_4     :=	S_IO_BIT_4	,\n"
                   "    S_Bit_5     :=	S_IO_BIT_5	,\n"
                   "    S_Bit_6     :=	S_IO_BIT_6	,\n"
                   "    S_Bit_7     :=	S_IO_BIT_7	,\n"
                   "    X_Byte_0    := 	X_IO_BYTE_0	,\n"
                   "    S_Bit_8     := 	S_IO_BIT_8	,\n"
                   "    S_Bit_9     := 	S_IO_BIT_9	,\n"
                   "    S_Bit_10    := 	S_IO_BIT_10	,\n"
                   "    S_Bit_11    := 	S_IO_BIT_11	,\n"
                   "    S_Bit_12    := 	S_IO_BIT_12	,\n"
                   "    S_Bit_13    := 	S_IO_BIT_13	,\n"
                   "    S_Bit_14    := 	S_IO_BIT_14	,\n"
                   "    S_Bit_15    := 	S_IO_BIT_15	,\n"
                   "    X_Byte_1    := 	X_IO_BYTE_1	,\n"
                   "    X_Word_0    := 	X_IO_WORD_0	,\n"
                   "    S_Bit_16    := 	S_IO_BIT_16	,\n"
                   "    S_Bit_17    := 	S_IO_BIT_17	,\n"
                   "    S_Bit_18    := 	S_IO_BIT_18	,\n"
                   "    S_Bit_19    := 	S_IO_BIT_19	,\n"
                   "    S_Bit_20    := 	S_IO_BIT_20	,\n"
                   "    S_Bit_21    := 	S_IO_BIT_21	,\n"
                   "    S_Bit_22    := 	S_IO_BIT_22	,\n"
                   "    S_Bit_23    := 	S_IO_BIT_23	,\n"
                   "    X_Byte_2    := 	X_IO_BYTE_2	,\n"
                   "    S_Bit_24    := 	S_IO_BIT_24	,\n"
                   "    S_Bit_25    := 	S_IO_BIT_25	,\n"
                   "    S_Bit_26    := 	S_IO_BIT_26	,\n"
                   "    S_Bit_27    := 	S_IO_BIT_27	,\n"
                   "    S_Bit_28    := 	S_IO_BIT_28	,\n"
                   "    S_Bit_29    := 	S_IO_BIT_29	,\n"
                   "    S_Bit_30    := 	S_IO_BIT_30	,\n"
                   "    S_Bit_31    := 	S_IO_BIT_31	,\n"
                   "    X_Byte_3    := 	X_IO_BYTE_3	,\n"
                   "    X_Word_1    := 	X_IO_WORD_1	,\n"
                   "    X_DWord_0   := X_IO_DWORD_0,\n"
                   "    S_Bit_32    := 	S_IO_BIT_32	,\n"
                   "    S_Bit_33    := 	S_IO_BIT_33	,\n"
                   "    S_Bit_34    := 	S_IO_BIT_34	,\n"
                   "    S_Bit_35    := 	S_IO_BIT_35	,\n"
                   "    S_Bit_36    := 	S_IO_BIT_36	,\n"
                   "    S_Bit_37    := 	S_IO_BIT_37	,\n"
                   "    S_Bit_38    := 	S_IO_BIT_38	,\n"
                   "    S_Bit_39    := 	S_IO_BIT_39	,\n"
                   "    X_Byte_4    := 	X_IO_BYTE_4	,\n"
                   "    S_Bit_40    := 	S_IO_BIT_40	,\n"
                   "    S_Bit_41    := 	S_IO_BIT_41	,\n"
                   "    S_Bit_42    := 	S_IO_BIT_42	,\n"
                   "    S_Bit_43    := 	S_IO_BIT_43	,\n"
                   "    S_Bit_44    := 	S_IO_BIT_44	,\n"
                   "    S_Bit_45    := 	S_IO_BIT_45	,\n"
                   "    S_Bit_46    := 	S_IO_BIT_46	,\n"
                   "    S_Bit_47    := 	S_IO_BIT_47	,\n"
                   "    X_Byte_5    := 	X_IO_BYTE_5	,\n"
                   "    X_Word_2    := 	X_IO_WORD_2	,\n"
                   "    S_Bit_48    := 	S_IO_BIT_48	,\n"
                   "    S_Bit_49    := 	S_IO_BIT_49	,\n"
                   "    S_Bit_50    := 	S_IO_BIT_50	,\n"
                   "    S_Bit_51    := 	S_IO_BIT_51	,\n"
                   "    S_Bit_52    := 	S_IO_BIT_52	,\n"
                   "    S_Bit_53    := 	S_IO_BIT_53	,\n"
                   "    S_Bit_54    := 	S_IO_BIT_54	,\n"
                   "    S_Bit_55    := 	S_IO_BIT_55	,\n"
                   "    X_Byte_6    := 	X_IO_BYTE_6	,\n"
                   "    S_Bit_56    := 	S_IO_BIT_56	,\n"
                   "    S_Bit_57    := 	S_IO_BIT_57	,\n"
                   "    S_Bit_58    := 	S_IO_BIT_58	,\n"
                   "    S_Bit_59    := 	S_IO_BIT_59	,\n"
                   "    S_Bit_60    := 	S_IO_BIT_60	,\n"
                   "    S_Bit_61    := 	S_IO_BIT_61	,\n"
                   "    S_Bit_62    := 	S_IO_BIT_62	,\n"
                   "    S_Bit_63    := 	S_IO_BIT_63	,\n"
                   "    X_Byte_7    := 	X_IO_BYTE_7	,\n"
                   "    X_Word_3    := 	X_IO_WORD_3	,\n"
                   "    X_DWord_1   := X_IO_DWORD_1,\n"
                   "    S_Sent_Ok   => S_Msg_Snt_Ok);\n" );



}
QString DBCHandler::convTypeApptoCom (QString signalName, unsigned short startbit,unsigned short length,  QString converType)
{

    QString ST="\n\n\n{region \""+signalName+"\"}\n\n\n";
    // TYPE CONVERTION AND E  NA CONTROL STARTS
    if((length<9)){
        if((converType == "toBYTE")|| (converType == "xtoBYTE")){
            ST.append(""+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= FrcVal_"+signalName+";"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= "+this->dutHeader+"."+signalName+".x ;"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN"
            "\n 		Cont_"+signalName+":= 16#FF;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 		Cont_"+signalName+":= 16#FE;"
            "\n 	ELSE"
            "\n 		Cont_"+signalName+":= 16#FE;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= "+this->dutHeader+"."+signalName+".Param_Def ;"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");
        }
        else if ((converType == "toUSINT")|| (converType == "xtoUSINT")){

            ST.append("<"+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= USINT_TO_BYTE(FrcVal_"+signalName+");"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= USINT_TO_BYTE("+this->dutHeader+"."+signalName+".x );"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN"
            "\n 		Cont_"+signalName+":= 16#FF;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 		Cont_"+signalName+":= 16#FE;"
            "\n 	ELSE"
            "\n 		Cont_"+signalName+":= 16#FE;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= USINT_TO_BYTE("+this->dutHeader+"."+signalName+".Param_Def );"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");

        }
        else if ((converType == "toREAL")|| (converType == "xtoREAL")){


            ST.append(""+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= USINT_TO_BYTE(REAL_TO_USINT(( FrcVal_"+signalName+" + "+this->dutHeader+"."+signalName+".Param_Off) / "+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= USINT_TO_BYTE(REAL_TO_USINT(("+this->dutHeader+"."+signalName+".x + "+this->dutHeader+"."+signalName+".Param_Off) / "+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN"
            "\n 		Cont_"+signalName+":= 16#FF;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 		Cont_"+signalName+":= 16#FE;"
            "\n 	ELSE"
            "\n 		Cont_"+signalName+":= 16#FE;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= USINT_TO_BYTE(REAL_TO_USINT(("+this->dutHeader+"."+signalName+".Param_Def + "+this->dutHeader+"."+signalName+".Param_Off) /"+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");

        }

    }else if((length<17)){
        if((converType == "toWORD")|| (converType == "xtoWORD")){
            ST.append(""+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
           "\n //Transmit Data : Range Control End"
           "\n IF FrcEn_"+signalName+" THEN"
           "\n 	//Transmit Data : Force variable Start"
           "\n 	Cont_"+signalName+"	:= FrcVal_"+signalName+";"
           "\n 	//Transmit Data : Force variable Start"
           "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
           "\n 	//Transmit Data : Normal Start"
           "\n 	Cont_"+signalName+"	:= "+this->dutHeader+"."+signalName+".x ;"
           "\n 	 //Transmit Data : Normal End"
           "\n ELSE"
           "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
           "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
           "\n 	IF "+this->dutHeader+"."+signalName+".na THEN"
           "\n 		Cont_"+signalName+":= 16#FF01;"
           "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
           "\n 		Cont_"+signalName+":= 16#FE01;"
           "\n 	ELSE"
           "\n 		Cont_"+signalName+":= 16#FE01;"
           "\n 	END_IF;"
           "\n 	//Transmit Data : Data not valid J1939 error transmission End"
           "\n 	ELSE"
           "\n 	//Transmit Data : Data not valid  Start"
           "\n 		Cont_"+signalName+":= "+this->dutHeader+"."+signalName+".Param_Def ;"
           "\n 	//Transmit Data : Data not valid  End"
           "\n 	END_IF;"
           "\n END_IF;");
        }
        else if ((converType == "toUINT")|| (converType == "xtoUINT")){

            ST.append("<"+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= UINT_TO_WORD(FrcVal_"+signalName+");"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= UINT_TO_WORD("+this->dutHeader+"."+signalName+".x );"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN"
            "\n 		Cont_"+signalName+":= 16#FF01;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 		Cont_"+signalName+":= 16#FE01;"
            "\n 	ELSE"
            "\n 		Cont_"+signalName+":= 16#FE01;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= UINT_TO_WORD("+this->dutHeader+"."+signalName+".Param_Def );"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");

        }
        else if ((converType == "toREAL")|| (converType == "xtoREAL")){
            ST.append(""+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= UINT_TO_WORD(REAL_TO_UINT(( FrcVal_"+signalName+" + "+this->dutHeader+"."+signalName+".Param_Off) / "+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= UINT_TO_WORD(REAL_TO_UINT(("+this->dutHeader+"."+signalName+".x + "+this->dutHeader+"."+signalName+".Param_Off) / "+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN"
            "\n 		Cont_"+signalName+":= 16#FF01;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 		Cont_"+signalName+":= 16#FE01;"
            "\n 	ELSE"
            "\n 		Cont_"+signalName+":= 16#FE01;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= UINT_TO_WORD(REAL_TO_UINT(("+this->dutHeader+"."+signalName+".Param_Def + "+this->dutHeader+"."+signalName+".Param_Off) /"+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");
        }

    }else if((length<33)){
        if((converType == "toDWORD")|| (converType == "xtoDWORD")){
            ST.append(""+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
           "\n //Transmit Data : Range Control End"
           "\n IF FrcEn_"+signalName+" THEN"
           "\n 	//Transmit Data : Force variable Start"
           "\n 	Cont_"+signalName+"	:= FrcVal_"+signalName+";"
           "\n 	//Transmit Data : Force variable Start"
           "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
           "\n 	//Transmit Data : Normal Start"
           "\n 	Cont_"+signalName+"	:= "+this->dutHeader+"."+signalName+".x ;"
           "\n 	 //Transmit Data : Normal End"
           "\n ELSE"
           "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
           "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
           "\n 	IF "+this->dutHeader+"."+signalName+".na THEN	"
           "\n 	Cont_"+signalName+":= 16#FF000001;"
           "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
           "\n 	Cont_"+signalName+":= 16#FE000001;"
           "\n 	ELSE"
           "\n 	Cont_"+signalName+":= 16#FE000001;"
           "\n 	END_IF;"
           "\n 	//Transmit Data : Data not valid J1939 error transmission End"
           "\n 	ELSE"
           "\n 	//Transmit Data : Data not valid  Start"
           "\n 		Cont_"+signalName+":= "+this->dutHeader+"."+signalName+".Param_Def ;"
           "\n 	//Transmit Data : Data not valid  End"
           "\n 	END_IF;"
           "\n END_IF;");
        }
        else if ((converType == "toUDINT")|| (converType == "xtoUDINT")){

            ST.append("<"+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
           "\n //Transmit Data : Range Control End"
           "\n IF FrcEn_"+signalName+" THEN"
           "\n 	//Transmit Data : Force variable Start"
           "\n 	Cont_"+signalName+"	:= UDINT_TO_DWORD(FrcVal_"+signalName+");"
           "\n 	//Transmit Data : Force variable Start"
           "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
           "\n 	//Transmit Data : Normal Start"
           "\n 	Cont_"+signalName+"	:= UDINT_TO_DWORD("+this->dutHeader+"."+signalName+".x );"
           "\n 	 //Transmit Data : Normal End"
           "\n ELSE"
           "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
           "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
           "\n 	IF "+this->dutHeader+"."+signalName+".na THEN	"
           "\n 	Cont_"+signalName+":= 16#FF000001;"
           "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
           "\n 	Cont_"+signalName+":= 16#FE000001;"
           "\n 	ELSE"
           "\n 	Cont_"+signalName+":= 16#FE000001;"
           "\n 	END_IF;"
           "\n 	//Transmit Data : Data not valid J1939 error transmission End"
           "\n 	ELSE"
           "\n 	//Transmit Data : Data not valid  Start"
           "\n 		Cont_"+signalName+":= UDINT_TO_DWORD("+this->dutHeader+"."+signalName+".Param_Def );"
           "\n 	//Transmit Data : Data not valid  End"
           "\n 	END_IF;"
           "\n END_IF;");

        }
        else if ((converType == "toREAL")|| (converType == "xtoREAL")){
            ST.append(""+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= UDINT_TO_DWORD(REAL_TO_UDINT(( FrcVal_"+signalName+" + "+this->dutHeader+"."+signalName+".Param_Off) / "+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= UDINT_TO_DWORD(REAL_TO_UDINT(("+this->dutHeader+"."+signalName+".x + "+this->dutHeader+"."+signalName+".Param_Off) / "+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN	"
            "\n 	Cont_"+signalName+":= 16#FF000001;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 	Cont_"+signalName+":= 16#FE000001;"
            "\n 	ELSE"
            "\n 	Cont_"+signalName+":= 16#FE000001;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= UDINT_TO_DWORD(REAL_TO_UDINT(("+this->dutHeader+"."+signalName+".Param_Def + "+this->dutHeader+"."+signalName+".Param_Off) /"+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");
        }
    }else if((length<65)){
        if((converType == "toLWORD")|| (converType == "xtoLWORD")){
            ST.append(""+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= FrcVal_"+signalName+";"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= "+this->dutHeader+"."+signalName+".x ;"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN	"
            "\n 	Cont_"+signalName+":= 16#FF00000000000001;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 	Cont_"+signalName+":= 16#FE00000000000001;"
            "\n 	ELSE"
            "\n 	Cont_"+signalName+":= 16#FE00000000000001;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= "+this->dutHeader+"."+signalName+".Param_Def ;"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");
        }
        else if ((converType == "toULINT")|| (converType == "xtoULINT")){
            ST.append("<"+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= ULINT_TO_LWORD(FrcVal_"+signalName+");"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= ULINT_TO_LWORD("+this->dutHeader+"."+signalName+".x );"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN	"
            "\n 	Cont_"+signalName+":= 16#FF00000000000001;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 	Cont_"+signalName+":= 16#FE00000000000001;"
            "\n 	ELSE"
            "\n 	Cont_"+signalName+":= 16#FE00000000000001;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= ULINT_TO_LWORD("+this->dutHeader+"."+signalName+".Param_Def );"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");
        }
        else if ((converType == "toLREAL")|| (converType == "xtoLREAL")){
            ST.append(""+this->dutHeader+"."+signalName+".RangeExcd :=  "+this->dutHeader+"."+signalName+".Param_Max < "+this->dutHeader+"."+signalName+".x OR "+this->dutHeader+"."+signalName+".Param_Min > "+this->dutHeader+"."+signalName+".x;"
            "\n //Transmit Data : Range Control End"
            "\n IF FrcEn_"+signalName+" THEN"
            "\n 	//Transmit Data : Force variable Start"
            "\n 	Cont_"+signalName+"	:= ULINT_TO_LWORD(LREAL_TO_ULINT(( FrcVal_"+signalName+" + "+this->dutHeader+"."+signalName+".Param_Off) / "+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	//Transmit Data : Force variable Start"
            "\n ELSIF ("+this->dutHeader+"."+signalName+".v AND NOT "+this->dutHeader+"."+signalName+".RangeExcd) THEN"
            "\n 	//Transmit Data : Normal Start"
            "\n 	Cont_"+signalName+"	:= ULINT_TO_LWORD(LREAL_TO_ULINT(("+this->dutHeader+"."+signalName+".x + "+this->dutHeader+"."+signalName+".Param_Off) / "+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	 //Transmit Data : Normal End"
            "\n ELSE"
            "\n 	IF "+this->dutHeader+"."+signalName+".J1939 THEN"
            "\n 	//Transmit Data : Data not valid J1939 error transmission Start"
            "\n 	IF "+this->dutHeader+"."+signalName+".na THEN	"
            "\n 	Cont_"+signalName+":= 16#FF00000000000001;"
            "\n 	ELSIF "+this->dutHeader+"."+signalName+".e THEN"
            "\n 	Cont_"+signalName+":= 16#FE00000000000001;"
            "\n 	ELSE"
            "\n 	Cont_"+signalName+":= 16#FE00000000000001;"
            "\n 	END_IF;"
            "\n 	//Transmit Data : Data not valid J1939 error transmission End"
            "\n 	ELSE"
            "\n 	//Transmit Data : Data not valid  Start"
            "\n 		Cont_"+signalName+":= ULINT_TO_LWORD(LREAL_TO_ULINT(("+this->dutHeader+"."+signalName+".Param_Def + "+this->dutHeader+"."+signalName+".Param_Off) /"+this->dutHeader+"."+signalName+".Param_Res));"
            "\n 	//Transmit Data : Data not valid  End"
            "\n 	END_IF;"
            "\n END_IF;");
        }
    }


    if(converType=="BOOL:BOOL"){
        ST.append("\nS_IO_Bit_"+QString::number(startbit)+"               :=	("+this->dutHeader+"."+signalName+".x OR FrcHi_"+signalName+") AND NOT FrcLo_"+signalName+" AND "+this->dutHeader+"."+signalName+".v ;");
    }else if(converType=="2BOOL:BOOL"){
        ST.append("\nS_IO_Bit_"+QString::number(startbit)+"               :=	("+this->dutHeader+"."+signalName+".x OR FrcHi_"+signalName+") AND NOT FrcLo_"+signalName+";"
                  "\nS_IO_Bit_"+QString::number(startbit+1)+"             :=	(NOT "+this->dutHeader+"."+signalName+".v ) AND NOT (FrcHi_"+signalName+" OR FrcLo_"+signalName+"); ");
    }else if((converType=="xtoBYTE")||(converType=="xtoUSINT")||((converType=="xtoREAL") && (length==8))){
        ST.append("\nX_IO_BYTE_"+QString::number(startbit/8)+"            :=Cont_"+signalName+";");
    }else if((converType=="xtoWORD")||(converType=="xtoUINT")||((converType=="xtoREAL") && (length==16))){
        ST.append("\n_FB_UNPACK_WORD_TO_BYTE_"+QString::number(counterfbWORDTOBYTE)+"(X_WORD_0:= Cont_"+signalName+", X_BYTE_0=> X_IO_BYTE_"+QString::number(startbit/8)+", X_BYTE_1=> X_IO_BYTE_"+QString::number((startbit/8)+1)+");");
        counterfbWORDTOBYTE++;
    }else if((converType=="xtoDWORD")||(converType=="xtoUDINT")||((converType=="xtoREAL") && (length==32))){
        ST.append("\n_FB_UNPACK_DWORD_TO_BYTE_"+QString::number(counterfbDWORDTOBYTE)+"(X_DWORD_0:= Cont_"+signalName+", X_BYTE_0=> X_IO_BYTE_"+QString::number(startbit/8)+", X_BYTE_1=> X_IO_BYTE_"+QString::number((startbit/8)+1)+", X_BYTE_2=>X_IO_BYTE_"+QString::number((startbit/8)+2)+", X_BYTE_3=>X_IO_BYTE_"+QString::number((startbit/8)+3)+");");
        counterfbDWORDTOBYTE++;
    }else if((converType=="xtoLWORD")||(converType=="xtoULINT")||(converType=="xtoLREAL") ){
        ST.append("\n_FB_UNPACK_LWORD_TO_BYTE_"+QString::number(counterfbDWORDTOBYTE)+"(X_LWORD_0:= Cont_"+signalName+", X_BYTE_0=> X_IO_BYTE_"+QString::number(startbit/8)+", X_BYTE_1=> X_IO_BYTE_"+QString::number((startbit/8)+1)+", X_BYTE_2=> X_IO_BYTE_"+QString::number((startbit/8)+2)+", X_BYTE_3=> X_IO_BYTE_"+QString::number((startbit/8)+3)+", X_BYTE_4=> X_IO_BYTE_"+QString::number((startbit/8)+4)+", X_BYTE_5=> X_IO_BYTE_"+QString::number((startbit/8)+5)+", X_BYTE_6=> X_IO_BYTE_"+QString::number((startbit/8)+6)+", X_BYTE_7=> X_IO_BYTE_"+QString::number((startbit/8)+7)+");");
        counterfbLWORDTOBYTE++;
    }else{
        bool flagPack = false;
        if((length>8)){
            if(length<17){
            ST.append("\n_FB_UNPACK_WORD_TO_BYTE_"+QString::number(counterfbWORDTOBYTE)+"(X_WORD_0:=Cont_"+signalName+");");
            counterfbWORDTOBYTE++;
            }else if(length <33){
             ST.append("_FB_UNPACK_DWORD_TO_BYTE_"+QString::number(counterfbDWORDTOBYTE)+"(X_DWORD_0:=Cont_"+signalName+");");
             counterfbDWORDTOBYTE++;
            }else {
              ST.append("\n_FB_UNPACK_LWORD_TO_BYTE_"+QString::number(counterfbLWORDTOBYTE)+"(X_LWORD_0:=Cont_"+signalName+");");
              counterfbLWORDTOBYTE++;
           }
        }
        unsigned packID=0;
        unsigned packByteID=0;
        unsigned counterBITBYTE=0;
        unsigned counterBYTEWORD=0;
        unsigned counterBYTEDWORD=0;
        for(unsigned i =0; i<length;i++){

            if(((i%8 == 0) && ((i/8)<8) )&& (!flagPack)){
                ST.append("\n_FB_UNPACK_BYTE_TO_8BITS_"+QString::number(counterfbBYTETO8BIT+qFloor(i/8.0))+"(");
                counterBITBYTE++;
                flagPack=true;
            }
            if(flagPack){
                    ST.append("S_BIT_"+QString::number(i%8)+"=> S_IO_BIT_"+QString::number(startbit+i)+"");
                    if(((i%8 != 7) &&(i%(length-1) != 0)) || (i==0)) {
                        ST.append(",");
                    }
            }
            if(flagPack){
                if(((i%8 == 7)||(i%(length-1) == 0) )&& (i>0)){
                    ST.append(" ,X_BYTE_0:=");
                    if(length<9){
                    ST.append("Cont_"+signalName);
                    ST.append(");");
                    }else if (length <17){
                    ST.append("_FB_UNPACK_WORD_TO_BYTE_"+QString::number(counterfbWORDTOBYTE-1));
                    ST.append(".X_BYTE_"+QString::number(packByteID)+");");
                    }else if (length <33){
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

    ST.append("\n\n\n{endregion}\n\n\n");
    return ST;

}


void DBCHandler::generateHandlers(QDomElement *pous, QDomDocument &doc)
{
    QDomAttr attr;
    QDomText text;

    unsigned long counterJ1939=0;
    foreach (dataContainer * curMessage , comInterface){
        if(curMessage->getIfSelected())
        for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
            if(curSignal->isJ1939){
                counterJ1939++;
            }
        }
    }
    if(counterJ1939>1){

        /*ERROR HANDLER*/
        {
            structFbdBlock* newBlock = new structFbdBlock;
            newBlock->name="_FB_"+this->dutHeader+"_ERR_Handler";
            QDomElement pou = doc.createElement("pou");
            /*set pou name*/
            attr=doc.createAttribute("name");
            attr.setValue("_FB_"+this->dutHeader+"_ERR_Handler");
            pou.setAttributeNode(attr);
            /*set pouType*/
            attr=doc.createAttribute("pouType");
            attr.setValue("functionBlock");
            pou.setAttributeNode(attr);

            QDomElement interface = doc.createElement("interface");
            /*Input Variables*/
            QDomElement inputVars = doc.createElement("inputVars");

            foreach (dataContainer * curMessage , comInterface){
                if(curMessage->getIfSelected())
                for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                    if(curSignal->isJ1939){

                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("ERR_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"ERR_"+curSignal->name,this->enableTest?"FrcTest."+curSignal->name+".e":"ASSIGN_ERROR_TRANSMISSION_SIGNAL","BOOL"," "});
                    }
                }
            }
            interface.appendChild(inputVars);

            /*Generate Output Input Variables - inoutVars*/
            QDomElement inoutVars = doc.createElement("inOutVars");
            {
                QDomElement variable = doc.createElement("variable");
                attr=doc.createAttribute("name");
                attr.setValue(dutHeader);
                variable.setAttributeNode(attr);
                QDomElement type = doc.createElement("type");
                QDomElement dataVarType = doc.createElement("derived");
                attr=doc.createAttribute("name");
                attr.setValue(dutName);
                dataVarType.setAttributeNode(attr);
                type.appendChild(dataVarType);
                variable.appendChild(type);
                inoutVars.appendChild(variable);
            };
            interface.appendChild(inoutVars);

            pou.appendChild(interface);

            QDomElement body = doc.createElement("body");
            QDomElement ST = doc.createElement("ST");
            QDomElement xhtml = doc.createElement("xhtml");
            attr=doc.createAttribute("xmlns");
            attr.setValue("http://www.w3.org/1999/xhtml");
            xhtml.setAttributeNode(attr);
            QString handlerText="";
            handlerText.append("(*\n"
                               "**********************************************************************\n"
                               "Bozankaya A.Ş.\n"
                               "**********************************************************************\n"
                               "Name					: _FB_"+this->dutHeader+"_ERR_Handler\n"
                               "POU Type				: FB\n"
                               "Created by              : COMMUNICATION INTERFACE GENERATOR , BZK.\n"
                               "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
                               "Modifications			: see version description below\n"
                               "\n"
                               "\n"
                               "FB Description:"
                               "This FB which is created by automatically by communication interface generator \n"
                               "\n to assign error status of signals to actual signal object. \n"
                               "*********************************************************************\n"
                               "\n"
                               "Version 1	\n"
                               "*********************************************************************\n"
                               "Version Description:\n"
                               "- initial version\n"
                               "*********************************************************************\n"
                               "*)");
            /*ST text of Error Handler*/
            foreach (dataContainer * curMessage , comInterface){
                if(curMessage->getIfSelected())
                for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                    if(curSignal->isJ1939){
                        handlerText.append("\n"+this->dutHeader+ "."+curSignal->name+ ".e 					:=	ERR_"+curSignal->name+ ";");
                    }
                }
            }
            /*ST text of Error Handler*/
            text=doc.createTextNode(handlerText);
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

            for(QList<QString> curVal : this->fbNameandObjId){
                if(curVal.at(0)== ("_FB_"+this->dutHeader+"_ERR_Handler")){
                    text=doc.createTextNode(curVal.at(1));
                }
            }

            ObjectId.appendChild(text);
            data.appendChild(ObjectId);
            addData.appendChild(data);
            pou.appendChild(addData);
            pous->appendChild(pou);
            fbdBlocks.append(newBlock);
        }

        /*NA HANDLER*/
        {
            structFbdBlock* newBlock = new structFbdBlock;
            newBlock->name="_FB_"+this->dutHeader+"_NA_Handler";
            QDomElement pou = doc.createElement("pou");
            /*set pou name*/
            attr=doc.createAttribute("name");
            attr.setValue("_FB_"+this->dutHeader+"_NA_Handler");
            pou.setAttributeNode(attr);
            /*set pouType*/
            attr=doc.createAttribute("pouType");
            attr.setValue("functionBlock");
            pou.setAttributeNode(attr);

            QDomElement interface = doc.createElement("interface");
            /*Input Variables*/
            QDomElement inputVars = doc.createElement("inputVars");

            foreach (dataContainer * curMessage , comInterface){
                if(curMessage->getIfSelected())
                for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                    if(curSignal->isJ1939){

                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("NA_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"NA_"+curSignal->name,this->enableTest?"FrcTest."+curSignal->name+".na":"ASSIGN_NOT_AVAILABLITY_SIGNAL","BOOL"," "});
                    }
                }
            }
            interface.appendChild(inputVars);

            /*Generate Output Input Variables - inoutVars*/
            QDomElement inoutVars = doc.createElement("inOutVars");
            {
                QDomElement variable = doc.createElement("variable");
                attr=doc.createAttribute("name");
                attr.setValue(dutHeader);
                variable.setAttributeNode(attr);
                QDomElement type = doc.createElement("type");
                QDomElement dataVarType = doc.createElement("derived");
                attr=doc.createAttribute("name");
                attr.setValue(dutName);
                dataVarType.setAttributeNode(attr);
                type.appendChild(dataVarType);
                variable.appendChild(type);
                inoutVars.appendChild(variable);
            };
            interface.appendChild(inoutVars);

            pou.appendChild(interface);

            QDomElement body = doc.createElement("body");
            QDomElement ST = doc.createElement("ST");
            QDomElement xhtml = doc.createElement("xhtml");
            attr=doc.createAttribute("xmlns");
            attr.setValue("http://www.w3.org/1999/xhtml");
            xhtml.setAttributeNode(attr);
            QString handlerText="";
            /*ST text of Validity Handler*/
            handlerText.append("(*\n"
                               "**********************************************************************\n"
                               "Bozankaya A.Ş.\n"
                               "**********************************************************************\n"
                               "Name					: _FB_"+this->dutHeader+"_NA_Handler\n"
                               "POU Type				: FB\n"
                               "Created by              : COMMUNICATION INTERFACE GENERATOR , BZK.\n"
                               "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
                               "Modifications			: see version description below\n"
                               "\n"
                               "\n"
                               "FB Description:"
                               "This FB which is created by automatically by communication interface generator \n"
                               "\n to assign not available status of signals to actual signal object. \n"
                               "*********************************************************************\n"
                               "\n"
                               "Version 1	\n"
                               "*********************************************************************\n"
                               "Version Description:\n"
                               "- initial version\n"
                               "*********************************************************************\n"
                               "*)");
            foreach (dataContainer * curMessage , comInterface){
                if(curMessage->getIfSelected())
                for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                    if(curSignal->isJ1939){
                        handlerText.append("\n"+this->dutHeader+ "."+curSignal->name+ ".na 					:=	NA_"+curSignal->name+ ";");
                    }
                }
            }
            /*ST text of Validty Handler*/
            text=doc.createTextNode(handlerText);
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

            foreach(QList<QString> curVal , this->fbNameandObjId){
                if(curVal.at(0)== ("_FB_"+this->dutHeader+"_NA_Handler")){
                    text=doc.createTextNode(curVal.at(1));
                }
            }

            ObjectId.appendChild(text);
            data.appendChild(ObjectId);
            addData.appendChild(data);
            pou.appendChild(addData);
            pous->appendChild(pou);
            fbdBlocks.append(newBlock);
        }
        }else{
        dataContainer::setWarning("INFO","J1939 mesajı bulunamadığı için NA  ve ERR Handler yerleştirilmedi");
        emit infoListChanged();
        }
        /*VALITIY HANDLER*/
        {
            structFbdBlock* newBlock = new structFbdBlock;
            newBlock->name="_FB_"+this->dutHeader+"_Validity_Handler";
            QDomElement pou = doc.createElement("pou");
            /*set pou name*/
            attr=doc.createAttribute("name");
            attr.setValue("_FB_"+this->dutHeader+"_Validity_Handler");
            pou.setAttributeNode(attr);
            /*set pouType*/
            attr=doc.createAttribute("pouType");
            attr.setValue("functionBlock");
            pou.setAttributeNode(attr);

            QDomElement interface = doc.createElement("interface");
            /*Input Variables*/
            QDomElement inputVars = doc.createElement("inputVars");

            foreach (dataContainer * curMessage , comInterface){
                if(curMessage->getIfSelected())
                for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){


                        QDomElement variable = doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue("VALID_"+curSignal->name);
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement BOOL = doc.createElement("BOOL");
                        type.appendChild(BOOL);
                        variable.appendChild(type);
                        inputVars.appendChild(variable);
                        newBlock->inputVars.append({"VALID_"+curSignal->name,this->enableTest?"FrcTest."+curSignal->name+".v":"ASSIGN_VALIDITY_SIGNAL","BOOL"," "});


                }
            }
            interface.appendChild(inputVars);

            /*Generate Output Input Variables - inoutVars*/
            QDomElement inoutVars = doc.createElement("inOutVars");
            {
                QDomElement variable = doc.createElement("variable");
                attr=doc.createAttribute("name");
                attr.setValue(dutHeader);
                variable.setAttributeNode(attr);
                QDomElement type = doc.createElement("type");
                QDomElement dataVarType = doc.createElement("derived");
                attr=doc.createAttribute("name");
                attr.setValue(dutName);
                dataVarType.setAttributeNode(attr);
                type.appendChild(dataVarType);
                variable.appendChild(type);
                inoutVars.appendChild(variable);
            };
            interface.appendChild(inoutVars);

            pou.appendChild(interface);

            QDomElement body = doc.createElement("body");
            QDomElement ST = doc.createElement("ST");
            QDomElement xhtml = doc.createElement("xhtml");
            attr=doc.createAttribute("xmlns");
            attr.setValue("http://www.w3.org/1999/xhtml");
            xhtml.setAttributeNode(attr);
            QString handlerText="";
            handlerText.append("(*\n"
                               "**********************************************************************\n"
                               "Bozankaya A.Ş.\n"
                               "**********************************************************************\n"
                               "Name					: _FB_"+this->dutHeader+"_Validity_Handler\n"
                               "POU Type				: FB\n"
                               "Created by              : COMMUNICATION INTERFACE GENERATOR , BZK.\n"
                               "Creation Date 			: "+creationDate.toString(Qt::DateFormat::ISODate)+"\n"
                               "Modifications			: see version description below\n"
                               "\n"
                               "\n"
                               "FB Description:"
                               "This FB which is created by automatically by communication interface generator \n"
                               "\n to assign validity status of signals to actual signal object. \n"
                               "*********************************************************************\n"
                               "\n"
                               "Version 1	\n"
                               "*********************************************************************\n"
                               "Version Description:\n"
                               "- initial version\n"
                               "*********************************************************************\n"
                               "*)");
            /*ST text of validity Handler*/
            foreach (dataContainer * curMessage , comInterface){
                if(curMessage->getIfSelected())
                for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                        handlerText.append("\n"+this->dutHeader+ "."+curSignal->name+ ".v					:=	VALID_"+curSignal->name+ ";");
                }
            }
            /*ST text of Error Handler*/
            text=doc.createTextNode(handlerText);
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

            for(QList<QString> curVal : this->fbNameandObjId){
                if(curVal.at(0)== ("_FB_"+this->dutHeader+"_Validity_Handler")){
                    text=doc.createTextNode(curVal.at(1));
                }
            }

            ObjectId.appendChild(text);
            data.appendChild(ObjectId);
            addData.appendChild(data);
            pou.appendChild(addData);
            pous->appendChild(pou);
            fbdBlocks.append(newBlock);

        }

        /*OUTPUT HANDLER*/
    {
                structFbdBlock* newBlock = new structFbdBlock;
                newBlock->name="_FB_"+this->dutHeader+"_Output_Handler";
                QDomElement pou = doc.createElement("pou");
                /*set pou name*/
                attr=doc.createAttribute("name");
                attr.setValue("_FB_"+this->dutHeader+"_Output_Handler");
                pou.setAttributeNode(attr);
                /*set pouType*/
                attr=doc.createAttribute("pouType");
                attr.setValue("functionBlock");
                pou.setAttributeNode(attr);

                QDomElement interface = doc.createElement("interface");
                /*Input Variables*/
                QDomElement inputVars = doc.createElement("inputVars");

                foreach (dataContainer * curMessage , comInterface){
                    if(curMessage->getIfSelected())
                    for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){


                            QDomElement variable = doc.createElement("variable");
                            attr=doc.createAttribute("name");
                            attr.setValue("VALUE_"+curSignal->name);
                            variable.setAttributeNode(attr);
                            QDomElement type = doc.createElement("type");
                            QDomElement BOOL = doc.createElement(curSignal->appDataType);
                            type.appendChild(BOOL);
                            variable.appendChild(type);
                            inputVars.appendChild(variable);
                            newBlock->inputVars.append({"VALUE_"+curSignal->name,this->enableTest?"FrcTest."+curSignal->name+".x":"ASSIGN_VARIABLE_OF_SIGNAL",curSignal->appDataType," "});

                    }
                }
                interface.appendChild(inputVars);

                /*Generate Output Input Variables - inoutVars*/
                QDomElement inoutVars = doc.createElement("inOutVars");
                {
                    QDomElement variable = doc.createElement("variable");
                    attr=doc.createAttribute("name");
                    attr.setValue(dutHeader);
                    variable.setAttributeNode(attr);
                    QDomElement type = doc.createElement("type");
                    QDomElement dataVarType = doc.createElement("derived");
                    attr=doc.createAttribute("name");
                    attr.setValue(dutName);
                    dataVarType.setAttributeNode(attr);
                    type.appendChild(dataVarType);
                    variable.appendChild(type);
                    inoutVars.appendChild(variable);
                };
                interface.appendChild(inoutVars);

                pou.appendChild(interface);

                QDomElement body = doc.createElement("body");
                QDomElement ST = doc.createElement("ST");
                QDomElement xhtml = doc.createElement("xhtml");
                attr=doc.createAttribute("xmlns");
                attr.setValue("http://www.w3.org/1999/xhtml");
                xhtml.setAttributeNode(attr);
                QString handlerText="";
                /*ST text of NA Handler*/
                foreach (dataContainer * curMessage , comInterface){
                    if(curMessage->getIfSelected())
                    for(const dataContainer::signal * curSignal : *curMessage->getSignalList()){
                            handlerText.append("\n"+this->dutHeader+ "."+curSignal->name+ ".x					:=	VALUE_"+curSignal->name+ ";");
                    }
                }
                /*ST text of Error Handler*/
                text=doc.createTextNode(handlerText);
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

                for(QList<QString> curVal : this->fbNameandObjId){
                    if(curVal.at(0)== ("_FB_"+this->dutHeader+"_Output_Handler")){
                        text=doc.createTextNode(curVal.at(1));
                    }
                }

                ObjectId.appendChild(text);
                data.appendChild(ObjectId);
                addData.appendChild(data);
                pou.appendChild(addData);
                pous->appendChild(pou);
                fbdBlocks.append(newBlock);

            }
}

void DBCHandler::generatePouFpd(QDomElement *pous, QDomDocument &doc)
{
    QDomAttr attr;
    QDomText text;
    /*set pou name*/
    QDomElement pou = doc.createElement("pou");
    attr=doc.createAttribute("name");
    attr.setValue("P_"+this->dutHeader);
    pou.setAttributeNode(attr);
    /*set pouType*/
    attr=doc.createAttribute("pouType");
    attr.setValue("program");
    pou.setAttributeNode(attr);



    {/*INTERFACE*/

        QDomElement interface = doc.createElement("interface");

        {/*LOCAL VARS*/
                QDomElement localVars = doc.createElement("localVars");

                for(DBCHandler::structFbdBlock * curStruct : fbdBlocks){
                    if(curStruct->name != "AND" && curStruct->name != "OR"){
                        QDomElement variable=doc.createElement("variable");
                        attr=doc.createAttribute("name");
                        attr.setValue(curStruct->name+"_0");
                        variable.setAttributeNode(attr);
                        QDomElement type = doc.createElement("type");
                        QDomElement derived = doc.createElement("derived");
                        attr=doc.createAttribute("name");
                        attr.setValue(curStruct->name);
                        derived.setAttributeNode(attr);
                        type.appendChild(derived);
                        variable.appendChild(type);
                        localVars.appendChild(variable);
                    }
                }

            {
                QDomElement variable = doc.createElement("variable");
                attr=doc.createAttribute("name");
                attr.setValue("FrcHi");
                variable.setAttributeNode(attr);
                QDomElement type = doc.createElement("type");
                QDomElement dataVarType = doc.createElement("derived");
                attr=doc.createAttribute("name");
                attr.setValue(dutName);
                dataVarType.setAttributeNode(attr);
                type.appendChild(dataVarType);
                variable.appendChild(type);
                localVars.appendChild(variable);
            }
            {
                QDomElement variable = doc.createElement("variable");
                attr=doc.createAttribute("name");
                attr.setValue("FrcLo");
                variable.setAttributeNode(attr);
                QDomElement type = doc.createElement("type");
                QDomElement dataVarType = doc.createElement("derived");
                attr=doc.createAttribute("name");
                attr.setValue(dutName);
                dataVarType.setAttributeNode(attr);
                type.appendChild(dataVarType);
                variable.appendChild(type);
                localVars.appendChild(variable);
            }
            if(this->enableTest){
                QDomElement variable = doc.createElement("variable");
                attr=doc.createAttribute("name");
                attr.setValue("FrcTest");
                variable.setAttributeNode(attr);
                QDomElement type = doc.createElement("type");
                QDomElement dataVarType = doc.createElement("derived");
                attr=doc.createAttribute("name");
                attr.setValue(dutName);
                dataVarType.setAttributeNode(attr);
                type.appendChild(dataVarType);
                variable.appendChild(type);
                localVars.appendChild(variable);
            }
            interface.appendChild(localVars);
        }
        pou.appendChild(interface);
    }
/*BODY*/
    {
        QDomElement body = doc.createElement("body");
        /*FBD*/
        {
            QDomElement FBD = doc.createElement("FBD");

            {
                const unsigned long long  baseLocalID= 10000000000;
                unsigned long long countLocalID = 0;
                unsigned long long countNetworkID = 1;
                {
                    QDomElement vendorElement = doc.createElement("vendorElement");
                    {
                        QDomElement position = doc.createElement("position");
                        attr=doc.createAttribute("x");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        attr=doc.createAttribute("y");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        vendorElement.appendChild(position);
                    }
                    {
                        QDomElement alternativeText = doc.createElement("alternativeText");
                        QDomElement xhtml = doc.createElement("xhtml");
                        attr=doc.createAttribute("xmlns");
                        attr.setValue("http://www.w3.org/1999/xhtml");
                        xhtml.setAttributeNode(attr);
                        text=doc.createTextNode("FBD Implementation Attributes");
                        xhtml.appendChild(text);
                        alternativeText.appendChild(xhtml);
                        vendorElement.appendChild(alternativeText);
                    }
                    {
                        QDomElement addData = doc.createElement("addData");
                        QDomElement data = doc.createElement("data");
                        attr= doc.createAttribute("name");
                        attr.setValue("http://www.3s-software.com/plcopenxml/fbd/implementationattributes");
                        data.setAttributeNode(attr);
                        attr=doc.createAttribute("handleUnknown");
                        attr.setValue("implementation");
                        data.setAttributeNode(attr);
                        QDomElement fbdattributes=doc.createElement("fbdattributes");
                        attr=doc.createAttribute("xmlns");
                        attr.setValue("");
                        fbdattributes.setAttributeNode(attr);
                        QDomElement attribute =doc.createElement("attribute");
                        attr=doc.createAttribute("name");
                        attr.setValue("BoxInputFlagsSupported");
                        attribute.setAttributeNode(attr);
                        attr=doc.createAttribute("value");
                        attr.setValue("true");
                        attribute.setAttributeNode(attr);
                        fbdattributes.appendChild(attribute);
                        data.appendChild(fbdattributes);
                        addData.appendChild(data);
                        vendorElement.appendChild(addData);
                    }
                    attr=doc.createAttribute("localId");
                    attr.setValue(QString::number(baseLocalID));
                    vendorElement.setAttributeNode(attr);
                    FBD.appendChild(vendorElement);
                }

                {
                    QDomElement comment = doc.createElement("comment");

                    {
                        QDomElement position = doc.createElement("position");
                        attr=doc.createAttribute("x");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        attr=doc.createAttribute("y");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        attr=doc.createAttribute("height");
                        attr.setValue("0");
                        comment.setAttributeNode(attr);
                        attr=doc.createAttribute("width");
                        attr.setValue("0");
                        comment.setAttributeNode(attr);
                        comment.appendChild(position);

                    }

                    {
                        QDomElement content = doc.createElement("content");
                        QDomElement xhtml = doc.createElement("xhtml");
                        attr=doc.createAttribute("xmlns");
                        attr.setValue("http://www.w3.org/1999/xhtml");
                        xhtml.setAttributeNode(attr);
                        QString creationHeader="THIS POU AUTOMATICALLY GENERATED BY INTERFACE GENERATOR AT"+creationDate.toString(Qt::DateFormat::ISODate);
                        text=doc.createTextNode(creationHeader);
                        xhtml.appendChild(text);
                        content.appendChild(xhtml);
                        comment.appendChild(content);

                    }
                    attr=doc.createAttribute("localId");
                    attr.setValue(QString::number(baseLocalID+1));
                    comment.setAttributeNode(attr);
                    FBD.appendChild(comment);
                }

                countNetworkID++;


                for(DBCHandler::structFbdBlock * curStruct : fbdBlocks){

                    QString dutHeaderLocalId;

                    for(QList<QString>  curVar : curStruct->inputVars){


                        QDomElement inVariable = doc.createElement("inVariable");
                        QDomElement position = doc.createElement("position");
                        QDomElement connectionPointOut = doc.createElement("connectionPointOut");
                        QDomElement expression = doc.createElement("expression");
                        attr=doc.createAttribute("x");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        attr=doc.createAttribute("y");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        inVariable.appendChild(position);
                        inVariable.appendChild(connectionPointOut);
                        text=doc.createTextNode(curVar.at(1));
                        expression.appendChild(text);
                        inVariable.appendChild(expression);
                        attr=doc.createAttribute("localId");
                        attr.setValue(QString::number((baseLocalID*countNetworkID)+countLocalID));
                        inVariable.setAttributeNode(attr);
                        FBD.appendChild(inVariable);
                        countLocalID++;
                    }

                    if(curStruct->name != "AND" && curStruct->name != "OR")
                    {
                        QDomElement inVariable = doc.createElement("inVariable");
                        QDomElement position = doc.createElement("position");
                        QDomElement connectionPointOut = doc.createElement("connectionPointOut");
                        QDomElement expression = doc.createElement("expression");
                        attr=doc.createAttribute("x");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        attr=doc.createAttribute("y");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        inVariable.appendChild(position);
                        inVariable.appendChild(connectionPointOut);
                        text=doc.createTextNode("GVL."+this->dutHeader);
                        expression.appendChild(text);
                        inVariable.appendChild(expression);
                        attr=doc.createAttribute("localId");
                        attr.setValue(QString::number((baseLocalID*countNetworkID)+countLocalID));
                        dutHeaderLocalId=QString::number((baseLocalID*countNetworkID)+countLocalID);
                        inVariable.setAttributeNode(attr);
                        FBD.appendChild(inVariable);
                        countLocalID++;
                    }
                    {
                        QDomElement block = doc.createElement("block");
                        attr=doc.createAttribute("localId");
                        attr.setValue(QString::number((baseLocalID*countNetworkID)+countLocalID));
                        block.setAttributeNode(attr);
                        attr=doc.createAttribute("typeName");
                        attr.setValue(curStruct->name);
                        block.setAttributeNode(attr);
                        //Add instaceName if only It is user derived fb, dont add for OR,AND etc.
                        if(curStruct->name != "AND" && curStruct->name != "OR")
                        {
                            attr=doc.createAttribute("instanceName");
                            attr.setValue(curStruct->name+"_0");
                            block.setAttributeNode(attr);
                        }

                        QDomElement position = doc.createElement("position");
                        attr=doc.createAttribute("x");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        attr=doc.createAttribute("y");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        block.appendChild(position);


                        {
                        unsigned long long localxId =0;
                        QDomElement inputVariables = doc.createElement("inputVariables");
                            foreach(QList<QString> curVar , curStruct->inputVars){

                                QDomElement variable = doc.createElement("variable");
                                attr=doc.createAttribute("formalParameter");
                                attr.setValue(curVar.at(0));
                                variable.setAttributeNode(attr);
                                QDomElement connectionPointIn = doc.createElement("connectionPointIn");
                                QDomElement connection = doc.createElement("connection");
                                attr=doc.createAttribute("refLocalId");
                                attr.setValue(QString::number((baseLocalID*countNetworkID)+localxId));
                                connection.setAttributeNode(attr);
                                connectionPointIn.appendChild(connection);
                                variable.appendChild(connectionPointIn);
                                inputVariables.appendChild(variable);

                                localxId++;
                            }
                            block.appendChild(inputVariables);

                        QDomElement inoutVariables = doc.createElement("inOutVariables");

                        if(curStruct->name != "AND" && curStruct->name != "OR"){
                            QDomElement variable = doc.createElement("variable");
                            attr=doc.createAttribute("formalParameter");
                            attr.setValue(this->dutHeader);
                            variable.setAttributeNode(attr);
                            QDomElement connectionPointIn = doc.createElement("connectionPointIn");
                            QDomElement connection = doc.createElement("connection");
                            attr=doc.createAttribute("refLocalId");
                            attr.setValue(dutHeaderLocalId);
                            connection.setAttributeNode(attr);
                            connectionPointIn.appendChild(connection);
                            variable.appendChild(connectionPointIn);
                            inoutVariables.appendChild(variable);

                            }
                        block.appendChild(inoutVariables);
                        QDomElement outputVariables = doc.createElement("outputVariables");
                            foreach(QList<QString> curVar , curStruct->outputVars){

                                QDomElement variable = doc.createElement("variable");
                                attr=doc.createAttribute("formalParameter");
                                attr.setValue(curVar.at(0));
                                variable.setAttributeNode(attr);
                                //Add negated attribute if only It is OR or AND gate
                                if(curStruct->name == "AND" || curStruct->name == "OR")
                                {
                                    attr=doc.createAttribute("negated");
                                    attr.setValue("true");
                                    variable.setAttributeNode(attr);
                                }
                                QDomElement connectionPointIn = doc.createElement("connectionPointOut");
                                variable.appendChild(connectionPointIn);
                                outputVariables.appendChild(variable);

                            }
                             block.appendChild(outputVariables);
                        }
                        /*ADDDATA OF Block*/
                        QDomElement addData = doc.createElement("addData");

                        {
                            {
                            QDomElement data = doc.createElement("data");
                            attr=doc.createAttribute("name");
                            attr.setValue("http://www.3s-software.com/plcopenxml/fbdcalltype");
                            data.setAttributeNode(attr);
                            attr=doc.createAttribute("handleUnknown");
                            attr.setValue("implementation");
                            data.setAttributeNode(attr);
                            QDomElement CallType = doc.createElement("CallType");
                            attr=doc.createAttribute("xmlns");
                            attr.setValue("");
                            CallType.setAttributeNode(attr);
                            text=doc.createTextNode("functionblock");
                            CallType.appendChild(text);
                            data.appendChild(CallType);
                            addData.appendChild(data);
                            }
                            {
                            QDomElement data = doc.createElement("data");
                            attr=doc.createAttribute("name");
                            attr.setValue("http://www.3s-software.com/plcopenxml/inputparamtypes");
                            data.setAttributeNode(attr);
                            attr=doc.createAttribute("handleUnknown");
                            attr.setValue("implementation");
                            data.setAttributeNode(attr);
                            QDomElement InputParamTypes = doc.createElement("InputParamTypes");
                            attr=doc.createAttribute("xmlns");
                            attr.setValue("");
                            InputParamTypes.setAttributeNode(attr);
                            QString dataTypes="";
                            foreach(QList<QString> curVar , curStruct->inputVars){
                            dataTypes.append(curVar.at(2)+" ");
                            }
                            text=doc.createTextNode(dataTypes);
                            InputParamTypes.appendChild(text);
                            data.appendChild(InputParamTypes);
                            addData.appendChild(data);
                            }
                            {
                            QDomElement data = doc.createElement("data");
                            attr=doc.createAttribute("name");
                            attr.setValue("http://www.3s-software.com/plcopenxml/outputparamtypes");
                            data.setAttributeNode(attr);
                            attr=doc.createAttribute("handleUnknown");
                            attr.setValue("implementation");
                            data.setAttributeNode(attr);
                            QDomElement OutputParamTypes = doc.createElement("OutputParamTypes");
                            attr=doc.createAttribute("xmlns");
                            attr.setValue("");
                            OutputParamTypes.setAttributeNode(attr);
                            QString dataTypes="";
                            foreach(QList<QString> curVar , curStruct->outputVars){
                            dataTypes.append(curVar.at(2)+" ");
                            }
                            text=doc.createTextNode(dataTypes);
                            OutputParamTypes.appendChild(text);
                            data.appendChild(OutputParamTypes);
                            addData.appendChild(data);
                            }
                        }



                        block.appendChild(addData);
                        FBD.appendChild(block);
                        countLocalID++;
                    }

                    foreach(QList<QString> curVar , curStruct->outputVars){


                        QDomElement outVariable = doc.createElement("outVariable");
                        QDomElement position = doc.createElement("position");
                        QDomElement connectionPointIn = doc.createElement("connectionPointIn");
                        QDomElement connection = doc.createElement("connection");
                        QDomElement expression = doc.createElement("expression");
                        text=doc.createTextNode(curVar.at(1));
                        expression.appendChild(text);
                        attr=doc.createAttribute("x");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        attr=doc.createAttribute("y");
                        attr.setValue("0");
                        position.setAttributeNode(attr);
                        outVariable.appendChild(position);
                        attr=doc.createAttribute("refLocalId");
                        attr.setValue(QString::number((baseLocalID*countNetworkID)+countLocalID-1));
                        connection.setAttributeNode(attr);
                        attr=doc.createAttribute("formalParameter ");
                        attr.setValue(curVar.at(1));
                        connection.setAttributeNode(attr);
                        connectionPointIn.appendChild(connection);
                        outVariable.appendChild(connectionPointIn);
                        outVariable.appendChild(expression);
                        attr=doc.createAttribute("localId");
                        attr.setValue(QString::number((baseLocalID*countNetworkID)+countLocalID));
                        outVariable.setAttributeNode(attr);
                        FBD.appendChild(outVariable);
                        countLocalID++;

                    }

                    countLocalID=0;
                    countNetworkID++;
                }


            }



            body.appendChild(FBD);
        }
        pou.appendChild(body);
    }

/*ADDDATA*/
    {
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
        text=doc.createTextNode(pouObjID);
        ObjectId.appendChild(text);
        data.appendChild(ObjectId);
        addData.appendChild(data);

        pou.appendChild(addData);
    }
    pous->appendChild(pou);
    foreach(DBCHandler::structFbdBlock * curStruct , fbdBlocks){
        delete curStruct;
    }
    fbdBlocks.clear();
}

