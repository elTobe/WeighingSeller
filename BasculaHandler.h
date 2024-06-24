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

        /// revisar configuracion
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

        /// Escribir a la bascula y leer. Limpiar la cadena para que sea convertible a Float.
        /// Si se envía texto que contenga al menos un "/" este se mostrará al usuario como codigo de error.
        forever{
            if( bascula.open(QIODevice::ReadWrite) ){
                emit this->estado_conexion_cambiado(true);
                forever{
                    bascula.write(QByteArray("P"));
                    if( bascula.waitForBytesWritten(100) ){

                        QByteArray response = bascula.readAll();
                        while (bascula.waitForReadyRead(10)){
                            response += bascula.readAll();
                        }

                        QString lectura = QString(response);
                        qDebug() << lectura;
                        if( lectura.contains("E") ){
                            emit this->lectura_terminada("/" + lectura.left(lectura.indexOf('\r')));
                        }else if(lectura.contains("lb") || lectura.contains("oz")){
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
                    }else{
                        bascula.close();
                        emit this->lectura_terminada("N/C");
                        emit this->estado_conexion_cambiado(false);
                        break;
                    }
                    this->msleep(100);
                }
            }else{
                qDebug() << "No se pudo conectar a la bascula, reintentando en 1 seg";
                this->msleep(1000);
            }
        }
    }

signals:
    void estado_conexion_cambiado(bool estado);

    void lectura_terminada(QString lectura);

    void emit_debug(QString lectura);

};

#endif // BASCULAHANDLER_H
