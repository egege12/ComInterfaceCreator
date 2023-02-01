#include "datacontainer.h"
#include <QDebug>

struct dataContainer::signal{
    //Datas can be impoerted from DBC file
    QString name;
    unsigned int length;
    unsigned int startBit;
    double resolution;
    double offset;
    double maxValue;
    double minValue;
    QString appDataType;
    QString comDataType;
    QString convDataType;
    bool isJ1939;

};

dataContainer::dataContainer(QObject *parent, QString Name, QString messageID, unsigned int dlc, bool isExtended)
    : QObject{parent}
{
    this->Name = Name;
    this->messageID = messageID;
    this->dlc= dlc;
    this->isExtended=isExtended;
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
    qInfo()<<"Signal Name: \t"<< "Length:\t"<< "Start Bit\t"<<"Resolution:\t"<<"Offset:\t"<<"Max Value\t"<<"Min Value"<<"Com Type"<<"App Type"<<"Conv Type";
    for(signal * curSignal : message){
         qInfo()<< curSignal->name <<"\t"<< curSignal->length <<"\t"<< curSignal->startBit<<"\t"<<curSignal->resolution<<"\t"<<curSignal->offset<<"\t"<<curSignal->maxValue<<"\t"<<curSignal->minValue<<"\t"<<curSignal->comDataType<<"\t"<<curSignal->appDataType<<"\t"<<curSignal->convDataType;
    }
}

dataContainer::~dataContainer()
{
    for(signal * curSignal : message){
        delete curSignal;
    }
    message.clear();
}
