#ifndef MYCAMERA_H
#define MYCAMERA_H

#include <QMainWindow>
#include "camerathread.h"
#include <QDebug>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QProcess>
#include "licensethread.h"
namespace Ui {
class mycamera;
}

class mycamera : public QMainWindow
{
    Q_OBJECT

public:
    explicit mycamera(QWidget *parent = nullptr);
    ~mycamera();
    void init_page();
    void database_init();


private:
    Ui::mycamera *ui;
    CameraThread *thread;
    //quint16 count;

    QLabel * camera_preview; //摄像头画面预览
     QLabel * currenttime;//显示实时时间
     QLabel * license_view;//显示识别出来的车牌
     QLabel * parktime;//显示停车时长
     QLabel * charge_money;//收费金额
     QLabel * balance_money;//充值卡余额
     QLabel * r_money;//实收金额
     QLabel * r_money_front;
     QLabel * r_money_end;
     QLabel * car_inout;
     QPushButton * cash_charge;//现金收费按钮
     QPushButton * re_recognition;//重新识别
     QPushButton * green_light;//放行
     QPushButton * records;//记录查询
     QPushButton * result_confirm;//识别结果确认====调试用

     QTimer * datetime_timer;
     QDateTime current_time;//当前日期时间

     QSqlDatabase db;//数据库驱动
     QSqlTableModel * carinfo_model;//车辆信息数据库table模型
     QSqlTableModel * cardinfo_model;//停车卡数据库table模型

     CameraThread * camera;//摄像头线程对象
     licensethread *  recognition;//识别车牌线程
//     rfid_thread * rfid;//刷卡线程
     int count;//拍照计数
     volatile bool recognition_success;//识别成功标志
     QImage recognition_tmp;//识别的图片缓存，识别成功后保存该图片
     QString recognition_result;//识别结果
     bool car_out_flag;//车辆出场并正确识别标志

     float cost_money;
     int data_row;//符合车牌的数据

     QProcess * pro;
//     //窗口
//     Records * record;
     bool update_recordUi_flag;


private slots:
    void cash_charge_clicked();
    void re_recognition_clicked();
    void green_light_clicked();
    void records_clicked();

    void send_image_to_recognition(QImage image);//发送图片给识别线程
    void recv_result_from_recognition(QString result);//从识别线程接收识别结果
    void recognition_success_event();//识别成功后处理事件
    void get_cardid_success(unsigned int cardid);//刷卡成功

signals:
    void send_recognition_image(QImage image);
    void recognition_success_signal();
    void send_pause_rfid();
};

#endif // MYTHREADCAMERA_H
