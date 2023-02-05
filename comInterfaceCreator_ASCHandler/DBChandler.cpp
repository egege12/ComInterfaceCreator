#include "DBChandler.h"

DBCHandler::DBCHandler(QObject *parent, QString fileLocation)
    : QObject{parent}
{
    try {
        if (fileLocation.isEmpty()){
            throw QString("File location cant be empty");
        }else if(!fileLocation.contains(".dbc")){
            throw QString("Please select \".dbc\" file to parse messages and signals");
        }
         else{
            QFile *ascFile = new QFile(fileLocation);
            if(!ascFile->open(QIODevice::ReadOnly | QIODevice::Text)){
                throw QString("File can not be opened, please check if the path or name is correct");
            }
            else{
                if (!parseMessages(ascFile)){
                throw QString("something goes wrong about asc file");
                }
            }
        }
    } catch (QString text) {
        qInfo()<<text;
    }
}

void DBCHandler::printMessages()
{
    foreach(dataContainer *const curValue , comInterface){
        qInfo()<< curValue->getMessageVector();
    }
    qInfo()<<"Total message number :"<< dataContainer::messageCounter;
    qInfo()<<"Total signal number :"<< dataContainer::signalCounter;
}

void DBCHandler::printSelectedMessages()
{
    foreach(dataContainer *const curValue , comInterface){
        if(curValue->getIfSelected()){
            qInfo()<< curValue->getMessageVector();
        }
    }

}

QList<QList<QString>> DBCHandler::messagesVector()
{
    QList<QList<QString>> data;
    foreach(dataContainer *const curValue , comInterface){
            data.append(curValue->getMessageVector());
    }
    return data;
}

QList<QList<QString> > DBCHandler::signalsVector(QString messageID)
{
    return comInterface.value(messageID)->getSignalVector();
}

bool DBCHandler::selectMessage(QString messageID)
{
    return comInterface.value(messageID)->setSelected();
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
            generateNewMessage(messageID,messageName,messageDLC);
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
            comInterface.value(messageID)->printAll();
        }
        inlineOfMessageOld = inlineOfMessage;
    }


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


