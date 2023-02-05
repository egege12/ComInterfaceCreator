#include "datacontainer.h"
#include <QDebug>

unsigned int dataContainer::messageCounter = 0;
unsigned int dataContainer::signalCounter = 0;
dataContainer::dataContainer(QObject *parent)
{
    ++dataContainer::messageCounter;
    this->isInserted=false;
    this->isSelected = false;
}

bool dataContainer::addSignal(signal newSignal)
{
    ++signalCounter;
    signal *Ptr = new signal;
    *Ptr = newSignal;
    dataTypeAss(Ptr);
    this->signalList.append(Ptr);
    return true;
}

const QList<dataContainer::signal *> *dataContainer::getSignalList()
{
    return &signalList;
}

QString dataContainer::getName()
{
    return this->Name;
}

QString dataContainer::getID()
{
    return this->messageID;
}



bool dataContainer::getIfSelected()
{
    return this->isSelected;
}

unsigned short dataContainer::getDLC()
{
    return this->dlc;
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

void dataContainer::setSelected()
{
    this->isSelected = true;
}

void dataContainer::setInserted()
{
    this->isInserted = true;
}

void dataContainer::dataTypeAss(signal *signalPtr)
{
    if(signalPtr->length == 1 ){
        signalPtr->appDataType = "BOOL";
        signalPtr->comDataType = "BOOL";
    }else if (signalPtr->length == 2){
        signalPtr->comDataType = "BOOL";
        if((signalPtr->isJ1939) || signalPtr->name.contains("C_") || signalPtr->name.contains("S_")){
            signalPtr->appDataType = "BOOL";
        }else{
            signalPtr->appDataType = "BYTE";
        }
    }else if (signalPtr->length < 8){
        signalPtr->comDataType = signalPtr->length + QString("XBOOL");
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "REAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "USINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "BYTE";
        }
    }else if (signalPtr->length == 8){
        signalPtr->comDataType = "BYTE";
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "REAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "USINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "BYTE";
        }
    }else if (signalPtr->length <  16){
        signalPtr->comDataType = signalPtr->length + QString("XBOOL");
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "REAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "UINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "WORD";
        }
    }else if (signalPtr->length == 16){
        signalPtr->comDataType = "WORD";
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "REAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "UINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "WORD";
        }
    }else if (signalPtr->length < 32){
        signalPtr->comDataType = signalPtr->length + QString("XBOOL");
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "REAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "UDINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "DWORD";
        }
    }else if (signalPtr->length == 32){
        signalPtr->comDataType = "DWORD";
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "REAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "UDINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "DWORD";
        }
    }else if (signalPtr->length < 64){
        signalPtr->comDataType = signalPtr->length + QString("XBOOL");
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "LREAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "ULINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "LWORD";
        }
    }else if (signalPtr->length == 64){
        signalPtr->comDataType = "LWORD";
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "LREAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "ULINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "LWORD";
        }
        else{
            signalPtr->appDataType = "****(*CHECK IDENTIFIER OR DATA LENGTH*)";
            signalPtr->comDataType = " ";
        }
    }
}


dataContainer::~dataContainer()
{
    --dataContainer::messageCounter;
    signalCounter = 0;
    for(signal * curSignal : signalList){
        delete curSignal;
    }
    signalList.clear();
}
