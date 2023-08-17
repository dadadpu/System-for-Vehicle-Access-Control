#include "licensethread.h"
#include <QDebug>

licensethread::licensethread(QObject *parent):QThread(parent)
{

}

licensethread::~licensethread()
{

}

void licensethread::run()
{
    while(1){
        //车牌识别
        CPlateRecognize pr;
        //不显示识别过程
        pr.setResultShow(false);
        //通过文字 边沿 颜色识别
        pr.setDetectType(PR_DETECT_CMSER|PR_DETECT_COLOR | PR_DETECT_SOBEL);
        //打开生活模式
        pr.setLifemode(true);

        vector<CPlate> plateVec;
        Mat src = Mat(image.height(),image.width(),CV_8UC3,(void *)image.bits(),image.bytesPerLine());
        //Mat src = imread("./1.jpg");
        //EasyPR只能处理三通道
        //cvtColor(src,src,CV_RGBA2RGB);

        //识别
        //识别
        int result = pr.plateRecognize(src, plateVec);
        if(result==0&&!plateVec.empty()){//识别到了车牌
            CPlate plate = plateVec.at(0);//识别到的第一张
            Mat plateMat = plate.getPlateMat();//车牌部分的图像
            RotatedRect rrect = plate.getPlatePos();
            string license = plate.getPlateStr();//车牌字符串
            qDebug()<<QString::fromLocal8Bit(license.c_str());
            QString tmp = QString::fromUtf8(license.c_str());
            emit send_recognize_result(tmp);

        }
        else{
            qDebug()<<"没有识别到车牌";
            emit send_recognize_result("fail");
        }

        break;
    }

}

void licensethread::receive_img(QImage image)
{
    this->image = image;

}
