#include "imgManager.h"

//different memory manager
//i defined in main function those data no need to delete i use &dataSet
void DataSet::operator=(DataSet &dataSet) {
    color = dataSet.color.clone();
    im = dataSet.im.clone();
    if(kp.size() > 1) kp.clear();
    for(int i=0;i<dataSet.kp.size();i++) kp.push_back(dataSet.kp[i]);
}

//function return dataSet will generator a lot of useless data
//those data need to delete so..... just use dataSet
void DataSet::set(DataSet dataSet) {
    color = dataSet.color.clone();
    im = dataSet.im.clone();
    if(kp.size() > 1) kp.clear();
    for(int i=0;i<dataSet.kp.size();i++) kp.push_back(dataSet.kp[i]);
}

DataSet ImgManager::copy() {
    isCopyOperation = true;
    Mutex::ScopedLock lock(imgLock);
    DataSet dataSet;
    dataSet.color = color.clone();
    if(isCalcFeature) {
        dataSet.im = im.clone();
        for(int i=0;i<kp.size();i++) dataSet.kp.push_back(kp[i]);
    }
    isCopyOperation = false;
    return dataSet;
}

cv::Mat ImgManager::clone() {
    isCopyOperation = true;
    Mutex::ScopedLock lock(imgLock);
    cv::Mat data = color.clone();
    isCopyOperation = false;
    return data;
}

bool ImgManager::calc_feature() {
    cv::OrbFeatureDetector ofd;
    cv::OrbDescriptorExtractor ode;
    cv::Mat src = color.clone();
    cv::Mat gray = src.clone();
    cv::blur(src,src,cv::Size(3,3));
    cv::cvtColor(src,gray,CV_BGR2GRAY);
    ofd.detect(gray,kp);
    ode.compute(gray,kp,im);
    if(im.empty() || src.empty()) return false;
    return true;
}

// ------------------------show Unit -------------------
int ImgManager::add(cv::Mat &src,std::string name) {
    VisImg tmp;
    tmp.name = name;
    std::cout << src.size() << std::endl;
    tmp.src = src.clone();
    tmp.index = sq;
    visImg.push_back(tmp);
    if(name == "tracker") {
        cv::namedWindow("tracker");
        cv::createTrackbar("vMin", "tracker", &vMin, 255, 0);
        cv::createTrackbar("vMax", "tracker", &vMax, 255, 0);
        cv::createTrackbar("sMin", "tracker", &sMin, 255, 0);
        usleep(1e5);
    }
    std::cout << tmp.src.size() << std::endl;
    return (sq++);
}

bool ImgManager::remove(int index,int pauseTime,int orderType) {
    if(visImg.size() < 1) return false;
    for(std::list<VisImg>::iterator it = visImg.begin();it!=visImg.end();) {
        if(it->index == index) {
            cv::destroyWindow(it->name);
            it = visImg.erase(it);
            break;
        }
        else it++;
    }
    pause_time = pauseTime;
    if(order == Next) {
        order = -1;
        return true;
    }
    else if(orderType == order) {
        order = -1;
        return true;
    }
    return false;
}

bool ImgManager::update(int index,cv::Mat& src) {
    if(!write_flag) write_flag = true;
    for(std::list<VisImg>::iterator it = visImg.begin();it!=visImg.end();it++) if(index == it->index) {
        it->src = src.clone();
    }
    write_flag = false;
    return (pause_time == 0);
}

bool ImgManager::getData() {
    if(isCopyOperation) return true;
    Mutex::ScopedLock lock(imgLock);
    cap >> color;
    if(isCalcFeature) calc_feature();
    cv::imshow("src",color);
    for(std::list<VisImg>::iterator it = visImg.begin();it!=visImg.end();it++) {
        if(!write_flag) {
            if(it->src.empty()) continue;
            cv::imshow(it->name,it->src);
        }
    }
    char key = char(cv::waitKey(pause_time));
    switch(key) {
        case 27:return false;break;
        case 'n': order = Next;break;
        case 'r': order = Roi;break;
        case 'd': order = Dest;break;
        case 'c': order = Exec;break;
    }
    if(pause_time == 0) pause_time = 1;
    return true;
}

void* ImgManager::getPthreadImg( void* __this ) {
    ImgManager* _this = (ImgManager*) __this;
    while(true) {
        if(!_this->getData()) {
            _this->isSignalLoss = true;
            cv::destroyWindow("src");
            for(std::list<VisImg>::iterator it = _this->visImg.begin();it!=_this->visImg.end();it++) cv::destroyWindow(it->name);
            _this->visImg.clear();
            pthread_exit(NULL);
        }
    }
}

void ImgManager::run_thread() {
    pthread_t id;
    pthread_create(&id,NULL,getPthreadImg,(void*)this);
}
