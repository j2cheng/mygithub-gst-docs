/**
 * Copyright (C) 2016 to the present, Crestron Electronics, Inc.
 * All rights reserved.
 * No part of this software may be reproduced in any form, machine
 * or natural, without the express written consent of Crestron Electronics.
 *
 * \file        csioCommBase.cpp
 *
 * \brief
 *
 * \author      John Cheng
 *
 * \date        12/18/2018
 *
 * \note
 *
 *
 * \todo
 */
#include "csioCommBase.h"
/***************************** csioThreadBaseClass class **********************************************************/
int   csioThreadBaseClass::m_threadBaseID = 0;  //init. static

csioThreadBaseClass::csioThreadBaseClass():m_forceThreadExit(0),
m_threadObjLoopCnt(0),m_ThreadIsRunning(0),
m_debugLevel(eLogLevel_debug),//subclass should overwrite this
eventPushIndex(0),eventPopIndex(0),_validThread(false)
{
    _thread = 0;

    //Note: used only for debugging
    m_threadBaseID++;

    memset(eventPushArray,0,sizeof(eventPushArray));
    memset(eventPopArray,0,sizeof(eventPopArray));
}

csioThreadBaseClass::~csioThreadBaseClass()
{
    /* empty */
}

int csioThreadBaseClass::CreateNewThread(const char* name, pthread_attr_t* attr)
{
    int ret = pthread_create(&_thread, attr, ThreadEntryFunc, this);

    if(ret == 0)//success
    {
        if(name)
        {
            pthread_setname_np(_thread,name);
        }

        m_ThreadIsRunning = 1;
        _validThread = true;
    }

    return ret;
}
void csioThreadBaseClass::logEventPush(int e)
{
    eventPushArray[eventPushIndex++] = e;
    if( eventPushIndex >= MAX_DEBUG_ARRAY_SIZE)
        eventPushIndex = 0;
}

void csioThreadBaseClass::logEventPop(int e)
{
    eventPopArray[eventPopIndex++] = e;
    if( eventPopIndex >= MAX_DEBUG_ARRAY_SIZE)
        eventPopIndex = 0;
}

/*****************************end of csioThreadBaseClass class **********************************************************/

