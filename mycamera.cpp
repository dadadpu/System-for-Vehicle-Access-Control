#include "mycamera.h"
#include "ui_mycamera.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <easypr.h>
#include <easypr/util/switch.hpp>
#include <QDebug>
#include <QString>

using namespace cv;
using namespace easypr;

mycamera::mycamera(QWidget *parent) :
    QMainWindow(parent), // 在初始化列表中调用 QMainWindow 的构造函数
    ui(new Ui::mycamera)
{
    ui->setupUi(this);
    count = 0;
        recognition_success = false;
        car_out_flag = false;
        cost_money = 0;

        init_page();//主监控画面初始化
        database_init();//数据库初始化


        //创建摄像头线程
        camera = new CameraThread(this);
        camera->start();

        //创建识别线程
        recognition = new licensethread(this);
        connect(this,&mycamera::send_recognition_image,recognition,&licensethread::receive_img);
        connect(recognition,&licensethread::send_recognize_result,this,&mycamera::recv_result_from_recognition);//接收识别结果
        connect(camera,&CameraThread::send_image,this,&mycamera::send_image_to_recognition);//图片发送给识别线程

        //绑定识别成功信号和槽函数
        connect(this,&mycamera::recognition_success_signal,this,&mycamera::recognition_success_event);

        //创建一个进程对象
        pro = new QProcess(this);

        //刷卡线程
       //rfid = new rfid_thread(this);
        //connect(rfid,&rfid_thread::get_id_done,this,&Widget::get_cardid_success);
        //connect(this,&Widget::send_pause_rfid,rfid,&rfid_thread::readcard_pause);

        //查询窗口
        //record = new Records();
        update_recordUi_flag = false;


}

mycamera::~mycamera()
{
    delete ui;
    delete thread;
}

