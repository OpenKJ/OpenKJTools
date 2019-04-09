#ifndef SONGDBAPI_H
#define SONGDBAPI_H

#include <QObject>

class SongDbApi : public QObject
{
    Q_OBJECT
public:
    explicit SongDbApi(QObject *parent = nullptr);

signals:

public slots:
};

#endif // SONGDBAPI_H