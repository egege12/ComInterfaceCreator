#ifndef ASCHANDLER_H
#define ASCHANDLER_H

#include <QObject>

class ASCHandler : public QObject
{
    Q_OBJECT
public:
    explicit ASCHandler(QObject *parent = nullptr);

signals:

};

#endif // ASCHANDLER_H