//画面初始化
void mycamera::init_page()
{

        //设置背景色
        this->setStyleSheet("background-color: lightblue");

        //摄像头预览
        camera_preview = new QLabel(this);
        camera_preview->setStyleSheet("background-color:#145b7d");
        camera_preview->setGeometry(10,10,530,400);

        //当前时间显示
        currenttime = new QLabel(this);//显示当前时间
        currenttime->setGeometry(10,420,530,50);
        currenttime->setStyleSheet("QLabel { color: white}");
        currenttime->setFont(QFont("Microsoft YaHei",20,20));
        currenttime->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        //更新时间
        datetime_timer = new QTimer();
        datetime_timer->start(1000);
        connect(datetime_timer,&QTimer::timeout,[=](){
            current_time = QDateTime::currentDateTime();
            currenttime->setText(current_time.toString("yyyy-MM-dd hh:mm:ss ddd"));
            if(!update_recordUi_flag){
//                record->in_timeedit->setDateTime(current_time);
//                record->out_timeedit->setDateTime(current_time);
                update_recordUi_flag = true;
            }
        });

        //车牌显示
        license_view = new QLabel(this);//显示识别的车牌
        license_view->setStyleSheet("QLabel { color: white;background-color: #102b6a;}");
        license_view->setGeometry(550,10,240,80);
        license_view->setFont(QFont("Microsoft YaHei",28,20));
        license_view->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        license_view->setText("京A88888");

        //显示车辆进场 出场
        car_inout = new QLabel(this);
        car_inout->setStyleSheet("QLabel { color: white;background-color: green;}");
        car_inout->setGeometry(730,220,60,50);
        car_inout->setFont(QFont("Microsoft YaHei",15,20));
        car_inout->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        car_inout->setText("");

        //人工确认按钮，调试用
        result_confirm = new QPushButton(this);
        result_confirm->setStyleSheet("QPushButton { color: white;background: transparent;border-image:transparent}");
        result_confirm->setGeometry(550,10,240,80);
        result_confirm->setFocusPolicy(Qt::NoFocus);
        connect(result_confirm,&QPushButton::clicked,[&](){
            if(recognition_success){
                emit recognition_success_signal();
            }

        });

        //停车时长
        parktime = new QLabel(this);
        parktime->setGeometry(550,100,240,50);
        parktime->setFont(QFont("Microsoft YaHei",15,20));
        parktime->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        parktime->setText("停车时长:0分钟");

        //收费金额
        charge_money = new QLabel(this);
        charge_money->setGeometry(550,160,240,50);
        charge_money->setFont(QFont("Microsoft YaHei",15,20));
        charge_money->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        charge_money->setText("应收费用:0.00元");

        //充值卡余额
        balance_money = new QLabel(this);
        balance_money->setGeometry(550,220,170,50);
        balance_money->setFont(QFont("Microsoft YaHei",15,20));
        balance_money->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        balance_money->setText("卡余额:0.00元");

        //实收金额
        r_money_front = new QLabel(this);
        r_money = new QLabel(this);
        r_money_end = new QLabel(this);
        r_money_front->setGeometry(550,290,60,50);
        r_money_front->setFont(QFont("Microsoft YaHei",15,20));
        r_money_front->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        r_money_front->setText("实收:");
        r_money->setGeometry(615,280,130,70);
        r_money->setStyleSheet("QLabel { color: red;background-color: #bed742;}");
        r_money->setFont(QFont("Microsoft YaHei",22,20));
        r_money->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        r_money->setText("0.00");
        r_money_end->setGeometry(750,290,30,50);
        r_money_end->setFont(QFont("Microsoft YaHei",15,20));
        r_money_end->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        r_money_end->setText("元");

        //现金收费按钮
        cash_charge = new QPushButton(this);
        cash_charge->setFocusPolicy(Qt::NoFocus);
        cash_charge->setStyleSheet("QPushButton{background-color:green;color: white;border-radius: 10px;border: 2px groove gray;border-style: outset;}"

                                    "QPushButton:hover{background-color:white; color: black;}"

                                    "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }"

                                               );
        cash_charge->setGeometry(550,360,100,50);
        cash_charge->setText("现金收费");
        connect(cash_charge,&QPushButton::clicked,this,&mycamera::cash_charge_clicked);

        //卡收费
        re_recognition = new QPushButton(this);
        re_recognition->setFocusPolicy(Qt::NoFocus);
        re_recognition->setStyleSheet("QPushButton{background-color:green;color: white;border-radius: 10px;border: 2px groove gray;border-style: outset;}"

                                    "QPushButton:hover{background-color:white; color: black;}"

                                    "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }"

                                               );
        re_recognition->setGeometry(670,360,100,50);
        re_recognition->setText("重新识别");
        connect(re_recognition,&QPushButton::clicked,this,&mycamera::re_recognition_clicked);

        //记录查询
        records = new QPushButton(this);
        records->setFocusPolicy(Qt::NoFocus);
        records->setStyleSheet("QPushButton{background-color:green;color: white;border-radius: 10px;border: 2px groove gray;border-style: outset;}"

                                    "QPushButton:hover{background-color:white; color: black;}"

                                    "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }"

                                               );
        records->setGeometry(550,420,100,50);
        records->setText("记录查询");
        //connect(records,&QPushButton::clicked,this,&Widget::records_clicked);

        //放行
        green_light = new QPushButton(this);
        green_light->setFocusPolicy(Qt::NoFocus);
        green_light->setStyleSheet("QPushButton{background-color:green;color: white;border-radius: 10px;border: 2px groove gray;border-style: outset;}"

                                    "QPushButton:hover{background-color:white; color: black;}"

                                    "QPushButton:pressed{background-color:rgb(85, 170, 255); border-style: inset; }"

                                               );
        green_light->setGeometry(670,420,100,50);
        green_light->setText("放行");
        connect(green_light,&QPushButton::clicked,this,&mycamera::green_light_clicked);


}

void mycamera::database_init()
{
    //添加数据库驱动
       db = QSqlDatabase::addDatabase("QSQLITE");
       //指定数据库路径
       db.setDatabaseName("./Parking_info.db");
       //打开数据库
       db.open();

       //初始化模型
       carinfo_model = new QSqlTableModel(this);
       //cardinfo_model = new QSqlTableModel(this);

       //创建车辆信息表
       //QString sql = QString("create table if not exists carinfo(license text not NULL,color text,in_time text,out_time text,parkingtime text,status text,inpic_path text,outpic_path text,cost float,in_time_int bigint,out_time_int bigint)");
       QString sql = QString("create table if not exists carinfo(license text not NULL,color text,in_time text,out_time text,parkingtime text,status text,inpic_path text,outpic_path text,cost float)");
       QSqlQuery query;
       if(!query.exec(sql)){
           QMessageBox::warning(this,"建表","建表失败!",QMessageBox::Ok);
       }

//       //创建停车卡信息表
//       sql = QString("create table if not exists rfidCardinfo(CardNo text not NULL,money float)");
//       if(!query.exec(sql)){
//           QMessageBox::warning(this,"停车卡","建表失败!",QMessageBox::Ok);
//       }

       //绑定表格
       carinfo_model->setTable("carinfo");
       //cardinfo_model->setTable("rfidCardinfo");

}

