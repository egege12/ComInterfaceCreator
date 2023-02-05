#include "DBChandler.h"
#include "datacontainer.h"


DBCHandler::DBCHandler(QObject *parent)
    : QObject{parent}
{

}


QList<QList<QString>> DBCHandler::messagesVector()
{   if (isAllInserted){
        QList<QList<QString>> data;
        data.append({"Name","ID","DLC","Status"});
        foreach(dataContainer *const curValue , comInterface){
            data.append({curValue->getName(),curValue->getID(),QString::number(curValue->getDLC()),curValue->getIfSelected() ? "X" : "O" });
        }
        return data;
    }
}

QList<QList<QString> > DBCHandler::signalsVector(QString messageID)
{
    if (isAllInserted){

        QList<QList<QString>> dataSignal;

        for ( const dataContainer::signal *data : *comInterface.value(messageID)->getSignalList()){
            dataSignal.append({data->name,QString::number(data->startBit),QString::number(data->length),QString::number(data->resolution),QString::number(data->offset),QString::number(data->minValue),QString::number(data->maxValue),data->appDataType});
        }
        return dataSignal;
    }
}

bool DBCHandler::selectMessage(QString messageID)
{
    comInterface.value(messageID)->setSelected();

    return comInterface.value(messageID)->getIfSelected();
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

void DBCHandler::readFile(QString fileLocation)
{
    dbcPath = fileLocation;
    openFile();
}

void DBCHandler::openFile()
{
    try {
        if (dbcPath.isEmpty()){
            throw QString("File location cant be empty");
        }else if(!dbcPath.contains(".dbc")){
            throw QString("Please select \".dbc\" file to parse messages and signals");
        }
        else{
            QFile *ascFile = new QFile(dbcPath);
            if(!ascFile->open(QIODevice::ReadOnly | QIODevice::Text)){
                throw QString("File can not be opened, please check if the path or name is correct");
            }
            else{
                if (!parseMessages(ascFile)){
                    throw QString("something goes wrong about asc file");
                }else{
                    QObject::connect(&watcher, SIGNAL(fileChanged(dbcPath)), this, SLOT(DBCHandler::update()));
                    qInfo()<< "watcher:" << watcher.addPath(dbcPath);


                }
            }
        }
    } catch (QString text) {
        qInfo()<<text;
    }
}

const dataContainer *DBCHandler::getMessage(QString messageID)
{

    return comInterface.value(messageID);
}

bool DBCHandler::parseMessages(QFile *ascFile)
{
    QTextStream lines(ascFile);
    bool inlineOfMessage=false;
    bool inlineOfMessageOld=false;
    QString messageID;
    QString messageName ;
    unsigned short  messageDLC ;

    while (!lines.atEnd()) {
        QString curLine = lines.readLine();

        //Message parse - split message line to items and rise message block flasg (inlineOfMessage)
        if(curLine.contains("BO_ ") && curLine.indexOf("BO_") < 2){
            inlineOfMessage = true;

            QStringList messageList = curLine.split(" ");

            messageID = QString::number(messageList.at(1).toUInt(),16).toUpper();
            messageName= messageList.at(2);
            messageName.remove(QChar(':'),Qt::CaseInsensitive);
            messageDLC = messageList.at(3).toUShort();
            if (!comInterface.contains(messageID)){
                generateNewMessage(messageID,messageName,messageDLC);
            }
            // Release pointer locations

            //Signal parse - split signal lines to items
        }else if(inlineOfMessage && curLine.contains("SG_")){
            QStringList signalList = curLine.split(" ");
            dataContainer::signal curSignal;
            curSignal.name = signalList.at(2);
            curSignal.startBit = parseStartBit(signalList.at(4));
            curSignal.length = parseLength(signalList.at(4));
            curSignal.resolution = parseResolution(signalList.at(5));
            curSignal.offset = parseOffset(signalList.at(5));
            curSignal.minValue = parseMinValue(signalList.at(6));
            curSignal.maxValue = parseMaxValue(signalList.at(6));
            QString commentContainer = parseComment(curLine);
            curSignal.comment = commentContainer;
            curSignal.isJ1939 = commentContainer.contains(QString("j1939"),Qt::CaseInsensitive);
            addSignalToMessage (messageID,curSignal);

        }else{
            inlineOfMessage = false;
        }
        if (inlineOfMessageOld && !inlineOfMessage){
            comInterface.value(messageID)->setInserted();
        }
        inlineOfMessageOld = inlineOfMessage;
    }
    this->isAllInserted = true;

    return true;
}
//BO_ <ID> <Message_name>: <DLC> Vector__XXX -> for messages
//SG_ <Name> : <Bit order> | <Bit length>@1+ (<resolution>,<offset>) [<min_value>|max_value] "comment" -> for signals

bool DBCHandler::generateNewMessage(QString messageID, QString messageName , unsigned short messageDLC)
{
    dataContainer* newMessage = new dataContainer();
    newMessage->setName(messageName);
    newMessage->setmessageID(messageID);
    newMessage->setDLC(messageDLC);
    comInterface.insert(messageID,newMessage);
    return true;
}

bool DBCHandler::addSignalToMessage(QString messageID,dataContainer::signal curSignal)
{
    comInterface.value(messageID)->addSignal(curSignal);
    return true;
}

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
    QString comment = splitedPart.mid(splitedPart.indexOf("]")+1,(splitedPart.indexOf("Vector__XXX")));
    comment.remove("\"");
    comment.remove("Vector__XXX");
    return comment.trimmed();
}


