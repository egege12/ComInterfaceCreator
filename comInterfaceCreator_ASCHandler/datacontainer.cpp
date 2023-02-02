#include "datacontainer.h"
#include <QDebug>



dataContainer::dataContainer(QObject *parent)
{

}

bool dataContainer::addSignal(signal newSignal)
{
    signal *Ptr = new signal;
    *Ptr = newSignal;
    this->message.append(Ptr);
    return true;
}

void dataContainer::printAll()
{
    qInfo()<<"_Message Name:"<<this->Name<<"ID"<<this->messageID<<" DLC:"<<this->dlc<< (isExtended ? "extended":"standard");
    qInfo()<<"Signal Name:\t"<< "Length:\t"<< "Start Bit\t"<<"Resolution:\t"<<"Offset:\t"<<"Max Value\t"<<"Min Value\t"<<"Com Type\t"<<"App Type\t"<<"Conv Type\t"<<"Comment\t";
    for(signal * curSignal : message){
         qInfo()<< curSignal->name <<"\t"<< curSignal->length <<"\t"<< curSignal->startBit<<"\t"<<curSignal->resolution<<"\t"<<curSignal->offset<<"\t"<<curSignal->maxValue<<"\t"<<curSignal->minValue<<"\t"<<curSignal->comDataType<<"\t"<<curSignal->appDataType<<"\t"<<curSignal->convDataType<<"\t"<<curSignal->comment;
    }
}

void dataContainer::setName(QString Name)
{
    this->Name = Name;

}

void dataContainer::setmessageID(QString messageID)
{

    bool isExtended = (messageID.toULong()> 1000);
    this->messageID = messageID;
    this->isExtended = isExtended;

}

void dataContainer::setDLC(unsigned short DLC)
{
    this->dlc= DLC;
}

dataContainer::~dataContainer()
{
    for(signal * curSignal : message){
        delete curSignal;
    }
    message.clear();
}
