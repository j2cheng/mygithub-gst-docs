/**
 * Copyright (C) 2016 to the present, Crestron Electronics, Inc.
 * All rights reserved.
 * No part of this software may be reproduced in any form, machine
 * or natural, without the express written consent of Crestron Electronics.
 *
 * \file        crestDBIntf.cpp
 *
 * \brief
 *
 * \author      John Cheng
 *
 * \date        12/18/2016
 *
 * \note
 *
 *
 * \todo
 */

#include <string>

#include <unistd.h>  // for sleep()
#include <errno.h>   //for errno
#include <stdio.h>   //for perror
#include <stdlib.h>  //for EXIT_FAILURE
#include <sys/time.h> //for gettimeofday

//#include "json/json.h"

//#include "cresstore.h"

//#include "crestDBIntf_utils/crestDBIntf_utils.h"

#include "crestDBIntf.h"
//#include "csioutils.h"   //Note: this header need csioCommonShare.h

#define MAX_PROJCT_OBJ          1
#define CRESTORE_SUBSCRIBE_STR      "Device:XioSubscription*"
#define CRESTORE_ROUTING_STR        "Device:Routing*"

#define DB_SUBSCRIBE_STR            "Device:XioSubscription"
#define DB_ROUTING_STR              "Device:Routing"

#define CSIO_LOG

static void ProjectSendEvent(int iId, int evnt, int data_size, void* bufP);

CCrestDBIntfProject** CrestDBIntfProjList = NULL ;
int CrestDBIntfProjDebugLevel = CSIO_DEFAULT_LOG_LEVEL;
static Mutex gProjectsLock;

extern int start_Subscribing(int subscr_id,char * url,int connMode);
extern void stop_Subscribing(int id,int connMode);
/*********************functions called from external**************************/
void CrestDBIntfProjectInit()
{
    CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: CrestDBIntfProjectInit enter\n");

    if(CrestDBIntfProjList == NULL)
    {
        CrestDBIntfProjList = new CCrestDBIntfProject* [MAX_PROJCT_OBJ];
        CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: new CrestDBIntfProjList:0x%x\n",CrestDBIntfProjList);

        //clean up the list
        for(int i = 0; i < MAX_PROJCT_OBJ; i++)
            CrestDBIntfProjList[i] = NULL;

        //create only one project object for now
        CrestDBIntfProjList[0] = new CCrestDBIntfProject(0);

        CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: CrestDBIntfProjList[0]:0x%x\n",CrestDBIntfProjList[0]);

        //create only one project thread for now
        CrestDBIntfProjList[0]->CreateNewThread("DBIntf",NULL);
    }
    else
    {
        CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: CrestDBIntf_project project already created.\n");
    }

    CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: CrestDBIntfProjectInit exit\n");
}

void CrestDBIntfProjectDeInit()
{
    CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: CrestDBIntfProjectDeInit enter\n");
    if(CrestDBIntfProjList)
    {
        for(int i = 0; i < MAX_PROJCT_OBJ; i++)
        {
            if(CrestDBIntfProjList[i])
            {
                //remove all CrestDBIntf tasks this project created.
                CrestDBIntfProjList[i]->removeAllTasks();

                //tell thread to exit
                CrestDBIntfProjList[i]->exitThread();

                //wait until thread exits
                CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: [%d]call WaitForThreadToExit[0x%x]\n",i,CrestDBIntfProjList[i]);
                CrestDBIntfProjList[i]->WaitForThreadToExit();
                CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: Wait is done\n");

                //delete the object, and set list to NULL
                delete CrestDBIntfProjList[i];
                CrestDBIntfProjList[i] = NULL;

                CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: delete CrestDBIntfProjList[%d] is DONE\n",i);
            }
        }

        delete[] CrestDBIntfProjList;
        CrestDBIntfProjList = NULL;
    }
    else
    {
        CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: no CrestDBIntf project has created.\n");
    }
    CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: CrestDBIntfProjectDeInit exit\n");
}

void CrestDBIntf_SetDebugLevel(int level)
{
    CrestDBIntfProjDebugLevel = level;

    gProjectsLock.lock();

    if(level > eLogLevel_extraVerbose)
    {
        int gCrestDBIntfUtilsDebugLevel = eLogLevel_extraVerbose;//not used
    }
    else if(CrestDBIntfProjList)
    {
        for(int i = 0; i < MAX_PROJCT_OBJ; i++)
        {
            if(CrestDBIntfProjList[i])
            {
                CrestDBIntfProjList[i]->setDebugLevel(level);
                CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: [%d]set debug level to: %d.\n",i,level);
            }
        }
    }
    else
    {
        CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: no project is running\n");
    }

    gProjectsLock.unlock();
}