//现金收费按钮
void mycamera::cash_charge_clicked()
{
    if(car_out_flag){
        //emit send_pause_rfid();//选择现金收费，暂停rfid读卡
        r_money->setNum(cost_money);
        //提交数据
        if(!carinfo_model->submitAll()){
            QMessageBox::warning(this,"出场计费","写入数据库失败!",QMessageBox::Ok);
        }

        //播放语音
        car_inout->setText("出场");
        pro->kill();
        pro->start("aplay ./sound/out_2.WAV");
        //重新开始识别
        recognition_success = false;
        car_out_flag = false;
    }

}

void mycamera::re_recognition_clicked()
{
    recognition_success = false;
    license_view->clear();
}

//放行按钮
void mycamera::green_light_clicked()
{
    if(car_out_flag){
        //emit send_pause_rfid();//选择现金收费，暂停rfid读卡
        r_money->setNum(0);
        //提交数据
        if(!carinfo_model->submitAll()){
            QMessageBox::warning(this,"出场计费","写入数据库失败!",QMessageBox::Ok);
        }

        //播放语音
        car_inout->setText("出场");
        pro->kill();
        pro->start("aplay ./sound/out_2.WAV");
        //重新开始识别
        recognition_success = false;
        car_out_flag = false;
    }
}

void mycamera::records_clicked()
{

}

void mycamera::send_image_to_recognition(QImage image)
{
    QString info = current_time.toString("yyyy-MM-dd hh:mm:ss ddd");
       QImage tmp = image.copy();//弄一个副本加水印，原图给识别线程
       QPainter pp(&tmp);
       QPen pen = QPen(Qt::red,15);
       QBrush brush = QBrush(Qt::red);
       pp.setPen(pen);
       pp.setBrush(brush);
       pp.setFont(QFont("Microsoft YaHei",12,20));
       pp.drawText(QPointF(380,470),info);

       QPixmap pix = QPixmap::fromImage(tmp).scaled(camera_preview->size());
       camera_preview->setPixmap(pix);

       count++;

       if((count == 20) && !recognition_success){
           recognition_tmp = image.copy();
           emit send_recognition_image(image);
       }

       if(count == 25){
           count = 0;
           if(!recognition_success){//如果识别成功，则停止识别
               qDebug() << "count" << count;
               license_view->clear();
               recognition->start();
           }
       }
}

void mycamera::recv_result_from_recognition(QString result)
{
    qDebug()<<"recv_result_from_recognition"<<result;
    if(result != "fail" && result.contains(':')){
            recognition_result = result;
            result = result.mid(result.indexOf(':')+1);
            license_view->setText(result);

            //车牌颜色
            QString color = recognition_result.left(recognition_result.indexOf(':')+1);
            if(color == "蓝牌:"){
                license_view->setStyleSheet("QLabel { color: white;background-color: #102b6a;}");
            }
            if(color == "黄牌:"){
                license_view->setStyleSheet("QLabel { color: white;background-color: #ffe600;}");
            }
            recognition_success = true; //识别成功标识，停止识别

        }
        else{
            license_view->setText("fail");
            recognition_result.clear();
        }
}

