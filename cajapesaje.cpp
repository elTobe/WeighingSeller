#include "cajapesaje.h"
#include "ui_cajapesaje.h"
#include <QtSql>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QtPrintSupport/QPrinterInfo>
#include <QFontDatabase>
#include <QPainter>
#include <QSerialPortInfo>

CajaPesaje::CajaPesaje(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CajaPesaje)
{
    ui->setupUi(this);

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
        ui->comboBox->addItem(port.portName());
    }
    QString com = "COM5";
    QFile file("puerto.txt");
    if(file.exists()){
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
            QTextStream ts(&file);
            while(!ts.atEnd()){
                QString linea = ts.readLine();
                qDebug() << "Linea leida : " + linea;
                if(linea.contains( "puerto", Qt::CaseInsensitive) ){
                    com = linea.right( linea.length()-linea.indexOf(":")-1 ).trimmed();
                    ui->comboBox->setCurrentText(com);
                }
            }
            file.close();
        }
    }

    this->handler = new BasculaHandler();

    connect(handler, &BasculaHandler::estado_conexion_cambiado,
            this, &CajaPesaje::cambiar_estado_bascula);

    connect(handler, &BasculaHandler::lectura_terminada,
            this, &CajaPesaje::cambiar_indicador_peso);

    handler->start();

    ui->label_name->setFocus();
    ui->label_codigo_hidden->hide();

    ui->lista->header()->hideSection(codigo); /// codigo articulo
    ui->lista->header()->setSectionResizeMode(peso, QHeaderView::ResizeToContents);
    ui->lista->header()->setSectionResizeMode(unidad, QHeaderView::ResizeToContents);
    ui->lista->header()->setSectionResizeMode(descripcion, QHeaderView::Stretch);
    ui->lista->header()->setSectionResizeMode(simbolo, QHeaderView::ResizeToContents);
    ui->lista->header()->setSectionResizeMode(importe, QHeaderView::ResizeToContents);

    this->debug = false;
}

bool CajaPesaje::conectar_db(QSqlDatabase* db){
    QString ip = "192.168.0.105";
    QString port = "3306";
    QString base = "sicar";
    QString username = "consultas";
    QString password = "123456";

    QFile file("ip_server.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream in(&file);
        ip = in.readLine();
        port = in.readLine();
        base = in.readLine();
        username = in.readLine();
        password = in.readLine();
        file.close();
    }

    db->setHostName(ip);
    db->setPort(port.toInt());
    db->setDatabaseName(base);
    db->setUserName(username);
    db->setPassword(password);

    if(!db->open()){
        QMessageBox messageBox;
        messageBox.critical(0,"Error", "No se pudo conectar a la base de datos !\nError: " + db->lastError().text() );
        messageBox.setFixedSize(500,200);
        return false;
    }

    return true;
}

bool CajaPesaje::conectar_db_root(QSqlDatabase* db){
    QString ip = "192.168.0.105";
    QString port = "3306";
    QString base = "sicar";
    QString username = "root";
    QString password = "super";

    QFile file("ip_server.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream in(&file);
        ip = in.readLine();
        port = in.readLine();
        base = in.readLine();
        file.close();
    }

    db->setHostName(ip);
    db->setPort(port.toInt());
    db->setDatabaseName(base);
    db->setUserName(username);
    db->setPassword(password);

    if(!db->open()){
        QMessageBox messageBox;
        messageBox.critical(0,"Error", "No se pudo conectar a la base de datos !\nError: " + db->lastError().text() );
        messageBox.setFixedSize(500,200);
        return false;
    }

    return true;
}

CajaPesaje::~CajaPesaje()
{
    delete ui;
}

void CajaPesaje::cambiar_estado_bascula(bool estado){
    if(estado){
        ui->label_estado_bascula->setPixmap(QPixmap(":/images/connected.png"));
    }else{
        ui->label_estado_bascula->setPixmap(QPixmap(":/images/disconnected.png"));
    }
}

void CajaPesaje::cambiar_indicador_peso(QString peso_string){

    if(debug){ui->debug->setText(peso_string);}

    if( ui->label_venta_pieza->text().contains("PIEZA") ){
        ui->label_importe->setText( ui->label_precio->text() );
    }else{
        bool tofloat;
        float peso = peso_string.toFloat(&tofloat);
        if(tofloat){
            if(peso < 10){
                ui->label_peso->setText( peso_string.setNum(peso,'f',3) );
                float precio = ui->label_precio->text().toFloat();
                float redondeado = redondear(precio*peso);
                QString importe;
                importe.setNum(redondeado,'f',2);
                ui->label_importe->setText(importe);
            }else{
                ui->label_peso->setText("O/L");
                ui->label_importe->setText("0.00");
            }
        }else{
            if( peso_string.contains("/") ){
                ui->label_importe->setText("0.00");
                ui->label_peso->setText(peso_string);
            }
        }
    }
}