/***************************** csioEventQueueListBase class ******************************************/
csioEventQueueListBase::csioEventQueueListBase(int maxCnt):
m_message_list(),
m_debugLevel(eLogLevel_debug),
m_msgQ_pipe_fd(),
m_msgQ_pipe_readfd(-1),
m_msgQ_pipe_writefd(-1),
m_MsgQMtx(NULL)
{
    m_message_list.clear();
    m_MsgQMtx = new Mutex();

    m_iMaxCount     = maxCnt ;
    m_iOverFlowThds = (maxCnt*3)/4 ;

    if(pipe2(m_msgQ_pipe_fd, O_NONBLOCK) < 0)
    {
        __android_log_print (ANDROID_LOG_ERROR, "csioComm", "csioEventQueueListBase ERROR: failed to create m_msgQ_pipe_fd\n");
    }
    else
    {
        m_msgQ_pipe_readfd  = m_msgQ_pipe_fd[0];
        m_msgQ_pipe_writefd = m_msgQ_pipe_fd[1];
    }

    __android_log_print (ANDROID_LOG_INFO, "csioComm", "csioEventQueueListBase: created.\n");
}
csioEventQueueListBase::~csioEventQueueListBase()
{
    if(!m_message_list.empty())
    {
        for (std::list<csioEventQueueStruct*>::iterator it = m_message_list.begin(); it != m_message_list.end(); it++)
        {
            csioEventQueueStruct* tmp = *it;
            clearQAndDeleteQ(tmp);
        }

        m_message_list.clear();
    }

    if (m_MsgQMtx != NULL)
    {
        delete m_MsgQMtx;
        m_MsgQMtx = NULL;
    }

    closeAllSig();
    __android_log_print (ANDROID_LOG_INFO, "csioComm", "~csioEventQueueListBase: exit\n");
}
int csioEventQueueListBase::EnqueueAndSignal(csioEventQueueStruct& queue,bool lock)
{
    int overflow = 0;

    if(lock)
        m_MsgQMtx->lock();

    if(m_message_list.size() < m_iMaxCount) //hard limit
    {
        csioEventQueueStruct* tmpQPtr = new csioEventQueueStruct();
        if(tmpQPtr)
        {
            //make a copy of the Struct
            *tmpQPtr = queue;

            //push to the list
            m_message_list.push_back(tmpQPtr);

            //check the size of the list
            if(m_message_list.size() >= m_iOverFlowThds)    //start warning
            {
                overflow = 1;
                __android_log_print (ANDROID_LOG_ERROR, "csioComm", "csioEventQueueListBase: EnqueueAndSignal: overflow[%d].\n",GetEvntQueueCount());
            }

            signalWaitingThread(tmpQPtr->event_type);
        }
    }
    else
    {
        overflow = 1;
        __android_log_print (ANDROID_LOG_ERROR, "csioComm", "csioEventQueueListBase: EnqueueAndSignal: m_iMaxCount[%d] reached.\n",m_iMaxCount);
    }

    if(lock)
        m_MsgQMtx->unlock();

    __android_log_print (ANDROID_LOG_DEBUG, "csioComm", "csioEventQueueListBase[0x%x]: EnqueueAndSignal:list.size[%d],overflow[%d].\n",
                    this,m_message_list.size(),overflow);
    return overflow;
}
void csioEventQueueListBase::signalWaitingThread(int v)
{
    int buf = v;

    if(m_msgQ_pipe_writefd >= 0)
        write(m_msgQ_pipe_writefd, &buf, sizeof(buf));

    __android_log_print (ANDROID_LOG_DEBUG, "csioComm", "csioEventQueueListBase: signalWaitingThread: v[%d]\n",v);
}
int csioEventQueueListBase::waitMsgQueueSignal(int pollTimeoutInMs)
{
    int ret = 0;//1 == ok, 0 == failed

    if(GetEvntQueueCount() > 0)
        return 1;

    if(m_msgQ_pipe_readfd >= 0)
    {
        struct pollfd pfds[1] ;
        memset((char*)pfds, 0, sizeof(struct pollfd));

        pfds[0].fd     = m_msgQ_pipe_readfd;
        pfds[0].events = POLLIN;

        int result = poll(pfds, 1, pollTimeoutInMs);

        if( result < 0 )// -1 means error
        {
            __android_log_print (ANDROID_LOG_ERROR, "csioComm", "csioEventQueueListBase:waitMsgQueueSignal ERROR: poll returns: %d.\n",result);
        }
        else// = 0 time out, > 0 ok
        {
            //STRLOG(LOGLEV_VERBOSE,LOGCAT_DEFAULT, "csioEventQueueListBase poll returns: %d.\n",result);
            //Note:flush out pipe
            if ( pfds[0].revents & POLLIN )  //POLLIN == 1
            {
                //read out pipe
                read(m_msgQ_pipe_readfd, &ret, sizeof(ret));
                //STRLOG(LOGLEV_VERBOSE,LOGCAT_DEFAULT, "csioEventQueueListBase:waitMsgQueueSignal read returns rc[%d],ret[%d]\n",rc,ret);
            }
        }
    }
    else
    {
        __android_log_print (ANDROID_LOG_ERROR, "csioComm", "csioEventQueueListBase:waitMsgQueueSignal ERROR: m_msgQ_pipe_readfd is NULL.\n");
    }

    __android_log_print (ANDROID_LOG_DEBUG, "csioComm", "csioEventQueueListBase: waitMsgQueueSignal exit[%d]\n",pollTimeoutInMs);
    return ret;
}
bool csioEventQueueListBase::GetFromQueueList(csioEventQueueStruct** ppQueue ,bool lock )
{
    if(m_message_list.empty() || !ppQueue)
        return false;

    if(lock)
        m_MsgQMtx->lock();

    csioEventQueueStruct* tmpQ = m_message_list.front();
    m_message_list.pop_front();

    *ppQueue = tmpQ;

    //Note: flush pipe here.
    if(m_msgQ_pipe_readfd >= 0)
    {
        int ret = 0;
        read(m_msgQ_pipe_readfd, &ret, sizeof(ret));
    }

    if(lock)
        m_MsgQMtx->unlock();

    __android_log_print (ANDROID_LOG_DEBUG, "csioComm", "csioEventQueueListBase: GetFromBuffer:list.size[%d].\n",m_message_list.size());
    return true;
}
void csioEventQueueListBase::closeAllSig()
{
    if(m_msgQ_pipe_readfd >= 0)
    {
        close(m_msgQ_pipe_fd[0]);
        m_msgQ_pipe_readfd = -1;
    }

    if(m_msgQ_pipe_writefd >= 0)
    {
        close(m_msgQ_pipe_fd[1]);
        m_msgQ_pipe_writefd = -1;
    }

    __android_log_print (ANDROID_LOG_INFO, "csioComm", "csioEventQueueListBase: closeAllSig: exit\n");
}
int csioEventQueueListBase::GetEvntQueueCount(void)
{
    if(!m_message_list.empty())
    {
        return m_message_list.size();
    }
    else
    {
        return 0;
    }
}
/* queue and its bufferPtr will be deleted. */
void csioEventQueueListBase::clearQAndDeleteQ(csioEventQueueStruct* evntQueue)
{
    if(evntQueue)
    {
        if(evntQueue->buffPtr)
            del_Q_buf(evntQueue->buffPtr);

        delete evntQueue;
    }
}
/***************************** end of csioEventQueueListBase class ******************************************/

