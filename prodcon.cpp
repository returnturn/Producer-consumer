#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <queue>
#include <chrono>
#include <iomanip>
#include <thread>
#include <future>
#include <vector>
#include <mutex>
#include <condition_variable>
using namespace std;

unsigned int queueSize;
queue<int> work;
mutex mu,log;
condition_variable produce, consume;

extern void Trans( int n );
extern void Sleep( int n );

string logFileName;
bool exi=false;
vector<int> summary;
double netTime;

//read inputs and push jobs to queue
void producer(chrono::time_point<std::chrono::high_resolution_clock> start){
    while(1){
        //if queue reaches limit then wait until consume
        unique_lock<mutex> lck(mu);
        produce.wait(lck, []{return work.size()!=queueSize;});
        char cstr[logFileName.size()+1];
        strcpy(cstr,logFileName.c_str());
        ofstream logFile;
        string cur;
        cin>>cur;
        //if reaches end of input then tell consumers ready to quit, the producer will quit as well
        if(cin.eof()){
            log.lock();
            logFile.open(cstr, std::ios_base::app);
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> ms = end - start;
            netTime=ms.count()/1000;
            logFile<<"   "<<fixed<<setprecision(3)<<ms.count()/1000<<" ID= 0      "<<"End           "<<"    // End of input for producer"<<endl;
            logFile.close();
            log.unlock();
            exi=true;
            break;
        }
        //the number followed by T and S
        int load = (stoi)(cur.substr(1));
        //if it's T push current job to the queue, it's called "work"
        if(cur[0]=='T'){
            work.push(load);
            int cur_size=work.size();
            lck.unlock();
            log.lock();
            logFile.open(cstr, std::ios_base::app);
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> ms = end - start;
            netTime=ms.count()/1000;
            logFile<<"   "<<fixed<<setprecision(3)<<ms.count()/1000<<" ID= 0 Q= "<<max(0,cur_size)<<" Work"<<right<<setw(10)<<load<<"    // Parent receives work with n="<<load<<endl;
            logFile.close();
            log.unlock();
            ++summary[0];
        }
        //if it's S, just call Sleep(x)
        else if(cur[0]=='S'){
            log.lock();
            logFile.open(cstr, std::ios_base::app);
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> ms = end - start;
            netTime=ms.count()/1000;
            logFile<<"   "<<fixed<<setprecision(3)<<ms.count()/1000<<" ID= 0      "<<"Sleep"<<right<<setw(9)<<load<<"    // Parent sleeps, n="<<load<<endl;
            logFile.close();
            log.unlock();
            ++summary[4];
            Sleep(load);
        }
    }
}

//take jobs from the queue and call Trans(x)
void consumer(int i,chrono::time_point<std::chrono::high_resolution_clock> start){
    while(1){
        //ask for work first
        char cstr[logFileName.size()+1];
        strcpy(cstr,logFileName.c_str());
        ofstream logFile;
        log.lock();
        logFile.open(cstr, std::ios_base::app);
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> ms = end - start;
        netTime=ms.count()/1000;
        logFile<<"   "<<fixed<<setprecision(3)<<ms.count()/1000<<" ID= "<<i<<"      Ask           "<<"    // Thread "<<i<<" asks for work"<<endl;
        logFile.close();
        log.unlock();
        ++summary[1];
        //if no job then wait until there're some; if receives exi (producer said quit!) then break immediately
        while(work.size()==0){
            if(exi and work.empty()) break;
        }
        if(exi and work.empty()) break;
        unique_lock<mutex> lck(mu);
        if(work.size()==0 and exi){
            lck.unlock();
            break;
        }
        int load=work.front();
        if(load){
        work.pop();
        int cur_size=work.size();
        lck.unlock();
        log.lock();
        logFile.open(cstr, std::ios_base::app);
        end = chrono::high_resolution_clock::now();
        ms = end - start;
        netTime=ms.count()/1000;
        //received job
        logFile<<"   "<<fixed<<setprecision(3)<<ms.count()/1000<<" ID= "<<i<<" Q= "<<max(0,cur_size)<<" Receive"<<right<<setw(7)<<load<<"    // Thread "<<i<<" takes work, n="<<load<<endl;
        logFile.close();
        log.unlock();
        ++summary[2];
        ++summary[i+4];
        Trans(load);
        log.lock();
        logFile.open(cstr, std::ios_base::app);
        end = chrono::high_resolution_clock::now();
        ms = end - start;
        netTime=ms.count()/1000;
        //complete job
        logFile<<"   "<<fixed<<setprecision(3)<<ms.count()/1000<<" ID= "<<i<<"      Complete"<<right<<setw(6)<<load<<"    // Thread "<<i<<" completes task, n="<<load<<endl;
        logFile.close();
        log.unlock();
        ++summary[3];
        if(exi and work.empty()) break;}
    }
}

//take inputs and do prints
int main(int argc, char** argv){
    //record starting time
    auto start = chrono::high_resolution_clock::now();
    int numConsumers=atoi(argv[1]);
    queueSize=2*numConsumers;
    string id="";
    if(argc==3){
        string temp(argv[2]);
        id="."+temp;
    }
    //create file (or clean)
    logFileName="prodcon" + id + ".log";
    char cstr[logFileName.size()+1];
    strcpy(cstr,logFileName.c_str());
    ofstream newFile(cstr);
    newFile.close();
    //summary is a vector to record data
    for(int i=0;i<5;++i){
        summary.push_back(0);
    }
    //create threads
    thread consumers[numConsumers];
    for(int i=0;i<numConsumers;++i){
        summary.push_back(0);
        consumers[i]=thread(consumer, i+1,start);
    }
    thread pproducer = thread(producer, start);
    //join threads
    pproducer.join();
    for(int i=0;i<numConsumers;++i){
        consumers[i].join();
    }
    //print summary
    ofstream logFile;
    logFile.open(cstr, std::ios_base::app);
    logFile<<"Summary:"<<endl;
    logFile<<"    Work"<<right<<setw(11)<<summary[0]<<"                   // Producer: # of ‘T’ commands"<<endl;
    logFile<<"    Ask"<<right<<setw(12)<<summary[1]<<"                   // Consumer: # of asks for work"<<endl;
    logFile<<"    Receive"<<right<<setw(8)<<summary[2]<<"                   // Consumer: # work assignments"<<endl;
    logFile<<"    Complete"<<right<<setw(7)<<summary[3]<<"                   // Consumer: # completed tasks"<<endl;
    logFile<<"    Sleep"<<right<<setw(10)<<summary[4]<<"                   // Producer: # of ‘S’ commands"<<endl;
    for(int i=0;i<numConsumers;++i){
        logFile<<"    Thread  "<<i+1<<right<<setw(6)<<summary[i+5]<<"                   // Number of ‘T’s completed by "<<i+1<<endl;
    }
    double transPerSecond = 1/netTime*summary[0];
    logFile<<"Transactions per second: "<<fixed<<setprecision(2)<<transPerSecond<<"        // "<<summary[0]<<" pieces of work in "<<fixed<<setprecision(3)<<netTime<<" secs"<<endl;
    logFile.close();
    return 0;
}