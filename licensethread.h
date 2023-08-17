#ifndef LICENSETHREAD_H
#define LICENSETHREAD_H

#include <QThread>
#include <QImage>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <easypr.h>
#include <easypr/util/switch.hpp>

using namespace easypr;

using namespace cv;

class licensethread : public QThread
{
    Q_OBJECT
public:
    licensethread(QObject *parent=nullptr);
    ~licensethread();
    void run();//重写线程函数
    QImage image;//接收到需要识别的图像

public slots:
    void receive_img(QImage image);
signals:
    void send_recognize_result(QString result);
};

#endif // LICENSETHREAD_H