/***************************** csioTimerClockBase class ******************************************/
csioTimerClockBase::csioTimerClockBase(int MaxEvts,int MaxTimers):
m_debugLevel(eLogLevel_debug),
pTimeoutInMsArray(NULL),pEventTime(NULL),pTimeoutWaitEvents(NULL),
m_MaxEvts(MaxEvts),m_MaxTimers(MaxTimers)
{
    pTimeoutInMsArray = new int[MaxTimers];

    pEventTime = new struct timeval[MaxEvts];
    pTimeoutWaitEvents = new struct timespec[MaxTimers];

    for(int i = 0; i < MaxTimers; i++)
        resetTimeout(i);
}
csioTimerClockBase::~csioTimerClockBase()
{
    if(pTimeoutInMsArray)
    {
        delete [] pTimeoutInMsArray;
        pTimeoutInMsArray = NULL;
    }

    if(pTimeoutWaitEvents)
    {
        delete [] pTimeoutWaitEvents;
        pTimeoutWaitEvents = NULL;
    }

    if(pEventTime)
    {
        delete [] pEventTime;
        pEventTime = NULL;
    }
}

void csioTimerClockBase::resetTimeout(int index)
{
    if(index >= 0 && index < m_MaxTimers)
    {
        if(pTimeoutWaitEvents)
        {
            pTimeoutWaitEvents[index].tv_sec  = 0;
            pTimeoutWaitEvents[index].tv_nsec = 0;
        }

        if(pTimeoutInMsArray)
            pTimeoutInMsArray[index] = 0;
    }
    __android_log_print (ANDROID_LOG_DEBUG, "csioComm", "csioTimerClockBase: resetTimeout: resetTimeout[%d] sub class.\n",index);
}
void csioTimerClockBase::setTimeout(int index, int inMs)
{
    if(pTimeoutWaitEvents && index >= 0 && index < m_MaxTimers)
    {
        clock_gettime(CLOCK_MONOTONIC, &pTimeoutWaitEvents[index]);

        if(pTimeoutInMsArray)
            pTimeoutInMsArray[index] = inMs;

        __android_log_print (ANDROID_LOG_DEBUG, "csioComm", "csioTimerClockBase: setTimeout: setTimeout[%d].\n",index);
    }
}
bool csioTimerClockBase::isTimeout(int index)
{
    // if(LOGLEV_VERBOSE > eLogLevel_extraVerbose)
    //     STRLOG(LOGLEV_EXTRAVERBOSE,LOGCAT_DEFAULT,  "csioTimerClockBase: isTimeout[%d] pTimeoutWaitEvents.tv_sec[%d],pTimeoutInMsArray[%d].\n",
    //                     index,pTimeoutWaitEvents[index].tv_sec,pTimeoutInMsArray[index]);

    if(pTimeoutWaitEvents && index >= 0 && index < m_MaxTimers && pTimeoutInMsArray)
    {
        if(pTimeoutWaitEvents[index].tv_sec  &&
           pTimeoutInMsArray[index])
        {
            struct timespec ts_now;
            clock_gettime(CLOCK_MONOTONIC, &ts_now);

            //TODO: depends on the resolution of the timer ticks
            int diffInMs = csioTimerClockBase::takeDiff(pTimeoutWaitEvents[index].tv_sec,
                                            pTimeoutWaitEvents[index].tv_nsec / 1000,
                                            ts_now.tv_sec,
                                            ts_now.tv_nsec / 1000);

            // if(LOGLEV_VERBOSE > eLogLevel_extraVerbose)
            //     STRLOG(LOGLEV_EXTRAVERBOSE,LOGCAT_DEFAULT,  "csioTimerClockBase: index[%d] ([%d]-[%d]) = [%d]ms.\n",
            //                 index,ts_now.tv_sec,pTimeoutWaitEvents[index].tv_sec,diffInMs);

            if(diffInMs > pTimeoutInMsArray[index])
            {
                resetTimeout(index);

                return true;
            }
        }
    }

    return false;
}
void csioTimerClockBase::recordEventTimeStamp(int index)
{
    if(pEventTime && index >= 0 && index < m_MaxEvts)
        gettimeofday(&pEventTime[index], NULL);
}
void csioTimerClockBase::convertTime(int index,char* time_string,int sizeofstring,long& milliseconds)
{
    if(!time_string)
        return;

    if(pEventTime && index >= 0 && index < m_MaxEvts)
    {
        struct timeval t = pEventTime[index];
        struct tm* ptm = localtime (&t.tv_sec);

        strftime (time_string, sizeofstring, "%Y-%m-%d %H:%M:%S", ptm);
        milliseconds = t.tv_usec / 1000;
    }

    //STRLOG(LOGLEV_VERBOSE,LOGCAT_DEFAULT, "csioTimerClockBase: convertTime: base class\n");
}

int csioTimerClockBase::takeDiff(int _start_sec, int _start_usec, int _end_sec, int _end_usec)
{
    uint64_t diff;

    diff = MILLION * (_end_sec - _start_sec) + _end_usec - _start_usec;
    return (diff / 1000);//change to ms
}
/***************************** end of csioTimerClockBase class ******************************************/