void CrestDBIntf_ProjectDumpClassPara(int level)
{
    gProjectsLock.lock();

    CSIO_LOG(eLogLevel_info, "CrestDBIntf_project: DebugLevel: %d.\n",CrestDBIntfProjDebugLevel);
    if(CrestDBIntfProjList)
    {
        for(int i = 0; i < MAX_PROJCT_OBJ; i++)
        {
            if(CrestDBIntfProjList[i])
            {
                CSIO_LOG(eLogLevel_info, "CrestDBIntf_project: Project ID: %d, taskObjList[0x%x].\n",
                         i,CrestDBIntfProjList[i]->m_CrestDBIntfTaskObjList);

                CrestDBIntfProjList[i]->DumpClassPara(level);

//                if(CrestDBIntfProjList[i]->m_CrestDBIntfTaskObjList)
//                {
//                    for(int j = 0; j < MAX_TOTAL_TASKS; j++)
//                    {
//                        if(CrestDBIntfProjList[i]->m_CrestDBIntfTaskObjList[j])
//                        {
//                            CrestDBIntfProjList[i]->m_CrestDBIntfTaskObjList[j]->DumpClassPara(level);
//                        }
//                    }
//                }
            }
        }
    }
    else
    {
        CSIO_LOG(eLogLevel_info, "CrestDBIntf_project: no project is running\n");
    }

    gProjectsLock.unlock();
}
void CrestDBIntf_add_rtspUrlToDB(const char*)
{
    // void * cs = CrestDBIntf_takeCresDBIntf();
    // if(cs)
    // {

    // }

    // CrestDBIntf_releaseCresDBIntf();
}

void* CrestDBIntf_takeCresDBIntf()
{
    void* retV = NULL;
    gProjectsLock.lock();

    CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_takeCresDBIntf.\n");
    if(CrestDBIntfProjList)
    {
        if(CrestDBIntfProjList[0])//only one project for now
        {
            retV = 0;//CrestDBIntfProjList[0]->m_takeCS_and_lock();

            CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: taskObjList[0x%x],retV[0x%x].\n",
                     CrestDBIntfProjList[0]->m_CrestDBIntfTaskObjList,retV);
        }
    }
    else
    {
        CSIO_LOG(eLogLevel_info, "CrestDBIntf_project: no project is running\n");
    }

    gProjectsLock.unlock();

    return retV;
}
void CrestDBIntf_releaseCresDBIntf()
{
    gProjectsLock.lock();

    CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_releaseCresDBIntf.\n");
    if(CrestDBIntfProjList)
    {
        if(CrestDBIntfProjList[0])//only one project for now
        {
            CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: taskObjList[0x%x].\n",
                     CrestDBIntfProjList[0]->m_CrestDBIntfTaskObjList);

//            CrestDBIntfProjList[0]->unlockCresstoreDb();
        }
    }
    else
    {
        CSIO_LOG(eLogLevel_info, "CrestDBIntf_project: no project is running\n");
    }

    gProjectsLock.unlock();

    return ;
}

