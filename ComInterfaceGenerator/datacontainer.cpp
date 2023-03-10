#include "datacontainer.h"
#include <QDebug>
#include <QDateTime>
#include <QtMath>
unsigned long long dataContainer::messageCounter = 0;
unsigned long long dataContainer::signalCounter = 0;
QMap<QString,QList<QString>> dataContainer::warningMessages ={};
QList<QString> dataContainer::infoMessages ={};


dataContainer::dataContainer(QObject *parent)
{
    ++dataContainer::messageCounter;
    this->isInserted=false;
    this->isSelected = false;
    this->isCycleTmSet = false;
    this->isTmOutSet = false;
    this->isNotSelectable=false;
    this->msTimeout ="2500";
    this->msCycleTime ="100";
    this->comment="No Comment";
}

bool dataContainer::addSignal(signal newSignal)
{
    ++signalCounter;
    signal *Ptr = new signal;
    *Ptr = newSignal;
    dataTypeAss(Ptr);
    signalChecker(Ptr);
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

QString dataContainer::getMsTimeOut()
{
    return this->msTimeout;
}

QString dataContainer::getMsCycleTime()
{
    return this->msCycleTime;
}

QString dataContainer::getComment()
{
    return this->comment;
}


bool dataContainer::getIfSelected()
{
    return this->isSelected;
}

bool dataContainer::getIfExtended()
{
    return this->isExtended;
}

bool dataContainer::getIfNotSelectable()
{
    return this->isNotSelectable;
}

unsigned short dataContainer::getDLC()
{
    return this->dlc;
}

const QList<QString> dataContainer::getWarningList()
{
    QList<QString> warningMessagesAll;
    foreach(QList <QString> warningMessages , dataContainer::warningMessages){

        warningMessagesAll.append(warningMessages);

    }
    return warningMessagesAll;
}

const QList<QString> dataContainer::getMsgWarningList(QString ID)
{
    return warningMessages.value(ID);
}

const QList<QString> dataContainer::getInfoList()
{
    return dataContainer::infoMessages;
}
void dataContainer::setName(QString Name)
{
    this->Name = Name;

}

void dataContainer::setmessageID(QString messageID)
{
    this->messageID = messageID;
}

void dataContainer::setDLC(unsigned short DLC)
{
    this->dlc= DLC;
}

void dataContainer::setSelected()
{
    this->isSelected = !this->isSelected;
}

void dataContainer::setNotSelectable()
{
    this->isNotSelectable=true;
}

void dataContainer::setInserted()
{
    this->isInserted = true;
}

void dataContainer::setMsTimeOut(QString msTimeout)
{
    this->msTimeout = msTimeout;
    this->isTmOutSet=true;
}

void dataContainer::setMsCycleTime(QString msCycleTime)
{
    this->msCycleTime = msCycleTime;
    this->isCycleTmSet=true;
}

void dataContainer::setComment(QString comment)
{
    this->comment = comment;
}

void dataContainer::setExtended(bool isExtended)
{
    this->isExtended=isExtended;
}

void dataContainer::setWarning(QString ID,const QString &warningCode)
{
    if(ID == ("INFO") || (ID == ("info") )){
        dataContainer::infoMessages.append(QDateTime::currentDateTime().toString("dd.mm.yy - hh:mm:ss.zzz")+": "+warningCode);
    }else{
        if(warningMessages.contains(ID)){
        dataContainer::warningMessages[ID].append("Uyar??: Mesaj=>"+ID+":"+warningCode);
        }else{
            QList<QString> temporary = {};
            temporary.append("Uyar??: Mesaj=>"+ID+":"+warningCode);
        dataContainer::warningMessages.insert(ID,temporary);
        }
    }
}

void dataContainer::dataTypeAss(signal *signalPtr)
{
    if(signalPtr->length == 1 ){
        signalPtr->appDataType = "BOOL";
        signalPtr->comDataType = "BOOL";
        signalPtr->convMethod="BOOL:BOOL";
    }else if (signalPtr->length == 2){ 
        if((signalPtr->isJ1939) || signalPtr->name.contains("C_") || signalPtr->name.contains("S_")){
            signalPtr->appDataType = "BOOL";
            signalPtr->convMethod="2BOOL:BOOL";
            signalPtr->comDataType = "BOOL";
        }else if (signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "BYTE";
            signalPtr->convMethod="toBYTE";
            signalPtr->comDataType = "BYTE";
        }else{
            signalPtr->appDataType="BYTE";
            signalPtr->convMethod="toBYTE";
            signalPtr->comDataType = "BYTE";
            this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde C_ S_ Z_ i??areti bulunmuyor");
        }
    }else if (signalPtr->length < 8){
        signalPtr->comDataType = "BYTE";
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
            signalPtr->appDataType = "REAL";
            signalPtr->convMethod="toREAL";
        }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
            signalPtr->appDataType = "REAL";
            signalPtr->convMethod="toREAL";
            this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "USINT";
            signalPtr->convMethod="toUSINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "BYTE";
            signalPtr->convMethod="toBYTE";
        }else{
            signalPtr->appDataType="BYTE";
            signalPtr->convMethod="toBYTE";
            this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");
        }
    this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katlar?? de??il,standart olmayan veri transferi");
    }else if (signalPtr->length == 8){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )||(signalPtr->startBit == 40 )||(signalPtr->startBit == 48 )||(signalPtr->startBit == 56 )){
            signalPtr->comDataType = "BYTE";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "USINT";
                signalPtr->convMethod="xtoUSINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "BYTE";
                signalPtr->convMethod="xtoBYTE";
            }else{
                signalPtr->appDataType="BYTE";
                signalPtr->convMethod="xtoBYTE";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");
            }
        }else{
            signalPtr->comDataType = "BYTE";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "USINT";
                signalPtr->convMethod="toUSINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "BYTE";
                signalPtr->convMethod="toBYTE";
            }else{
                signalPtr->appDataType="BYTE";
                signalPtr->convMethod="toBYTE";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali ba??lang???? biti 8 ve katlar?? de??il, d??????k performans");

        }
    }else if (signalPtr->length <  16){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )||(signalPtr->startBit == 40 )||(signalPtr->startBit == 48 )){
            signalPtr->comDataType = "WORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UINT";
                signalPtr->convMethod="toUINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "WORD";
                signalPtr->convMethod="toWORD";
            }else{
                signalPtr->appDataType="WORD";
                signalPtr->convMethod="toWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katlar?? de??il,standart olmayan veri transferi");
        }else{
            signalPtr->comDataType = "WORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UINT";
                signalPtr->convMethod="toUINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "WORD";
                signalPtr->convMethod="toWORD";
            }else{
                signalPtr->appDataType="WORD";
                signalPtr->convMethod="toWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katlar?? de??il,standart olmayan veri transferi");
            this->setWarning(this->messageID,signalPtr->name+" sinyali ba??lang???? biti 8 ve katlar?? de??il,d??????k perfomans");
        }
    }else if (signalPtr->length == 16){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )||(signalPtr->startBit == 40 )||(signalPtr->startBit == 48 )){
            signalPtr->comDataType = "WORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UINT";
                signalPtr->convMethod="xtoUINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->convMethod="xtoWORD";
            }else{
                signalPtr->appDataType="WORD";
                signalPtr->convMethod="xtoWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");
            }
        }else{
            signalPtr->comDataType = "WORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UINT";
                signalPtr->convMethod="toUINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "WORD";
                signalPtr->convMethod="toWORD";
            }else{
                signalPtr->appDataType="WORD";
                signalPtr->convMethod="toWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali ba??lang???? biti 16 ve katlar?? de??il, d??????k performans");
        }
    }else if (signalPtr->length < 32){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )){
            signalPtr->comDataType = "DWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UDINT";
                signalPtr->convMethod="toUDINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "DWORD";
                signalPtr->convMethod="toDWORD";
            }else{
                signalPtr->appDataType="DWORD";
                signalPtr->convMethod="toDWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katlar?? de??il,standart olmayan veri transferi");
        }else{
            signalPtr->comDataType = "DWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UDINT";
                signalPtr->convMethod="toUDINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "DWORD";
                signalPtr->convMethod="toDWORD";
            }else{
                signalPtr->appDataType="DWORD";
                signalPtr->convMethod="toDWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katlar?? de??il,standart olmayan veri transferi");
            this->setWarning(this->messageID,signalPtr->name+" sinyali ba??lang???? biti 8 ve katlar?? de??il,d??????k perfomans");
        }

    }else if (signalPtr->length == 32){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )){
            signalPtr->comDataType = "DWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UDINT";
                signalPtr->convMethod="xtoUDINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "DWORD";
                signalPtr->convMethod="xtoDWORD";
            }else{
                signalPtr->appDataType="DWORD";
                signalPtr->convMethod="xtoDWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");
            }
        }else{
            signalPtr->comDataType = "DWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UDINT";
                signalPtr->convMethod="toUDINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "DWORD";
                signalPtr->convMethod="toDWORD";
            }else{
                signalPtr->appDataType="DWORD";
                signalPtr->convMethod="toDWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali ba??lang???? biti 8 ve katlar?? de??il,d??????k performans");
        }
    }else if (signalPtr->length < 64){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )){
            signalPtr->comDataType = "LWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "LREAL";
                signalPtr->convMethod="toLREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "LREAL";
                signalPtr->convMethod="toLREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "ULINT";
                signalPtr->convMethod="toULINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "LWORD";
                signalPtr->convMethod="toLWORD";
            }else{
                signalPtr->appDataType="LWORD";
                signalPtr->convMethod="toLWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");
            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katlar?? de??il,standart olmayan veri transferi");
        }else{
            signalPtr->comDataType = "LWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")||(signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "LREAL";
                signalPtr->convMethod="toLREAL";
            }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
                signalPtr->appDataType = "LREAL";
                signalPtr->convMethod="toLREAL";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "ULINT";
                signalPtr->convMethod="toULINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "LWORD";
                signalPtr->convMethod="toLWORD";
            }else{
                signalPtr->appDataType="LWORD";
                signalPtr->convMethod="toLWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");
            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katlar?? de??il,standart olmayan veri transferi");
            this->setWarning(this->messageID,signalPtr->name+" sinyali ba??lang???? biti 8 ve katlar?? de??il,d??????k perfomans");
        }
    }else if (signalPtr->length == 64){
        signalPtr->comDataType = "LWORD";
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "LREAL";
            signalPtr->convMethod="xtoLREAL";
        }else if((signalPtr->resolution != 1)||(signalPtr->offset != 0)){
            signalPtr->appDataType = "LREAL";
            signalPtr->convMethod="xtoLREAL";
            this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ veya W_ i??areti bulunmal??");
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "ULINT";
            signalPtr->convMethod="xtoULINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "LWORD";
            signalPtr->convMethod="xtoLWORD";
        }else{
            signalPtr->appDataType = "LWORD";
            this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ i??areti bulunmuyor");
        }
    }
}