float CajaPesaje::redondear(float num){
    float imp = int( num*100 ) / 100.0 ;
    float centavos = imp - int(imp) ;
    if(centavos != 0.00 && centavos != 0.50){
        if(centavos < 0.50){
            centavos = 0.50 - centavos;
        }else{
            centavos = 1 - centavos;
        }
        imp = imp + centavos;
    }
    return imp;
}

QString CajaPesaje::encode128(QString text){

    QStringList code128chars;
    code128chars << " " << "!" << "\"" << "#" << "$" << "%" << "&" << "'" << "(" << ")";
    code128chars << "*" << "+" << "," << "-" << "." << "/" << "0" << "1" << "2" << "3";
    code128chars << "4" << "5" << "6" << "7" << "8" << "9" << ":" << ";" << "<" << "=";
    code128chars << ">" << "?" << "@" << "A" << "B" << "C" << "D" << "E" << "F" << "G";
    code128chars << "H" << "I" << "J" << "K" << "L" << "M" << "N" << "O" << "P" << "Q";
    code128chars << "R" << "S" << "T" << "U" << "V" << "W" << "X" << "Y" << "Z" << "[";
    code128chars << "\\" << "]" << "^" << "_" << "`" << "a" << "b" << "c" << "d" << "e";
    code128chars << "f" << "g" << "h" << "i" << "j" << "k" << "l" << "m" << "n" << "o";
    code128chars << "p" << "q" << "r" << "s" << "t" << "u" << "v" << "w" << "x" << "y";
    code128chars << "z" << "{" << "|" << "}" << "~" << "Ã" << "Ä" << "Å" << "Æ" << "Ç";
    code128chars << "È" << "É" << "Ê" << "Ë" << "Ì" << "Í" << "Î" << "";

    QString encoded = "Ì";
    int sumador = 103;

    for(int i = 0; i < text.length(); i++){
        encoded += text.at(i);
        sumador += code128chars.indexOf(text.at(i)) * (i+1);
        qDebug() << "Letra actual : " << text.at(i);
        qDebug() << "Valor : " << code128chars.indexOf(text.at(i));
        qDebug() << "Posicion actual : " << i+1;
        qDebug() << "Sumador : " << sumador;
    }
    int mod = sumador % 103;
    encoded += code128chars.at( mod + 1 );

    encoded += "Î";
    qDebug() << "Modulo : " << mod;
    qDebug() << "Encoded : " << encoded;
    return encoded;
}

