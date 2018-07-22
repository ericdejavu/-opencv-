#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <sys/time.h>
#include <unistd.h>


class Mutex {
private:
    pthread_mutex_t m_mutex;

public:
    Mutex() {pthread_mutex_init(&m_mutex, NULL);}
    void lock() {pthread_mutex_lock(&m_mutex);}
    void unlock() {pthread_mutex_unlock(&m_mutex);}

    class ScopedLock {
        private:
            Mutex &_mutex;

    public:
        ScopedLock(Mutex &mutex) : _mutex(mutex) {_mutex.lock();}
        ~ScopedLock() {_mutex.unlock();}
    };
};

class ImgManager : public DataSet {
private:
    bool isSelect,isShowInfo,isCopyOperation;
    //输入源按照具体项目而定
    cv::VideoCapture ∩
    int sq;
    int pause_time;
    bool write_flag;
    bool getData();
    static void* getPthreadImg(void*);
    void run_thread();

public:
    enum {Exec,Roi,Dest,Next};
    Mutex imgLock;
    bool isSignalLoss;
    bool isCalcFeature;
    int order;
    std::list<VisImg> visImg;
    int vMin,vMax,sMin;

    ImgManager(cv::VideoCapture &c) : isSignalLoss(false),vMin(30),vMax(200),sMin(5),
    cap(c),order(-1),write_flag(false),pause_time(1),sq(0),
    isSelect(false),isShowInfo(false),isCopyOperation(false) {
        run_thread();
    }
    ~ImgManager() {isSignalLoss = true;}

    bool calc_feature();
    DataSet copy();
    cv::Mat clone();
    int add(cv::Mat &src,std::string name);
    bool update(int index,cv::Mat& src);
    bool remove(int index,int pauseTime=1,int orderType=-1);
    bool show();
};
