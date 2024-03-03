#ifndef BASCULAHANDLER_H
#define BASCULAHANDLER_H

#include <QThread>
#include <QSerialPort>
#include <QFile>
#include <QTextStream>
#include <QDebug>

class BasculaHandler : public QThread
{
    Q_OBJECT

    void run() override{

        QString com = "COM5";

        ///revisar configuracion
        QFile file("puerto.txt");
        if(file.exists()){
            if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
                QTextStream ts(&file);
                while(!ts.atEnd()){
                    QString linea = ts.readLine();
                    qDebug() << "Linea leida : " + linea;
                    if(linea.contains( "puerto", Qt::CaseInsensitive) ){
                        com = linea.right( linea.length()-linea.indexOf(":")-1 ).trimmed();
                    }
                }
                file.close();
            }
        }

        qDebug() << "Puerto : " + com;
        QSerialPort bascula;
        bascula.setPortName(com);

        forever{
            if( !bascula.open(QIODevice::ReadWrite) ){
                bascula.close();
                emit this->estado_conexion_cambiado(false);
                emit this->lectura_terminada("N/C");
                this->msleep(1000);
                continue;
            }

            forever{
                emit this->estado_conexion_cambiado(true);

                QString lectura = "";
                bascula.write(QByteArray("P"));

                if( !bascula.waitForReadyRead(100) ){
                    lectura = QString(bascula.readAll());
                    if(lectura.length()<3){
                        emit this->estado_conexion_cambiado(false);
                        break;
                    }
                }
                if(lectura==""){
                    lectura = QString(bascula.readAll());
                }

                if(lectura.contains("lb") || lectura.contains("oz")){
                    emit this->lectura_terminada( "W/U" );
                }else if(lectura.contains(".")){
                    for(int i = 0; i < lectura.length(); i++){
                        if( !(lectura.at(i) == "0" ||
                              lectura.at(i) == "1" ||
                              lectura.at(i) == "2" ||
                              lectura.at(i) == "3" ||
                              lectura.at(i) == "4" ||
                              lectura.at(i) == "5" ||
                              lectura.at(i) == "6" ||
                              lectura.at(i) == "7" ||
                              lectura.at(i) == "8" ||
                              lectura.at(i) == "9" ||
                              lectura.at(i) == ".") ){
                              lectura.remove(i,1);
                            i = i-1;
                        }
                    }
                    if(lectura.at(0) != '.'){
                        emit this->lectura_terminada(lectura);
                    }

                }
                this->msleep(50);
            }
        }
    }

signals:
    void estado_conexion_cambiado(bool estado);

    void lectura_terminada(QString lectura);

};

#endif // BASCULAHANDLER_H
