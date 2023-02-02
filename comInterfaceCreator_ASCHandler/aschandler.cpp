#include "aschandler.h"

ASCHandler::ASCHandler(QObject *parent, QString fileLocation)
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
//BO_ <ID> <Message_name>: <DLC> Vector__XXX
// 3. ADRESTEN BOŞLUĞA KADAR ID DECİMAL
// Boşluktan ":"'e kadar mesaj ismi
// Boşluktan boşluğa kadar DLC numarası
//SG_ <Name> : <Bit order> | <Bit length>@1+ (<resolution>,<offset>) [<min_value>|max_value] "comment"
bool ASCHandler::parseMessages(QFile *ascFile)
{
    QTextStream lines(ascFile);
    bool inlineOfMessage=false;

    QString *messageID = nullptr;
    QString *messageName = nullptr;
    unsigned short * messageDLC = nullptr;

    while (!lines.atEnd()) {
         QString curLine = lines.readLine();

//Message parse - split message line to items and rise message block flasg (inlineOfMessage)
        if(curLine.contains("BO_ ") && curLine.indexOf("BO_") < 2){
            inlineOfMessage = true;

            QStringList messageList = curLine.split(" ");

            messageID = new QString(QString::number(messageList.at(1).toUInt(),16).toUpper());
            messageName= new QString(messageList.at(2));
            messageName->remove(QChar(':'),Qt::CaseInsensitive);
            messageDLC = new unsigned short(messageList.at(3).toUShort());
            generateNewMessage(*messageID,*messageName,*messageDLC);
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
            QString commentContainer = parseComment(signalList.at(7));
            curSignal.comment = commentContainer;
            curSignal.isJ1939 = commentContainer.contains(QString("j1939"),Qt::CaseInsensitive);
            addSignalToMessage (*messageID,curSignal);

        }else
            inlineOfMessage = false;


    }
    delete messageID;
    delete messageName;
    delete messageDLC;

    return true;
}

bool ASCHandler::generateNewMessage(QString messageID, QString messageName , unsigned short messageDLC)
{
    dataContainer* newMessage = new dataContainer();
    newMessage->setName(messageName);
    newMessage->setmessageID(messageID);
    newMessage->setDLC(messageDLC);
    comInterface.insert(messageID,newMessage);
    return true;
}

bool ASCHandler::addSignalToMessage(QString messageID,dataContainer::signal curSignal)
{
    comInterface.value(messageID)->addSignal(curSignal);
    comInterface.value(messageID)->printAll();
    return true;
}

unsigned short ASCHandler::parseLength(QString splitedPart)
{
    splitedPart.remove("@1+");
    splitedPart.remove("@1-");
    QStringList container = splitedPart.trimmed().split("|");
    return container.at(1).toUShort();
}

unsigned short ASCHandler::parseStartBit(QString  splitedPart)
{
    splitedPart.remove("@1+");
    splitedPart.remove("@1-");
    QStringList container = splitedPart.trimmed().split("|");
    return container.at(0).toUShort();
}

double ASCHandler::parseResolution(QString  splitedPart)
{
    splitedPart.remove("(");
    splitedPart.remove(")");
    QStringList container = splitedPart.trimmed().split(",");
    return container.at(0).toDouble();
}

double ASCHandler::parseOffset(QString  splitedPart)
{
    splitedPart.remove("(");
    splitedPart.remove(")");
    QStringList container = splitedPart.trimmed().split(",");
    return container.at(1).toDouble();
}

double ASCHandler::parseMaxValue(QString  splitedPart)
{
    splitedPart.remove("[");
    splitedPart.remove("]");
    QStringList container = splitedPart.trimmed().split("|");
    return container.at(1).toDouble();
}

double ASCHandler::parseMinValue(QString  splitedPart)
{
    splitedPart.remove("[");
    splitedPart.remove("]");
    QStringList container = splitedPart.trimmed().split("|");
    return container.at(0).toDouble();
}

QString ASCHandler::parseComment(QString splitedPart)
{
    splitedPart.remove("\\");
    splitedPart.remove("//");
    splitedPart.remove("\"");

    return splitedPart.trimmed();
}