/*********************local static functions **************************/
static void convertTime(struct timeval t,char* time_string,int sizeofstring,long& milliseconds)
{
    struct tm* ptm = localtime (&t.tv_sec);
    strftime (time_string, sizeofstring, "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = t.tv_usec / 1000;
}

//Note: calling function should gProjectsLock.lock
static void ProjectSendEvent(int iId, int evnt, int data_size, void* bufP)
{
    //only one project for now
    if(CrestDBIntfProjList && CrestDBIntfProjList[0])
    {
        CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: [%d]call sendEvent.\n",iId);
        csioEventQueueStruct EvntQ;
        memset(&EvntQ,0,sizeof(csioEventQueueStruct));
        EvntQ.obj_id = iId;
        EvntQ.event_type = evnt;
        EvntQ.buf_size   = data_size;
        EvntQ.buffPtr    = bufP;

        CrestDBIntfProjList[0]->sendEvent(&EvntQ);
        CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: [%d]done sendEvent.\n",iId);
    }
    else
    {
        CSIO_LOG(CrestDBIntfProjDebugLevel, "CrestDBIntf_project: no proxy project is running\n");
    }
}

/*********************CCrestDBIntfProject functions **************************/
CCrestDBIntfProject::CCrestDBIntfProject(int iId):
m_projectID(iId)//,
//m_cresstoreDBPtr(NULL)
{
    m_debugLevel = CrestDBIntfProjDebugLevel;

    m_projEvent  = new csioEventQueueListBase(CREST_DB_EVENTS_MAX);

    // m_projEventQ = new CrestDBIntfEventRingBuffer(EVNT_DEFAULT_QUEUE_SIZE);

    mLock        = new Mutex();

    m_CrestDBIntfTaskObjList = new CCrestDBIntfManager* [MAX_TOTAL_TASKS];

    for(int i = 0; i < MAX_TOTAL_TASKS; i++)
        m_CrestDBIntfTaskObjList[i] = NULL;

    for(int i = 0; i < MAX_XIO_SUBCRIPTION; i++)
    {
        m_subscrUuidArray[i].clear();
        m_subscrUriArray[i].clear();
    }

    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: CCrestDBIntfProject exit\n");
}

CCrestDBIntfProject::~CCrestDBIntfProject()
{
    removeAllTasks();
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: ~CCrestDBIntfProject delete Tasks is DONE\n");

    if(m_projEvent)
        delete m_projEvent;

    if(m_projEventQ)
        delete m_projEventQ;
    
    if(mLock)
        delete mLock;

    //Note:removeAllTasks() should be called before delete this object

    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: ~CCrestDBIntfProject exit\n");
}

void CCrestDBIntfProject::DumpClassPara(int level)
{
    CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: CrestDBIntf project m_projectID:       %d.\n",m_projectID);
    CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: CrestDBIntf project threadObjLoopCnt:  %d.\n",m_threadObjLoopCnt);

    CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: ThredId 0x%x\n", (unsigned long int)getThredId());
    CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: m_debugLevel %d\n", m_debugLevel);

    if(m_projEventQ)
        CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: Event queued up %d\n", m_projEventQ->GetEvntQueueCount());
    
    for(int i = 0; i < MAX_XIO_SUBCRIPTION; i++)
    {
        if(m_subscrUuidArray[i].empty())
            CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: Subscription[%d] uuid is NULL\n", i);
        else
        {
            CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: Subscription[%d] uuid is %s\n", i, m_subscrUuidArray[i].c_str());
            if(m_subscrUriArray[i].empty())
                CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: Subscription[%d] uri is NULL\n", i);
            else
                CSIO_LOG(eLogLevel_info, "--CrestDBIntf_project: Subscription[%d] uri is %s\n", i, m_subscrUriArray[i].c_str());
        }
    }
}

void CCrestDBIntfProject::cresstoreCallback(char * key, char * json, void * userptr)
{
    CCrestDBIntfProject* projPtr = (CCrestDBIntfProject*)userptr;
    if(!projPtr || !key || ! json)
        CSIO_LOG(eLogLevel_error,"CrestDBIntf_project: pionter is NULL");

    CSIO_LOG(projPtr->m_debugLevel, "CrestDBIntf_project: cresstoreCallback[0x%x] key[%s] was set to[%s]\r\n",
             userptr, key, json);

    if(!strcmp(DB_SUBSCRIBE_STR, key))
    {
        gProjectsLock.lock();

        csioEventQueueStruct EvntQ;
        memset(&EvntQ,0,sizeof(csioEventQueueStruct));
        EvntQ.obj_id     = 0;
        EvntQ.event_type = CREST_DB_EVENTS_SUBSCRIB;
        EvntQ.buf_size   = strlen(json);
        EvntQ.buffPtr    = json;

        projPtr->sendEvent(&EvntQ);

        gProjectsLock.unlock();
        CSIO_LOG(projPtr->m_debugLevel,"CrestDBIntf_project: SUBSCRIBE command posted.");
    }
    else if(!strcmp(DB_ROUTING_STR, key))
    {
        gProjectsLock.lock();

        csioEventQueueStruct EvntQ;
        memset(&EvntQ,0,sizeof(csioEventQueueStruct));
        EvntQ.obj_id     = 0;
        EvntQ.event_type = CREST_DB_EVENTS_ROUTING;
        EvntQ.buf_size   = strlen(json);
        EvntQ.buffPtr    = json;

        projPtr->sendEvent(&EvntQ);

        gProjectsLock.unlock();
        CSIO_LOG(projPtr->m_debugLevel,"CrestDBIntf_project: ROUTING command posted.");
    }
    else
    {
        CSIO_LOG(eLogLevel_error,"CrestDBIntf_project: cresstoreCallback unprocessed key : %s", key);
    }

    /*
        parsestring = jsonReader.parse(json, jsonObj);
        if(parsestring)
        {
            for(Json::Value::iterator it=jsonObj.begin(); it!=jsonObj.end(); ++it)
            {
                const Json::Value& layer_1_jsonObj = *it_1;
                if(!strcmp("Device", it_1.memberName())//got Device string
                {
                    for(Json::Value::iterator it_sub=layer_1_jsonObj.begin(); it_sub!=layer_1_jsonObj.end(); ++it_sub)
                    {
                        const Json::Value& layer_2_jsonObj = *it_sub;
                        if(!strcmp("XioSubscription", it_sub.memberName())//got XioSubscription string
                        {
                            for(Json::Value::iterator it_XioSub=layer_2_jsonObj.begin(); it_XioSub!=layer_2_jsonObj.end(); ++it_XioSub)
                            {
                                gProjectsLock.lock();

                                std::string uuidString = it_XioSub.memberName();
                                std::string uriString  = it_XioSub["RtspUri"].asString();

                                CrestDBIntfEventConfig evntConf;
                                memset(&evntConf,0,sizeof(CrestDBIntfEventConfig));
                                evntConf.type = CREST_DB_EVENTS_ROUTING  or CREST_DB_EVENTS_SUBSCRIB;
                                evntConf.pUuidString = uuidString.c_str();
                                evntConf.pURLString  = uriString.c_str();

                                csioEventQueueStruct EvntQ;
                                memset(&EvntQ,0,sizeof(csioEventQueueStruct));
                                EvntQ.obj_id     = 0;
                                EvntQ.event_type = CREST_DB_EVENTS_SUBSCRIB;
                                EvntQ.buf_size   = sizeof(CrestDBIntfEventConfig);
                                EvntQ.buffPtr    = &evntConf.;

                                projPtr->sendEvent(&EvntQ);

                                gProjectsLock.unlock();
                                CSIO_LOG(projPtr->m_debugLevel,"CrestDBIntf_project: SUBSCRIBE command posted.");
                            }

                        }
                        else if(!strcmp("Routing", it.memberName())//got Routing string
                        {
                            gProjectsLock.lock();

                            csioEventQueueStruct EvntQ;
                            memset(&EvntQ,0,sizeof(csioEventQueueStruct));
                            EvntQ.obj_id     = 0;
                            EvntQ.event_type = CREST_DB_EVENTS_ROUTING;
                            EvntQ.buf_size   = strlen(json);
                            EvntQ.buffPtr    = json;

                            projPtr->sendEvent(&EvntQ);

                            gProjectsLock.unlock();
                            CSIO_LOG(projPtr->m_debugLevel,"CrestDBIntf_project: ROUTING command posted.");

                        }
                    }
                }
                else
                {
                    CSIO_LOG(eLogLevel_error,"CrestDBIntf_project: cresstoreCallback this is not "Device"\n");
                }
            }
        }


      CrestDBIntfEventConfig evntConf;
      memset(&evntConf,0,sizeof(CrestDBIntfEventConfig));

      evntConf.type = CREST_DB_EVENTS_ROUTING  or CREST_DB_EVENTS_SUBSCRIB;
      evntConf.pUuidString = ;
      evntConf.pURLString  = ;
     */
}

void* CCrestDBIntfProject::ThreadEntry()
{
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: Enter ThreadEntry, m_projectID ID:%d.\n",m_projectID);

    CRESSTORERESULT result;
    Json::Reader jsonReader;
    Json::Value  jsonObj;
    bool parsestring;

    int wtRtn  = 0;
    int evntId = CREST_DB_EVENTS_MAX;

    m_cresstoreDBPtr = cresstore_create();

    if(!m_projEvent || !m_projEventQ || !m_cresstoreDBPtr)
    {
        CSIO_LOG(eLogLevel_error, "CrestDBIntf_project: m_projectID ID:%d exiting...  m_projEvent[0x%x],m_projEventQ[0x%x],m_cresstoreDBPtr[0x%x]\n",
                m_projectID,m_projEvent,m_projEventQ,m_cresstoreDBPtr);

        m_destroyCresDB();

        //thread exit here
        m_ThreadIsRunning = 0;
        return NULL;
    }

    result = cresstore_subscribe(m_cresstoreDBPtr, CRESTORE_SUBSCRIBE_STR, cresstoreCallback, this);
    if(result != CRESSTORE_SUCCESS)
    {
        CSIO_LOG(eLogLevel_error, "CrestDBIntf_project: cresstore_subscribe(Device:XioSubscription) failed with error %d.", result);

        m_destroyCresDB();

        m_ThreadIsRunning = 0;
        return NULL;
    }

    //Note: one m_cresstoreDBPtr can only register one callback
    result = cresstore_subscribe(m_cresstoreDBPtr, CRESTORE_ROUTING_STR, cresstoreCallback, this);
    if(result != CRESSTORE_SUCCESS)
    {
        CSIO_LOG(eLogLevel_error, "CrestDBIntf_project: cresstore_subscribe(Device:Routing) failed with error %d.", result);

        m_destroyCresDB();

        m_ThreadIsRunning = 0;
        return NULL;
    }

    m_CrestDBIntfTaskObjList[0] = new CCrestDBIntfManager(m_projectID,m_cresstoreDBPtr);
    //start thread
    m_CrestDBIntfTaskObjList[0]->CreateNewThread();

    for(;;)
    {
        //sleep if nothing queued
        if(m_projEventQ->GetEvntQueueCount() == 0)
        {
            //NOTE: currently, this evntId is not being used to identify anything
            wtRtn = m_projEvent->waitForEvent(&evntId,THREAD_LOOP_TO);
        }
        else//consume all events here
        {
            CSIO_LOG(eLogLevel_extraVerbose, "CrestDBIntf_project: skip wait, GetEvntQueueCount:%d\n",
                     m_projEventQ->GetEvntQueueCount());
        }

        m_threadObjLoopCnt++;

        //TODO: we need to check wtRtn here
        //CSIO_LOG(eLogLevel_extraVerbose, "CrestDBIntf_project: waitForEvent return:%d, event ID:%d\n",wtRtn,evntId);

        csioEventQueueStruct evntQ;

        if(m_projEventQ->GetFromBuffer(&evntQ))
        {
            CSIO_LOG(m_debugLevel+1, "CrestDBIntf_project: evntQ is:size[%d],type[%d],iId[%d],buffPtr[0x%x]\n",\
                     evntQ.buf_size,evntQ.event_type,evntQ.obj_id,evntQ.buffPtr);

            logEventPop(evntQ.event_type);

            switch (evntQ.event_type)
            {
                case CREST_DB_EVENTS_SUBSCRIB:
                {
                    if(evntQ.buffPtr && evntQ.buf_size)
                    {
                        parsestring = jsonReader.parse((char*)evntQ.buffPtr, jsonObj);
                        if(parsestring)
                        {
                            CSIO_LOG(ABOVE_DEBUG_VERB(m_debugLevel),"CrestDBIntf_project: parsestring success %d\n", parsestring);

                            //one way to go through list of keys
                            for(Json::Value::iterator it=jsonObj.begin(); it!=jsonObj.end(); ++it)
                            {
                                const Json::Value& uuidJObj = *it;
                                std::string memberString = it.memberName();

                                CSIO_LOG(ABOVE_DEBUG_VERB(m_debugLevel),"CrestDBIntf_project: member name[%s]\n", memberString.c_str());

                                //asString() is slow(it returns stl::string), but it can handle zero characters: returns ""
                                //asCString() is the fast(it returns char*),it is only suitable for strings without zero characters.
                                std::string UniqueIdString = uuidJObj["UniqueId"].asString();
                                CSIO_LOG(ABOVE_DEBUG_VERB(m_debugLevel),"CrestDBIntf_project: UniqueIdString [%s]\n\n", UniqueIdString.c_str());

                                std::string uriString = uuidJObj["RtspUri"].asString();
                                int   uriStringSize = uuidJObj["RtspUri"].asString().size();

                                CSIO_LOG(ABOVE_DEBUG_VERB(m_debugLevel),"CrestDBIntf_project: RtspUri size :%d\n", uriStringSize);

                                int   subscribe_Id;
                                std::string& uuidString = memberString;

                                //Note: isThisUuidSubscribed() will return true with subscribe_Id updated
                                if(isThisUuidSubscribed(uuidString,subscribe_Id))
                                {
                                    //TODO: find out the correct way of deleting(unsubscriibe)!!!
                                    if(uriStringSize == 0)
                                    {
                                        CSIO_LOG(m_debugLevel,"CrestDBIntf_project: unsubscribe[%d] this uuid:%s\n", subscribe_Id,uuidString.c_str());
                                        deleteThisItem(subscribe_Id);
                                        stop_Subscribing(subscribe_Id,RX_CONN_SUBSCRIBING);
                                    }
                                    else
                                    {
                                        //if url has changed
                                        if(isUriChanged(subscribe_Id,uriString))//uriString.compare(m_subscrUriArray[index]) != 0)//compare NOT equal
                                        {
                                            storeNewUri(subscribe_Id,uriString);
                                            CSIO_LOG(m_debugLevel,"CrestDBIntf_project: unsubscribe this uuid:%s\n",uuidString.c_str());
                                            stop_Subscribing(subscribe_Id,RX_CONN_SUBSCRIBING);

                                            CSIO_LOG(m_debugLevel,"CrestDBIntf_project: subscribe to new url:%s\n",uriString.c_str());
                                            start_Subscribing(subscribe_Id,(char*)uriString.c_str(),RX_CONN_SUBSCRIBING);
                                        }
                                    }
                                }
                                else
                                {
                                    subscribe_Id = storeUuidAndUri(uuidString,uriString);
                                    if(subscribe_Id == -1)
                                    {
                                        CSIO_LOG(eLogLevel_error,"CrestDBIntf_project: subscribe exceeded limitation of %d\n",MAX_XIO_SUBCRIPTION);
                                    }
                                    else
                                    {
                                        CSIO_LOG(m_debugLevel,"CrestDBIntf_project: call start_Subscribing: id %d\n",subscribe_Id);
                                        start_Subscribing(subscribe_Id,(char*)uriString.c_str(),RX_CONN_SUBSCRIBING);
                                    }
                                }
                            }

                            //another way to get the list of member
                            std::vector<std::string> list = jsonObj.getMemberNames();

                            if(list.empty())
                            {
                                CSIO_LOG(m_debugLevel,"CrestDBIntf_project: Member list is empty\n");
                            }
                            else
                            {
                                for(std::vector<std::string>::const_iterator it=list.begin(); it!=list.end(); ++it)
                                {
                                    const std::string& memstr = *it;
                                    CSIO_LOG(ABOVE_DEBUG_VERB(m_debugLevel),"CrestDBIntf_project: Member string is %s\n",memstr.c_str());
                                }
                            }
                        }
                        else
                        {
                            CSIO_LOG(eLogLevel_error,"CrestDBIntf_project: parsestring failed: %d\n", parsestring);
                        }

                        //delete when done
                        m_projEventQ->del_Q_buf(evntQ.buffPtr);
                    }

                    break;
                }
                case CREST_DB_EVENTS_ROUTING:
                {
                    if(evntQ.buffPtr && evntQ.buf_size)
                    {
                        CSIO_LOG(m_debugLevel,"CrestDBIntf_project: parsing rounting string.\n");
                        parsestring = jsonReader.parse((char*)evntQ.buffPtr, jsonObj);
                        if(parsestring)
                        {
                            CSIO_LOG(m_debugLevel,"CrestDBIntf_project: parsestring success %d\n", parsestring);
                            for(Json::Value::iterator it=jsonObj.begin(); it!=jsonObj.end(); ++it)
                            {
                                const Json::Value& uuidJObj = *it;

                                char* UniqueIdCStr = (char*)uuidJObj["UniqueId"].asString().c_str();
                                std::string UniqueIdString = uuidJObj["UniqueId"].asString();

                                if(UniqueIdCStr)
                                {
                                    CSIO_LOG(m_debugLevel,"CrestDBIntf_project: UniqueId is  :%s\n", UniqueIdCStr);
                                    CSIO_LOG(m_debugLevel,"CrestDBIntf_project: UniqueId UniqueIdString  :%s\n",
                                             UniqueIdString.c_str());

                                    int index;
                                    if(isThisUuidSubscribed(UniqueIdString,index))
                                    {
                                        CSIO_LOG(m_debugLevel,"CrestDBIntf_project: call start_Subscribing(streaming) index  :%d\n", index);
                                        csio_setplayStatus(true,index);
                                        start_Subscribing(index,NULL,RX_CONN_ROUTING_STREAMING);
                                    }
                                }
                                else
                                {
                                    CSIO_LOG(eLogLevel_error,"CrestDBIntf_project: UniqueId is NULL\n");
                                }
                            }

                        }
                        else
                        {
                            CSIO_LOG(eLogLevel_error,"CrestDBIntf_project: parsestring failed: %d\n", parsestring);
                        }

                        //delete when done
                        m_projEventQ->del_Q_buf(evntQ.buffPtr);

                        CSIO_LOG(m_debugLevel,"CrestDBIntf_project: CREST_DB_EVENTS_ROUTING is done\n");
                    }
                    break;
                }
                default:
                    break;
            }
        }
        //else loop back.


        if(m_forceThreadExit)
        {
            //TODO: exit all child thread and wait here
            break;
        }
    }

    m_destroyCresDB();

    removeAllTasks();

    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: m_projectID ID:%d exiting...\n",m_projectID);

    //thread exit here
    m_ThreadIsRunning = 0;

    return NULL;
}
void CCrestDBIntfProject::sendEvent(csioEventQueueStruct* pEvntQ)
{
    //TODO: need to work on buffer copy
    if(m_projEvent && m_projEventQ && pEvntQ)
    {
        csioEventQueueStruct evntQ;
        memset(&evntQ,0,sizeof(csioEventQueueStruct));
        evntQ.obj_id  = pEvntQ->obj_id;
        evntQ.event_type    = pEvntQ->event_type;
        evntQ.buf_size      = 0;
        evntQ.buffPtr       = NULL;
        evntQ.ext_obj       = pEvntQ->ext_obj;;
        evntQ.ext_obj2      = pEvntQ->ext_obj2;;

        void* bufP = pEvntQ->buffPtr;
        int dataSize = pEvntQ->buf_size;

        CSIO_LOG(eLogLevel_verbose, "CrestDBIntf_project: iId[%d],evnt[%d],dataSize[%d],bufP[0x%x]\n",\
        		pEvntQ->obj_id,pEvntQ->event_type,pEvntQ->buf_size,pEvntQ->buffPtr);

        if(bufP && dataSize)
        {
            switch (evntQ.event_type)
            {
                case CREST_DB_EVENTS_SUBSCRIB:
                case CREST_DB_EVENTS_ROUTING:
                {
                    char* tmp = (char*)createCharArray(dataSize + 1);
                    if(tmp)
                    {
                        //first copy configure structure
                        memcpy(tmp,(char*)bufP,dataSize);
                        tmp[dataSize] = 0;
                        evntQ.buffPtr = tmp;
                        evntQ.buf_size = dataSize;
                    }
                    else
                    {
                        CSIO_LOG(eLogLevel_warning, "CrestDBIntf_project: create buffer failed\n");
                    }

                    /*
                     CrestDBIntfEventConfig* evntConfPtr = (CrestDBIntfEventConfig*)pEvntQ->buffPtr;

                    char* tmp = (char*)createCharArray(dataSize + 1);
                    if(tmp)
                    {
                        //first copy configure structure
                        memcpy(tmp,(char*)bufP,dataSize);
                        tmp[dataSize] = 0;

}

                     */
                    break;
                }
            }
        }

        //passing a copy of the queue
        if(m_projEventQ->AddToBuffer(&evntQ))// returns true if overflow
        {
            CSIO_LOG(eLogLevel_warning, "CrestDBIntf_project: event queue overflow[%d], clean it now.\n",m_projEventQ->GetEvntQueueCount());
            //fixOverflow();
            CSIO_LOG(eLogLevel_warning, "CrestDBIntf_project: fix queue overflow is done[%d].\n",m_projEventQ->GetEvntQueueCount());
        }
        CSIO_LOG(eLogLevel_verbose, "CrestDBIntf_project: event added to the queue[0x%x].\n",evntQ.buffPtr);

        m_projEvent->signalEvent(evntQ.event_type);
        CSIO_LOG(eLogLevel_verbose, "CrestDBIntf_project: event[%d] sent.\n",evntQ.event_type);

        logEventPush(evntQ.event_type);
    }
    else
    {
        CSIO_LOG(eLogLevel_warning, "CrestDBIntf_project: m_projEvent or m_projEventQ is NULL\n");
    }
}

void CCrestDBIntfProject::removeAllTasks()
{
    if(m_CrestDBIntfTaskObjList)
    {
        for(int i = 0; i < MAX_TOTAL_TASKS; i++)
        {
            if(m_CrestDBIntfTaskObjList[i])
            {
                //tell thread to exit
                m_CrestDBIntfTaskObjList[i]->exitThread();

                //wait until thread exits
                CSIO_LOG(m_debugLevel, "CresRTSP_manager: call WaitForThreadToExit[0x%x]\n",m_CrestDBIntfTaskObjList[i]);
                m_CrestDBIntfTaskObjList[i]->WaitForThreadToExit();
                CSIO_LOG(m_debugLevel, "CresRTSP_manager: Wait is done\n");

                //delete the object, and set list to NULL
                delete m_CrestDBIntfTaskObjList[i];
                m_CrestDBIntfTaskObjList[i] = NULL;
            }
        }

        delete[] m_CrestDBIntfTaskObjList;
        m_CrestDBIntfTaskObjList = NULL;
    }
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: removeAllTasks() exit.\n");
}
void CCrestDBIntfProject::setDebugLevel(int level)
{
    m_debugLevel = level;
    if(m_CrestDBIntfTaskObjList)
    {
        for(int i = 0; i < MAX_TOTAL_TASKS; i++)
        {
            if(m_CrestDBIntfTaskObjList[i])
            {
                m_CrestDBIntfTaskObjList[i]->setDebugLevel(level);
            }
        }
    }
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: setDebugLevel() exit.\n");
}

//Note: returns 0 ~ MAX_XIO_SUBCRIPTION-1 if found
//      returns -1 if failed
int CCrestDBIntfProject::getEmptySlot()
{
    int ret = -1;
    for(int i = 0; i < MAX_XIO_SUBCRIPTION; i++)
    {
        if(m_subscrUuidArray[i].empty())
            ret = i;
    }

    return ret;
}

bool CCrestDBIntfProject::isThisUuidSubscribed(const std::string& uuid, int& index)
{
    int ret = false;
    for(int i = 0; i < MAX_XIO_SUBCRIPTION; i++)
    {
        std::string& subStr = m_subscrUuidArray[i];
        if(subStr.empty())
            continue;

        if(subStr.compare(uuid) == 0)//They compare equal
        {
            index = i;
            ret = true;
        }
    }

    return ret;
}

int CCrestDBIntfProject::storeUuidAndUri(const std::string& uuidStr,const std::string& uriStr)
{
    int id = -1;

    //looking for the same uuid
    for(int i = 0; i < MAX_XIO_SUBCRIPTION; i++)
    {
        std::string subStr = m_subscrUuidArray[i];
        if(subStr.compare(uuidStr) == 0)//They compare equal
        {
            id = i;
            CSIO_LOG(m_debugLevel, "CrestDBIntf_project: storeUuidAndUri() id[%d] had the same uuid\n",i);

            storeNewUri(i,uriStr);

            return id;
        }
    }
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: storeUuidAndUri() id: %d\n",id);

    //uuid does not exist, looking for empty slot
    for(int i = 0; i < MAX_XIO_SUBCRIPTION; i++)
    {
        std::string subStr = m_subscrUuidArray[i];
        if(subStr.empty())
        {
            id = i;
            CSIO_LOG(m_debugLevel, "CrestDBIntf_project: id: %d is empty\n",i);

            storeNewUuid(i,uuidStr);
            storeNewUri(i,uriStr);

            return id;
        }
    }
    return id;
}
bool CCrestDBIntfProject::isUriChanged(int index,const std::string& uriStr)
{
    if(uriStr.compare(m_subscrUriArray[index]) != 0)//compare NOT equal
    {
        CSIO_LOG(m_debugLevel, "CrestDBIntf_project: index: %d uri changed\n",index);
        return true;
    }

    return false;
}
int CCrestDBIntfProject::storeNewUuid(int index,const std::string& uuidStr)
{
    m_subscrUuidArray[index].clear();
    m_subscrUuidArray[index] = uuidStr;
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: storeNewUuid() id: %d\n",index);
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: m_subscrUuidArray: %s\n",m_subscrUuidArray[index].c_str());
}
int CCrestDBIntfProject::storeNewUri(int index,const std::string& uriStr)
{
    m_subscrUriArray[index].clear();
    m_subscrUriArray[index] = uriStr;
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: storeNewUri() id: %d\n",index);
    CSIO_LOG(m_debugLevel, "CrestDBIntf_project: m_subscrUriArray: %s\n",m_subscrUriArray[index].c_str());
}

void CCrestDBIntfProject::deleteThisItem(int index)
{
    if(IsValidCrestDBIntfId(index))
    {
        m_subscrUuidArray[index].clear();
        m_subscrUriArray[index].clear();
    }
}

