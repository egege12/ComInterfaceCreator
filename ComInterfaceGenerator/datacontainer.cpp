#include "datacontainer.h"
#include <QDebug>
#include <QDateTime>
unsigned int dataContainer::messageCounter = 0;
unsigned int dataContainer::signalCounter = 0;
QMap<QString,QList<QString>> dataContainer::warningMessages ={};
QList<QString> dataContainer::infoMessages ={};
dataContainer::dataContainer(QObject *parent)
{
    ++dataContainer::messageCounter;
    this->isInserted=false;
    this->isSelected = false;
    this->isCycleTmSet = false;
    this->isTmOutSet = false;
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
    this->isSelected = !this->isSelected;
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

void dataContainer::setWarning(QString ID,const QString &warningCode)
{
    if(ID == ("INFO") || (ID == ("info") )){
        dataContainer::infoMessages.append(QDateTime::currentDateTime().toString("dd.mm.yy - hh:mm:ss.zzz")+": "+warningCode);
    }else{
        if(warningMessages.contains(ID)){
        dataContainer::warningMessages[ID].append("Uyarı: Mesaj=>"+ID+":"+warningCode);
        }else{
            QList<QString> temporary = {};
            temporary.append("Uyarı: Mesaj=>"+ID+":"+warningCode);
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
            this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde C_ S_ Z_ işareti bulunmuyor");
        }
    }else if (signalPtr->length < 8){
        signalPtr->comDataType = "BYTE";
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "REAL";
            signalPtr->convMethod="toREAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "USINT";
            signalPtr->convMethod="toUSINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "BYTE";
            signalPtr->convMethod="toBYTE";
        }else{
            signalPtr->appDataType="BYTE";
            signalPtr->convMethod="toBYTE";
            this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");
        }
    this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katları değil,standart olmayan veri transferi");
    }else if (signalPtr->length == 8){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )||(signalPtr->startBit == 40 )||(signalPtr->startBit == 48 )||(signalPtr->startBit == 56 )){
            signalPtr->comDataType = "BYTE";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "USINT";
                signalPtr->convMethod="xtoUSINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "BYTE";
                signalPtr->convMethod="xtoBYTE";
            }else{
                signalPtr->appDataType="BYTE";
                signalPtr->convMethod="xtoBYTE";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");
            }
        }else{
            signalPtr->comDataType = "BYTE";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "USINT";
                signalPtr->convMethod="toUSINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "BYTE";
                signalPtr->convMethod="toBYTE";
            }else{
                signalPtr->appDataType="BYTE";
                signalPtr->convMethod="toBYTE";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali başlangıç biti 8 ve katları değil, düşük performans");

        }
    }else if (signalPtr->length <  16){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )||(signalPtr->startBit == 40 )||(signalPtr->startBit == 48 )){
            signalPtr->comDataType = "WORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UINT";
                signalPtr->convMethod="toUINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "WORD";
                signalPtr->convMethod="toWORD";
            }else{
                signalPtr->appDataType="WORD";
                signalPtr->convMethod="toWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katları değil,standart olmayan veri transferi");
        }else{
            signalPtr->comDataType = "WORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UINT";
                signalPtr->convMethod="toUINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "WORD";
                signalPtr->convMethod="toWORD";
            }else{
                signalPtr->appDataType="WORD";
                signalPtr->convMethod="toWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katları değil,standart olmayan veri transferi");
            this->setWarning(this->messageID,signalPtr->name+" sinyali başlangıç biti 8 ve katları değil,düşük perfomans");
        }
    }else if (signalPtr->length == 16){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )||(signalPtr->startBit == 40 )||(signalPtr->startBit == 48 )){
            signalPtr->comDataType = "WORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UINT";
                signalPtr->convMethod="xtoUINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->convMethod="xtoWORD";
            }else{
                signalPtr->appDataType="WORD";
                signalPtr->convMethod="xtoWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");
            }
        }else{
            signalPtr->comDataType = "WORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UINT";
                signalPtr->convMethod="toUINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "WORD";
                signalPtr->convMethod="toWORD";
            }else{
                signalPtr->appDataType="WORD";
                signalPtr->convMethod="toWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali başlangıç biti 16 ve katları değil, düşük performans");
        }
    }else if (signalPtr->length < 32){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )){
            signalPtr->comDataType = "DWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UDINT";
                signalPtr->convMethod="toUDINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "DWORD";
                signalPtr->convMethod="toDWORD";
            }else{
                signalPtr->appDataType="DWORD";
                signalPtr->convMethod="toDWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katları değil,standart olmayan veri transferi");
        }else{
            signalPtr->comDataType = "DWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UDINT";
                signalPtr->convMethod="toUDINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "DWORD";
                signalPtr->convMethod="toDWORD";
            }else{
                signalPtr->appDataType="DWORD";
                signalPtr->convMethod="toDWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katları değil,standart olmayan veri transferi");
            this->setWarning(this->messageID,signalPtr->name+" sinyali başlangıç biti 8 ve katları değil,düşük perfomans");
        }

    }else if (signalPtr->length == 32){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )||(signalPtr->startBit == 32 )){
            signalPtr->comDataType = "DWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="xtoREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UDINT";
                signalPtr->convMethod="xtoUDINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "DWORD";
                signalPtr->convMethod="xtoDWORD";
            }else{
                signalPtr->appDataType="DWORD";
                signalPtr->convMethod="xtoDWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");
            }
        }else{
            signalPtr->comDataType = "DWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "REAL";
                signalPtr->convMethod="toREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "UDINT";
                signalPtr->convMethod="toUDINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "DWORD";
                signalPtr->convMethod="toDWORD";
            }else{
                signalPtr->appDataType="DWORD";
                signalPtr->convMethod="toDWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");

            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali başlangıç biti 8 ve katları değil,düşük performans");
        }
    }else if (signalPtr->length < 64){
        if((signalPtr->startBit == 0 )||(signalPtr->startBit == 8 )||(signalPtr->startBit == 16 )||(signalPtr->startBit == 24 )){
            signalPtr->comDataType = "LWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "LREAL";
                signalPtr->convMethod="toLREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "ULINT";
                signalPtr->convMethod="toULINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "LWORD";
                signalPtr->convMethod="toLWORD";
            }else{
                signalPtr->appDataType="LWORD";
                signalPtr->convMethod="toLWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");
            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katları değil,standart olmayan veri transferi");
        }else{
            signalPtr->comDataType = "LWORD";
            if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
                signalPtr->appDataType = "LREAL";
                signalPtr->convMethod="toLREAL";
            }else if(signalPtr->name.contains("N_")){
                signalPtr->appDataType = "ULINT";
                signalPtr->convMethod="toULINT";
            }else if(signalPtr->name.contains("Z_")){
                signalPtr->appDataType = "LWORD";
                signalPtr->convMethod="toLWORD";
            }else{
                signalPtr->appDataType="LWORD";
                signalPtr->convMethod="toLWORD";
                this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");
            }
            this->setWarning(this->messageID,signalPtr->name+" sinyali veri boyutu 8 ve katları değil,standart olmayan veri transferi");
            this->setWarning(this->messageID,signalPtr->name+" sinyali başlangıç biti 8 ve katları değil,düşük perfomans");
        }
    }else if (signalPtr->length == 64){
        signalPtr->comDataType = "LWORD";
        if(signalPtr->name.contains("X_") || signalPtr->name.contains("W_")){
            signalPtr->appDataType = "LREAL";
            signalPtr->convMethod="xtoLREAL";
        }else if(signalPtr->name.contains("N_")){
            signalPtr->appDataType = "ULINT";
            signalPtr->convMethod="xtoULINT";
        }else if(signalPtr->name.contains("Z_")){
            signalPtr->appDataType = "LWORD";
            signalPtr->convMethod="xtoLWORD";
        }else{
            signalPtr->appDataType = "LWORD";
            this->setWarning(this->messageID,signalPtr->name+" sinyali isimlendirmesinde X_ W_ N_ Z_ işareti bulunmuyor");
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
    warningMessages.clear();
}

void dataContainer::signalChecker(signal *signalPtr)
{
    if(signalPtr->length == 8){
        if(signalPtr ->maxValue > 250){
            if(signalPtr ->isJ1939){
                this->setWarning(this->messageID,signalPtr->name+" sinyali J1939 olarak tanımlanmış ancak maksimum değeri ERR ve NA tanım aralığında, maksimum değeri 250 olarak atandı.");
                signalPtr->maxValue = 250;
            }
             this->setWarning(this->messageID,signalPtr->name+" sinyali maksimum değeri atamasu veri tipi kapasitesi baz alınarak yapılmış, mantıksal değişken değer aralığı hesaplanması önerilir.");
        }
    }else if(signalPtr->length == 16){
        if(signalPtr ->maxValue > 64255){
            if(signalPtr ->isJ1939){
                this->setWarning(this->messageID,signalPtr->name+" sinyali J1939 olarak tanımlanmış ancak maksimum değeri ERR ve NA tanım aralığında, maksimum değeri 64255 olarak atandı.");
                signalPtr->maxValue = 64255;
            }
             this->setWarning(this->messageID,signalPtr->name+" sinyali maksimum değeri atamasu veri tipi kapasitesi baz alınarak yapılmış, mantıksal değişken değer aralığı hesaplanması önerilir.");
        }
    }else if(signalPtr->length == 32){
        if(signalPtr ->maxValue > 4211081215){
            if(signalPtr ->isJ1939){
                this->setWarning(this->messageID,signalPtr->name+" sinyali J1939 olarak tanımlanmış ancak maksimum değeri ERR ve NA tanım aralığında, maksimum değeri 4211081215 olarak atandı.");
                signalPtr->maxValue = 4211081215;
            }
             this->setWarning(this->messageID,signalPtr->name+" sinyali maksimum değeri atamasu veri tipi kapasitesi baz alınarak yapılmış, mantıksal değişken değer aralığı hesaplanması önerilir.");
        }
    }

    if(signalPtr->offset >=0 && signalPtr->minValue <0){
        this->setWarning(this->messageID,signalPtr->name+" sinyali offset verilmeden negatif minimum aralık tanımlanmış, minimum değer 0 atandı.");
        signalPtr->minValue = 0;
    }
    if(signalPtr->length > (64 - signalPtr->startBit)){
        this->setWarning(this->messageID,signalPtr->name+" sinyali için veri boyutu tanım aralığından büyük.");
    }

}