void CajaPesaje::keyPressEvent(QKeyEvent *event){
    int tecla = event->key();

    /// --------------------------------- RESET ALERTAS ------------------------------
    ui->label_peso->setStyleSheet("color:#7F6A00");
    ui->label_codigo->setStyleSheet("background : white; color:#555555;");


    /// ------------------------------------ NUMEROS ---------------------------------
    if( (tecla > 47 && tecla < 58) || tecla == Qt::Key_Period ){
        QString texto = ui->label_codigo->text();
        texto.append(char(tecla));
        ui->label_codigo->setText(texto);
    }

    /// ---------------------------------- TECLA BORRAR ------------------------------
    if( tecla == Qt::Key_Backspace ){
        ui->label_codigo->setText("");
    }

    /// ---------------------------------- TECLA DIVISION ------------------------------
    if( tecla == Qt::Key_Slash ){
        if( ui->label_total->text() != "0.00"){

            /////////////////////////////  IMPRESION DE TICKET  ////////////////////////////

            QPrinterInfo impresora_info = QPrinterInfo::defaultPrinter();
            QPrinter impresora = QPrinter(impresora_info,
                                          QPrinter::PrinterMode::ScreenResolution);
            int ancho = impresora_info.defaultPageSize().rectPixels(impresora.resolution()).width();
            qDebug() << "Ancho impresora = " << ancho;
            int borde = 15;
            int fila = 3;
            QPainter documento;

            QFontDatabase::addApplicationFont(":/fonts/LucidaTypewriterRegular.ttf");
            QFont fuente = QFont("Lucida Sans Typewriter",10,QFont::Normal,false);
            fuente.setStretch(QFont::Condensed);
            QFontMetrics fuente_metricas = QFontMetrics(fuente);
            int alto_letra = fuente_metricas.height();
            int vspace = 1;
            QDateTime date = QDateTime::currentDateTime();
            QString formattedTime =  "Fecha :        " + date.toString("dd/MM/yyyy       hh:mm:ss");

            QImage logo = QImage(":/images/small_elegant_logo.png");
            QPoint punto = QPoint( (ancho-logo.width())/2 ,0);

            documento.begin(&impresora);
            documento.setFont(fuente);
            documento.drawImage(punto,logo);
            documento.drawText(borde, (fila*alto_letra)+(fila*vspace)+borde, "El Sauz Alto, Pedro Escobedo, Querétaro."); fila++;
            documento.drawText(borde, (fila*alto_letra)+(fila*vspace)+borde, "                                        "); fila++;
            documento.drawText(borde, (fila*alto_letra)+(fila*vspace)+borde, formattedTime); fila++;
            documento.drawText(borde, (fila*alto_letra)+(fila*vspace)+borde, "========================================"); fila++;

            ///Articulos Aqui
            QStringList codigos;
            QStringList cantidades;
            for(int i = ui->lista->topLevelItemCount(); i > 0 ; i--){
                QTreeWidgetItem* item = ui->lista->takeTopLevelItem(0);
                QString s = item->text(peso).leftJustified(6,' ',false);
                s += item->text(unidad) + " ";
                s += item->text(descripcion).leftJustified(25,' ',true);
                s += item->text(simbolo);
                s += item->text(importe).rightJustified(7,' ',false);
                documento.drawText(borde, (fila*alto_letra)+(fila*vspace)+borde, s); fila++;
                codigos << item->text(codigo);
                cantidades << item->text(peso);
            }

            QString total = ui->label_total->text();
            QString total_text = QString("TOTAL : $" + ui->label_total->text().rightJustified(10,' ',false) + "   ").rightJustified(25,' ',true);
            while( total.endsWith("0") || total.endsWith(".") ){
                if( total.endsWith(".") ){
                    total.remove(total.length()-1,1);
                    break;
                }
                total.remove(total.length()-1,1);
            }
            QString unencoded_code = total + "/V";
            QString code = encode128(unencoded_code);
            ui->label_total->setText("0.00");

            documento.drawText(borde, (fila*alto_letra)+(fila*vspace)+borde, "========================================"); fila++;
            fuente.setWeight(QFont::ExtraBold);
            fuente.setPointSize(15);
            documento.setFont(fuente);
            documento.drawText(borde, (fila*alto_letra)+(fila*vspace)+borde+10*vspace, total_text ); fila++;
            fila++;
            fila++;
            fuente.setWeight(QFont::Normal);
            fuente.setPointSize(10);
            documento.setFont(fuente);

            /////////////////////////////  CODIGO DE BARRAS ////////////////////////////

            QFontDatabase::addApplicationFont(":/fonts/Barcode128.ttf");
            QFont fuente_codebar = QFont("Libre Barcode 128", 48, QFont::Normal,false);
            QFontMetrics metricas_codebar = QFontMetrics(fuente_codebar);
            fuente_codebar.setLetterSpacing(QFont::AbsoluteSpacing, 0);
            documento.setFont(fuente_codebar);
            int alto_codebar = metricas_codebar.height();
            int ancho_codebar = metricas_codebar.horizontalAdvance(code);
            documento.drawText( (ancho-ancho_codebar)/2 , (fila*alto_letra)+(fila*vspace)+borde+10*vspace, code );
            documento.drawText( (ancho-ancho_codebar)/2 , (fila*alto_letra)+(fila*vspace)+borde+10*vspace + alto_codebar/3, code );

            documento.end();

            ///////////////////////////  DESCONTAR INVENTARIO /////////////////////////////
            bool descontar = false;

            QFile file("descontar_inventario.txt");
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
                QTextStream in(&file);
                QString line = in.readLine();
                if( line.contains("true") ){
                    descontar = true;
                }
                file.close();
            }

            if( descontar ){
                QSqlDatabase sicardb = QSqlDatabase::addDatabase("QMYSQL");
                if( !conectar_db_root(&sicardb) ){
                    QMessageBox messageBox;
                    messageBox.critical(0,"Error", "No se pudo conectar a la base de datos !\nError: " + sicardb.lastError().text() + "\n No se modificó el inventario" );
                    messageBox.setFixedSize(500,200);
                    return;
                }

                for(int i = 0; i < codigos.length(); i++){
                    QString cant_i = cantidades.at(i);
                    bool ok_conv;
                    float descontar_f = cant_i.toFloat(&ok_conv);

                    if(ok_conv){
                        QSqlQuery consulta;
                        consulta.exec("UPDATE articulo SET existencia=existencia-"+ QVariant(descontar_f).toString() + " WHERE clave='VERYCAR" + codigos.at(i) + "'");
                        qDebug() << consulta.lastError().text();
                    }
                }
            }
        }
    }


    /// ---------------------------------- TECLA MAS(+) ------------------------------
    if( tecla == Qt::Key_Plus ){

        if(ui->label_importe->text() != "0.00" && ui->label_name->text() != "NO ENCONTRADO"){

            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setText( codigo, ui->label_codigo_hidden->text() );
            if( ui->label_venta_pieza->text().contains("OFERTA") ){
                item->setText( descripcion, ui->label_name->text()  + " * OFERTA *" );
            }else{
                item->setText( descripcion, ui->label_name->text() );
            }

            item->setText( simbolo, "$" );

            if( ui->label_venta_pieza->text().contains("PIEZA") ){
                item->setText( unidad, "pz" );
                if( ui->label_codigo->text() != "." && ui->label_codigo->text() != "0"){
                    if(ui->label_codigo->text() == ""){
                        item->setText( peso, "1.000" );
                        QString t;
                        t.setNum( redondear(ui->label_importe->text().toFloat()) , 'f', 2);
                        item->setText( importe, t );
                    }else{
                        QString t;
                        t.setNum(ui->label_codigo->text().toFloat(), 'f', 2);
                        item->setText( peso, t );
                        t.setNum( redondear( ui->label_importe->text().toFloat() * ui->label_codigo->text().toFloat() ) , 'f', 2);
                        item->setText( importe, t );
                    }
                }else{
                    ui->label_codigo->setStyleSheet("background : white; color:#555555; border : 5px solid red;");
                }
            }else{
                item->setText( peso, ui->label_peso->text().trimmed() );
                item->setText( unidad, "kg" );
                item->setText( importe, ui->label_importe->text() );
            }

            if( ui->lista->topLevelItemCount() != 0 ){

                QTreeWidgetItem* item2 = ui->lista->takeTopLevelItem( ui->lista->topLevelItemCount()-1 );
                if( item2->text(codigo) == item->text(codigo) && item2->text(importe) == item->text(importe)  ){
                    ui->lista->addTopLevelItem(item2);
                    ui->lista->scrollToBottom();
                    return;
                }else{
                    ui->lista->addTopLevelItem(item2);
                    ui->lista->addTopLevelItem(item);
                    ui->lista->scrollToBottom();
                }
            }

            QString t;
            t.setNum(ui->label_total->text().toFloat() + item->text(importe).toFloat(), 'f', 2);
            ui->label_total->setText(t);
            ui->lista->addTopLevelItem(item);
            ui->lista->scrollToBottom();
            ui->label_codigo->setText("");

        }else{

            bool tofloat;
            float peso = ui->label_peso->text().toFloat(&tofloat);
            if(!tofloat){
                ui->label_peso->setStyleSheet("color:#7F6A00; border: 5px solid red;");
            }
        }
    }

    /// ---------------------------------- TECLA MENOS(-) ------------------------------
    if( tecla == Qt::Key_Minus ){
        if(ui->lista->topLevelItemCount()>0){
            QTreeWidgetItem* item = ui->lista->takeTopLevelItem(ui->lista->topLevelItemCount()-1);

            float art_importe = item->text(importe).toFloat();
            float total = ui->label_total->text().toFloat();
            QString t;
            t.setNum(total - art_importe, 'f', 2);
            ui->label_total->setText(t);
        }
    }

    /// ---------------------------------- TECLA ENTER ------------------------------
    if( (tecla == Qt::Key_Return || tecla == Qt::Key_Enter) && ui->label_codigo->text().length()>0 ){

        QSqlDatabase sicardb = QSqlDatabase::addDatabase("QMYSQL");
        if( !conectar_db(&sicardb) ){
            ui->label_codigo->setText("");
            return;
        }

        QString cod = ui->label_codigo->text();
        while( cod.startsWith('0') ){
            cod.remove(0,1);
        }

        QSqlQuery consulta;
        consulta.exec("SELECT descripcion,precio1,precio4,unidadVenta,unidadCompra,impuesto FROM articulo a LEFT JOIN articuloimpuesto ai ON a.art_id=ai.art_id LEFT JOIN impuesto i ON i.imp_id=ai.imp_id WHERE clave='VERYCAR" + cod + "' AND a.status!=-1");
        qDebug() << consulta.lastError().text();
        if(consulta.next()) {

            ui->label_codigo_hidden->setText(ui->label_codigo->text());

            float impuesto = 1;
            if( !consulta.value("impuesto").toString().isEmpty() ){
                impuesto = impuesto + consulta.value("impuesto").toFloat()/100.0;
            } float p = consulta.value("precio1").toFloat();

            if( !consulta.value("precio4").toString().isEmpty() ){
                precio_oferta = consulta.value("precio4").toFloat();
            }else{
                precio_oferta = 0.0;
            } precio_oferta *= impuesto;

            QString precio;
            precio.setNum(p*impuesto,'f',2);
            ui->label_precio->setText(precio);

            ui->label_name->setStyleSheet("color: #3379B7");
            ui->label_name->setText(consulta.value("descripcion").toString());

            if( consulta.value("unidadVenta") == 6 || consulta.value("unidadCompra") == 6 ){
                ui->label_peso->setStyleSheet("text-decoration:line-through; color:#7F6A00;");
                ui->label_venta_pieza->setText("VENTA X PIEZA");
                ui->label_cantidad_codigo->setText("Numero de PIEZAS : ");
                ui->label_cantidad_codigo->setStyleSheet("border: 3px solid green;");
                ui->label_codigo->setStyleSheet("background : white; color:#555555; border: 3px solid green");
                ui->label_venta_pieza->setStyleSheet("color: green;");
                ui->label_kg_simbolo->setText("pz : $");
                ui->label_indicador_cu->setText("C/U");
            }else{
                ui->label_peso->setStyleSheet("text-decoration:none; color:#7F6A00;");
                ui->label_cantidad_codigo->setText("Codigo : ");
                ui->label_cantidad_codigo->setStyleSheet("");
                ui->label_codigo->setStyleSheet("background : white; color:#555555;");
                ui->label_venta_pieza->setStyleSheet("");
                ui->label_venta_pieza->setText("");
                ui->label_kg_simbolo->setText("kg : $");
                ui->label_indicador_cu->setText("");
            }

            precio.setNum(redondear(p/2),'f',2);
            ui->label_medio->setText(precio);
            precio.setNum(redondear(p/4),'f',2);
            ui->label_cuarto->setText(precio);
            precio.setNum(redondear(p/10),'f',2);
            ui->label_cien->setText(precio);

        }else{

            ui->label_name->setStyleSheet("color: red");
            ui->label_name->setText("NO ENCONTRADO");
            ui->label_precio->setText("0.00");

        }
        ui->label_codigo->setText("");
    }

    /// ---------------------------------- TECLA BORRAR ------------------------------
    if( tecla == Qt::Key_Backspace ){
        ui->label_codigo->setText("");
    }

    /// ---------------------------------- OFERTAS ------------------------------
    if( tecla == Qt::Key_Asterisk && precio_oferta != 0.0 ){

        if( ui->label_venta_pieza->text().contains("OFERTA") ){
            ui->label_precio->setStyleSheet("");
            ui->label_kg_simbolo->setStyleSheet("");
            ui->label_venta_pieza->setText(ui->label_venta_pieza->text().remove("** OFERTA **"));
            if( ui->label_venta_pieza->text().contains("PIEZA") ){
                ui->label_venta_pieza->setStyleSheet("color: green;");
            }else{
                ui->label_venta_pieza->setStyleSheet("");
            }
        }else{
            ui->label_precio->setStyleSheet("color: teal");
            ui->label_venta_pieza->setText(ui->label_venta_pieza->text() + "** OFERTA **");
            ui->label_venta_pieza->setStyleSheet("color: teal;");
            ui->label_kg_simbolo->setStyleSheet("color: teal;");
        }

        float temp = ui->label_precio->text().toFloat();

        QString precio;
        precio.setNum(precio_oferta,'f',2);
        ui->label_precio->setText(precio);

        precio_oferta = temp;
    }
}

void CajaPesaje::reimpresion_ticket(){

}

void CajaPesaje::on_pushButton_3_clicked()
{
    exit(0);
}


void CajaPesaje::on_pushButton_4_clicked()
{
    QFile file("puerto.txt");
    if(file.exists()){
        if(file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)){
            QTextStream ts(&file);
            ts << "puerto:" << ui->comboBox->currentText() << "\n";
            file.close();
        }
    }
}


void CajaPesaje::on_pushButton_2_clicked()
{

}

