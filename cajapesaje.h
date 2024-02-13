#ifndef CAJAPESAJE_H
#define CAJAPESAJE_H

#include "BasculaHandler.h"
#include <QMainWindow>
#include <QKeyEvent>
#include <QtSql/QSqlDatabase>
#include <QTreeWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class CajaPesaje; }
QT_END_NAMESPACE

class CajaPesaje : public QMainWindow
{
    Q_OBJECT

public:
    CajaPesaje(QWidget *parent = nullptr);
    ~CajaPesaje();

    bool debug = true;

    float precio_oferta = 0.0;

public slots:
    void cambiar_estado_bascula(bool estado);

    void cambiar_indicador_peso(QString peso);

    void reimpresion_ticket();

private slots:
    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::CajaPesaje *ui;

    void keyPressEvent(QKeyEvent *event);

    bool conectar_db(QSqlDatabase* db);

    bool conectar_db_root(QSqlDatabase* db);

    BasculaHandler* handler;

    float redondear(float num);

    QString encode128(QString text);

    bool insertar(QTreeWidgetItem* item);

    enum Cabeceras:int {codigo=0, peso, unidad, descripcion, simbolo, importe};
};
#endif // CAJAPESAJE_H
