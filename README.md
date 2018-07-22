*created by [Dejavu](http://www.dejavu.link:8000/Blog)*

----
你们有过这样的经历吗？当在多线程中使用opencv的imshow和waitkey方法，不知道该如何下手，因为waitkey是管理按键事件和图像刷新的，imshow则是用来更新对应窗口的图像，如果这两者在多线程的程序中滥用，会导致很多致命的错误，大部分是以栈错误为主，在这里我有一个比较好的解决多线程中管理opencv图像显示的办法，就是将所有的对图像显示和按键事件操作放入到一个单独的线程中，在这个线程，要做的是仅仅是收集外部处理过后的图像

- 以下代码，是针对与linux系统编写的

该类的使用方法
```c++
#include "imgManager.h"

int main() {
    //输入源，按照具体项目而定
	cv::VideoCapture cap(0);
	cv::Mat src;cap>>src;
	ImgManager iMag(cap);
	int src_id = iMag.add(src,"tracker");
	while(!iMag.isSignalLoss) {
        src = cv::Scalar(iMag.vMin,iMag.vMax,iMag.sMin);
		iMag.update(src_id,src);
        //iMag.remove(src_id)
	}
}

```

要用到的头文件
```c++
#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <sys/time.h>
#include <unistd.h>

using namespace std;
```

线程锁类
```c++
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
```
外部输出输入数据类，包括orb特征算子的描述符im，和关键点集kp
```c++
class DataSet{
public:
	cv::Mat color;
	std::vector<cv::KeyPoint> kp;
	cv::Mat im;
	void set(DataSet dataSet);
	void operator=(DataSet &dataSet);
};
```

图像管理类，这里我用了VideoCapture类作为主要输入源，具体输入源，需符合项目要求即可
```c++
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
```

cpp文件 类中函数的实现
```c++
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

```

编译用到的makefile参考
``` makefile
OPENCV = $(shell pkg-config --cflags opencv) $(shell pkg-config --libs opencv)


LINK_ALL = $(OPENCV)

SRC = $(CAMERASHIFT) $(GUIDENCE) imgManager.cpp exec.cpp
TAR = exec

$(TAR):$(SRC)
	g++ -o $(TAR) $(SRC) $(LINK_ALL) -z execstack -fno-stack-protector -g

clean:
	rm $(OBJ) $(TAR)

%.o:%.cpp
	g++ -I $(LINK_ALL) -o $@ -c $<

```