dataContainer::~dataContainer()
{
    --dataContainer::messageCounter;
    for(signal * curSignal : signalList){
        delete curSignal;
        --dataContainer::signalCounter;
    }
    signalList.clear();
}

void dataContainer::signalChecker(signal *signalPtr)
{
    if(signalPtr->length == 8){
        if(signalPtr ->isJ1939){
            if( signalPtr ->offset != 0 ){
                if(signalPtr -> minValue < signalPtr ->offset) {
                    signalPtr->minValue =  signalPtr ->offset ;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??erine ofset de??eri atand??.");
                }
            }else{
                if(signalPtr -> minValue < 0) {
                    signalPtr -> offset = -250* signalPtr ->resolution/2;
                    signalPtr -> minValue = -250* signalPtr ->resolution/2;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??er ve ofset de??eri atand??.");
                }
            }
            if(signalPtr ->maxValue > 250 * signalPtr ->resolution + signalPtr ->offset ){
                this->setWarning(this->messageID,signalPtr->name+" sinyali J1939 olarak tan??mlanm???? ancak maksimum de??eri ERR ve NA tan??m aral??????nda, maksimum de??eri 250*??l??ek olarak atand??.");
                signalPtr->maxValue =  250 * signalPtr ->resolution + signalPtr ->offset;
            }

        }else{
            if( signalPtr ->offset != 0 ){
                if(signalPtr -> minValue < signalPtr ->offset) {
                    signalPtr->minValue =  signalPtr ->offset ;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??erine ofset de??eri atand??.");
                }
            }else{
                if(signalPtr -> minValue < 0) {
                    signalPtr -> offset = -254* signalPtr ->resolution/2;
                    signalPtr -> minValue = -254* signalPtr ->resolution/2;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??er ve ofset de??eri atand??.");
                }
            }
            if(signalPtr ->maxValue > 254 * signalPtr ->resolution + signalPtr ->offset){
                this->setWarning(this->messageID,signalPtr->name+" sinyali maksimum de??eri ??l??ek ve ofset d??????nda, 254*??L??EK-OFSET yap??ld??");
                signalPtr->maxValue =  254 * signalPtr ->resolution + signalPtr ->offset;
            }
        }
    }else if(signalPtr->length == 16){
        if(signalPtr ->isJ1939){
            if( signalPtr ->offset != 0 ){
                if(signalPtr -> minValue < signalPtr ->offset) {
                    signalPtr->minValue =  signalPtr ->offset ;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??erine ofset de??eri atand??.");
                }
            }else{
                if(signalPtr -> minValue < 0) {
                    signalPtr -> offset = -64255* signalPtr ->resolution/2;
                    signalPtr -> minValue = -64255* signalPtr ->resolution/2;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??er ve ofset de??eri atand??.");
                }
            }
            if(signalPtr ->maxValue > 64255 * signalPtr ->resolution + signalPtr ->offset ){
                this->setWarning(this->messageID,signalPtr->name+" sinyali J1939 olarak tan??mlanm???? ancak maksimum de??eri ERR ve NA tan??m aral??????nda, maksimum de??eri 64255*??l??ek olarak atand??.");
                signalPtr->maxValue =  64255 * signalPtr ->resolution + signalPtr ->offset;
            }
        }else{
            if( signalPtr ->offset != 0 ){
                if(signalPtr -> minValue < signalPtr ->offset) {
                    signalPtr->minValue =  signalPtr ->offset ;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??erine ofset de??eri atand??.");
                }
            }else{
                if(signalPtr -> minValue < 0) {
                    signalPtr -> offset = -65535* signalPtr ->resolution/2;
                    signalPtr -> minValue = -65535* signalPtr ->resolution/2;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??er ve ofset de??eri atand??.");
                }
            }
            if(signalPtr ->maxValue > 65535 * signalPtr ->resolution + signalPtr ->offset){
                this->setWarning(this->messageID,signalPtr->name+" sinyali maksimum de??eri ??l??ek ve ofset d??????nda, 65534*??L??EK-OFSET yap??ld??");
                signalPtr->maxValue =  65534 * signalPtr ->resolution + signalPtr ->offset;
            }
        }
    }else if(signalPtr->length == 32){
        if(signalPtr ->isJ1939){
            if( signalPtr ->offset != 0 ){
                if(signalPtr -> minValue < signalPtr ->offset) {
                    signalPtr->minValue =  signalPtr ->offset ;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??erine ofset de??eri atand??.");
                }
            }else{
                if(signalPtr -> minValue < 0) {
                    signalPtr -> offset = -4211081215* signalPtr ->resolution/2;
                    signalPtr -> minValue = -4211081215* signalPtr ->resolution/2;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??er ve ofset de??eri atand??.");
                }
            }
            if(signalPtr ->maxValue > 4211081215 * signalPtr ->resolution + signalPtr ->offset ){
                this->setWarning(this->messageID,signalPtr->name+" sinyali J1939 olarak tan??mlanm???? ancak maksimum de??eri ERR ve NA tan??m aral??????nda, maksimum de??eri 4211081215*??l??ek olarak atand??.");
                signalPtr->maxValue =  4211081215 * signalPtr ->resolution + signalPtr ->offset;
            }
        }else{
            if( signalPtr ->offset != 0 ){
                if(signalPtr -> minValue < signalPtr ->offset) {
                    signalPtr->minValue =  signalPtr ->offset ;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??erine ofset de??eri atand??.");
                }
            }else{
                if(signalPtr -> minValue < 0) {
                    signalPtr -> offset = -4294967294* signalPtr ->resolution/2;
                    signalPtr -> minValue = -4294967294* signalPtr ->resolution/2;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??er ve ofset de??eri atand??.");
                }
            }
            if(signalPtr ->maxValue > 4294967294 * signalPtr ->resolution + signalPtr ->offset){
                this->setWarning(this->messageID,signalPtr->name+" sinyali maksimum de??eri ??l??ek ve ofset d??????nda, 4294967294*??L??EK-OFSET yap??ld??");
                signalPtr->maxValue =  4294967294 * signalPtr ->resolution + signalPtr ->offset;
            }
        }
    }else{
        if(signalPtr->isJ1939){
            if( signalPtr ->offset != 0 ){
                if(signalPtr -> minValue < signalPtr ->offset) {
                    signalPtr->minValue =  signalPtr ->offset ;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??erine ofset de??eri atand??.");
                }
            }else{
                if(signalPtr -> minValue < 0) {
                    signalPtr -> offset = -(((qPow(2,signalPtr->length))-1) * 0.988598)* signalPtr ->resolution/2;
                    signalPtr -> minValue = -(((qPow(2,signalPtr->length))-1) * 0.988598)* signalPtr ->resolution/2;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??er ve ofset de??eri atand??.");
                }
            }
            if(signalPtr->maxValue > (((qPow(2,signalPtr->length))-1) * 0.988598)* signalPtr ->resolution + signalPtr ->offset){
                signalPtr->maxValue = (((qPow(2,signalPtr->length))-1) * 0.988598)* signalPtr ->resolution + signalPtr ->offset;
                this->setWarning(this->messageID,signalPtr->name+" sinyali maksimum de??eri olabilecek de??erden b??y??k oldu??u i??in m??mk??n maksimum de??er atand??.");
            }
        }else{
            if( signalPtr ->offset != 0 ){
                if(signalPtr -> minValue < signalPtr ->offset) {
                    signalPtr->minValue =  signalPtr ->offset ;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??erine ofset de??eri atand??.");
                }
            }else{
                if(signalPtr -> minValue < 0) {
                    signalPtr -> offset = -((qPow(2,signalPtr->length))-1)* signalPtr ->resolution/2;
                    signalPtr -> minValue = -((qPow(2,signalPtr->length))-1)* signalPtr ->resolution/2;
                    this->setWarning(this->messageID,signalPtr->name+" minimum de??er ve ofset de??eri atand??.");
                }
            }
            if(signalPtr->maxValue > ((qPow(2,signalPtr->length))-1) * signalPtr ->resolution + signalPtr ->offset){
                signalPtr->maxValue = ((qPow(2,signalPtr->length))-1) * signalPtr ->resolution + signalPtr ->offset;
                this->setWarning(this->messageID,signalPtr->name+" sinyali maksimum de??eri olabilecek de??erden b??y??k oldu??u i??in m??mk??n maksimum de??er atand??.");
            }
        }
    }



    if(signalPtr->length > (dlc*8 - signalPtr->startBit)){
        this->setWarning(this->messageID,signalPtr->name+" sinyali i??in veri boyutu tan??m aral??????ndan b??y??k.Mesaj OpenXML format?? d??n????t??r??lemez.");
        setNotSelectable();
    }
    if(signalPtr->startBit > dlc*8){
        this->setWarning(this->messageID,signalPtr->name+" sinyali i??in veri boyutu DLC tan??m aral??????ndan b??y??k.Mesaj OpenXML format?? d??n????t??r??lemez.");
        setNotSelectable();
    }
    if(signalPtr->length+signalPtr->startBit > dlc*8){
        this->setWarning(this->messageID,signalPtr->name+" sinyali DLC'yi ta????rd?????? mesaj OpenXML format?? d??n????t??r??lemez.");
        setNotSelectable();
    }
    if(dlc>8){
        this->setWarning(this->messageID,signalPtr->name+" LIB500 k??t??phanesi DLC 8'den b??y??k mesajlar i??in ??al????t??r??lamaz.Mesaj OpenXML format?? d??n????t??r??lemez.");
        setNotSelectable();
    }

    if(signalPtr->minValue > signalPtr->maxValue ){
        this->setWarning(this->messageID,signalPtr->name+" sinyali minimum de??eri maksimum de??erden b??y??k.Mesaj OpenXML format?? d??n????t??r??lemez. ");
        setNotSelectable();
    }
}