//识别成功后的操作
void mycamera::recognition_success_event()
{

    car_out_flag = false;

        //车牌颜色
        QString color = recognition_result.left(recognition_result.indexOf(':')+1);

        //车牌
        QString license = recognition_result.mid(recognition_result.indexOf(':')+1);//苏A2396V

        //根据车牌查询当前车辆状态
        int status = 0;//1 入场 2 出场
        data_row = -1;
        //设置过滤器
        QString filter_text = QString("license='%1'").arg(license);
        carinfo_model->setFilter(filter_text);
        carinfo_model->select();
        int num = carinfo_model->rowCount();
        if(num == 0){//查询不到相关数据，表示车辆入场
            status = 1;
        }
        if(num > 0){//查询到相关记录，比较status
            int i ;
            for(i = 0;i < num;i++){
                if(carinfo_model->record(i).value("status").toString() == "入场"){
                    data_row = i;
                    status = 2;
                    break;
                }
            }
            if(i == num){
                status = 1;
            }
        }


        //日期时间 格式2021-8-4 10:00
        QString datatime = current_time.toString("yyyy-MM-dd hh:mm ddd ");

        //拼接车牌和日期
        QString inout;
        QString inout_zh;
        if(status == 1){
            inout = "in";
            inout_zh = " 入场";
        }else if(status == 2){
            inout = "out";
            inout_zh = " 出场";
        }
        QString pic_name = "camera/" + recognition_result.mid(recognition_result.indexOf(':')+2) +
                                current_time.toString("yyyyMMddhhmm") +
                                inout + ".png";

        //保存出入场照片
        QString info = datatime + license + inout_zh;
        QPainter pp(&recognition_tmp);
        QPen pen = QPen(Qt::red,15);
        QBrush brush = QBrush(Qt::red);
        pp.setPen(pen);
        pp.setBrush(brush);
        pp.setFont(QFont("Microsoft YaHei",12,20));

        pp.drawText(QPointF(250,470),info);
        recognition_tmp.save(pic_name);

        //写入数据库处理
        if(status == 1){//车辆入场
            QSqlRecord record = carinfo_model->record();
            record.setValue("license",license);
            record.setValue("color",color);
            record.setValue("in_time",current_time.toString("yyyy-MM-dd hh:mm:ss ddd"));
            record.setValue("status","入场");
            record.setValue("inpic_path",pic_name);
            //record.setValue("in_time_int",current_time.toMSecsSinceEpoch());

            //提交数据
            carinfo_model->insertRecord(-1,record);
            carinfo_model->submitAll();
            car_inout->setText("入场");
            //播放语音
            pro->kill();
            pro->start("aplay ./sound/in_1.WAV");

            //重新开始识别
            recognition_success = false;

        }
        float cost;//停车费用
        if(status == 2){//车辆出场
            //停车时间计算
            QDateTime in = QDateTime::fromString(carinfo_model->record(data_row).value("in_time").toString(),"yyyy-MM-dd hh:mm:ss ddd");
            QDateTime out = current_time;
            qint64 parkingtime_s = (out.toMSecsSinceEpoch()-in.toMSecsSinceEpoch())/1000;
            int day,hour,minute,second;
            day=hour=minute=second=0;
            day = parkingtime_s/86400;
            hour = parkingtime_s%86400/3600;
            minute = parkingtime_s%3600/60;
            second = parkingtime_s%60;
            QString parkingtime = QString("%1天%2时%3分").arg(day).arg(hour).arg(minute);
            //停车费用计算 一天20元 一个小时2元，不足一个小时按一个小时,30分钟内免费
            //cost = day*20 + hour*2 + (hour>0?(minute>0?2:0):(minute>30?2:0));//30分钟内免费
            cost = day*20 + hour*2 + 2;//不免费
            QSqlRecord record = carinfo_model->record(data_row);
            record.setValue("out_time",current_time.toString("yyyy-MM-dd hh:mm:ss ddd"));
            record.setValue("parkingtime",parkingtime);
            record.setValue("status","出场");
            record.setValue("outpic_path",pic_name);
            record.setValue("cost",cost);
            //record.setValue("out_time_int",current_time.toMSecsSinceEpoch());
            carinfo_model->setRecord(data_row,record);
            parktime->setText(QString("停车时长:%1").arg(parkingtime));
            charge_money->setText(QString("应收费用:%1元").arg(cost));
            cost_money = cost;

            car_inout->setText("出场");
            //播放语音
            if(cost > 0)
            {
                pro->kill();
                pro->start("aplay ./sound/out_1.WAV");
                car_out_flag = true;
                //rfid->start();
            }
            if(cost == 0){
                r_money->setNum(cost_money);
                //提交数据
                if(!carinfo_model->submitAll()){
                    QMessageBox::warning(this,"出场计费","写入数据库失败!",QMessageBox::Ok);
                }

                //播放语音
                car_inout->setText("出场");
                pro->kill();
                pro->start("aplay ./sound/out_2.WAV");
                //重新开始识别
                recognition_success = false;
                car_out_flag = false;

            }

        }

        //提示信息更新
        if(status == 1){//车辆入场
            parktime->setText("停车时长:0分钟");
            charge_money->setText("应收费用:0.00元");
            balance_money->setText("卡余额:0.00元");
            r_money->setText("0.00");
        }
}

void mycamera::get_cardid_success(unsigned int cardid)
{

}
