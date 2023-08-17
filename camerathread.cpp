#include "camerathread.h"
#include <QDebug>
CameraThread::CameraThread(QObject *parent):QThread(parent)
{
      sign3 = 0;
      camera_init();
      isrunning = false;
}

CameraThread::~CameraThread()
{
     camera_destroy();

}

void CameraThread::run()
{
    isrunning = true;
    char rgb[640*480*3];
        //8.采集数据
        while(isrunning){
            //qDebug() << "获取图形中";
            //出队
            struct v4l2_buffer buf = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            int  res = ioctl(camera_fd,VIDIOC_DQBUF,&buf);
            if(res==-1){
                perror("dqbuf");
                //exit(-1);
            }

            //拷贝数据
            memcpy(current.start,buffer[buf.index].start,buf.bytesused);
            current.length = buf.bytesused;

            //入队
             res = ioctl(camera_fd,VIDIOC_QBUF,&buf);
            if(res==-1){
                perror("qbuf");
                //exit(-1);
            }

            //显示
            //YUYV转RGB
            yuyv2rgb0((unsigned char *)current.start,(unsigned char *)rgb,640,480);

            QImage image = QImage((uchar *)rgb,640,480,QImage::Format_RGB888);
            emit send_image(image);

            //延时
            msleep(10);

        }
}

void CameraThread::stop_thread()
{
    isrunning = false;
}

int CameraThread::yuyv2rgb(int y, int u, int v)
{
    unsigned int pixel24 = 0;
        unsigned char *pixel = (unsigned char *)&pixel24;
        int r, g, b;
        static int  ruv, guv, buv;

        if(sign3)
        {
            sign3 = 0;
            ruv = 1159*(v-128);
            guv = 380*(u-128) + 813*(v-128);
            buv = 2018*(u-128);
        }

        r = (1164*(y-16) + ruv) / 1000;
        g = (1164*(y-16) - guv) / 1000;
        b = (1164*(y-16) + buv) / 1000;

        if(r > 255) r = 255;
        if(g > 255) g = 255;
        if(b > 255) b = 255;
        if(r < 0) r = 0;
        if(g < 0) g = 0;
        if(b < 0) b = 0;

        pixel[0] = r;
        pixel[1] = g;
        pixel[2] = b;

        return pixel24;
}

int CameraThread::yuyv2rgb0(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
    unsigned int in, out;
         int y0, u, y1, v;
         unsigned int pixel24;
         unsigned char *pixel = (unsigned char *)&pixel24;
         unsigned int size = width*height*2;

         for(in = 0, out = 0; in < size; in += 4, out += 6)
         {
              y0 = yuv[in+0];
              u  = yuv[in+1];
              y1 = yuv[in+2];
              v  = yuv[in+3];

              sign3 = 1;
              pixel24 = yuyv2rgb(y0, u, v);
              rgb[out+0] = pixel[0];
              rgb[out+1] = pixel[1];
              rgb[out+2] = pixel[2];

              pixel24 = yuyv2rgb(y1, u, v);
              rgb[out+3] = pixel[0];
              rgb[out+4] = pixel[1];
              rgb[out+5] = pixel[2];

         }
         return 0;
}

void CameraThread::camera_init()
{
    //1.打开
         camera_fd = open("/dev/video1",O_RDWR);
        if(camera_fd==-1){
            perror("open");
            exit(-1);
        }

        //2.获取功能参数
        struct v4l2_capability cap = {};
        int res = ioctl(camera_fd,VIDIOC_QUERYCAP,&cap);
        if(res==-1){
            perror("cap");
            exit(-1);
        }
        if(cap.capabilities&V4L2_CAP_VIDEO_CAPTURE){
            printf("is a camera capture!\n");
        }

        //3.获取支持的采集格式
        struct v4l2_fmtdesc fmt = {};
        fmt.index = 0;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while((res=ioctl(camera_fd,VIDIOC_ENUM_FMT,&fmt))==0){
            printf("pixelformat = %c%c%c%c,description = %s\n",fmt.pixelformat&0xff,
                    (fmt.pixelformat>>8)&0xff,(fmt.pixelformat>>16)&0xff,(fmt.pixelformat>>24)&0xff,
                    fmt.description);
            fmt.index++;
        }

        //4.设置通道
        int index = 0;
        res = ioctl(camera_fd,VIDIOC_S_INPUT,&index);
        if(res==-1){
            perror("sinput");
            exit(-1);
        }

        //5.设置采集格式
        struct v4l2_format format = {};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width = 640;//像素高度
        format.fmt.pix.height = 480;//像素宽度
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;//YUYV格式
        format.fmt.pix.field = V4L2_FIELD_NONE;
        res = ioctl(camera_fd,VIDIOC_S_FMT,&format);
        if(res==-1){
            perror("sfmt");
            exit(-1);
        }

        //6.申请缓存空间
        struct v4l2_requestbuffers req = {};
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        res = ioctl(camera_fd,VIDIOC_REQBUFS,&req);
        if(res==-1){
            perror("reqbufs");
            exit(-1);
        }

        //映射，入队的操作
        size_t i = 0,max_len = 0;
        for(i=0;i<4;i++){
            //分配空间
            struct v4l2_buffer buf = {};
            buf.index = i;
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            res = ioctl(camera_fd,VIDIOC_QUERYBUF,&buf);
            if(res==-1){
                perror("querybuf");
                exit(-1);
            }

            if(buf.length>max_len)
                max_len = buf.length;

            //映射
            buffer[i].length = buf.length;
            buffer[i].start = (char*)mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,camera_fd,buf.m.offset);
            if(buffer[i].start==MAP_FAILED){
                perror("mmap");
                exit(-1);
            }

            //入队
            res = ioctl(camera_fd,VIDIOC_QBUF,&buf);
            if(res==-1){
                perror("qbuf");
                exit(-1);
            }
        }
        //为current分配空间
        current.start = (char *)malloc(max_len);
        if(current.start==NULL){
            perror("malloc");
            exit(-1);
        }

        //7.启动摄像头采集
        enum v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        res = ioctl(camera_fd, VIDIOC_STREAMON, &buf_type);
        if(res==-1){
            perror("streamon");
            exit(-1);
        }
}

void CameraThread::camera_destroy()
{
    //9.关闭摄像头
       enum v4l2_buf_type buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        int res = ioctl(camera_fd, VIDIOC_STREAMOFF, &buf_type);
        if(res==-1){
            perror("streamoff");
            exit(-1);
        }

        //解除映射
        for(int i=0;i<4;i++){
            munmap(buffer[i].start,buffer[i].length);
        }
        free(current.start);
        close(camera_fd);
}
