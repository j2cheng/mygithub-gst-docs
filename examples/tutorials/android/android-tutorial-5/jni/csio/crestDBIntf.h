#ifndef _CRESTDBINTF_H_
#define _CRESTDBINTF_H_

#include "csioCommBase.h"

#define MAX_STR_LEN    256

#define MAX_TOTAL_TASKS   1

#define MAX_XIO_SUBCRIPTION   64

#define IsValidCrestDBIntfId(a)  ( (a >= 0) && (a < MAX_XIO_SUBCRIPTION) )

typedef struct _CrestDBIntfEventConfig
{
    int type;
    char *pUuidString;                 // Null terminated uuid string
    char *pURLString;                  // Null terminated URL string

}CrestDBIntfEventConfig;

class CCrestDBIntfManager;
class CrestDBIntfEventRingBuffer;
class csioEventQueueListBase;

class CCrestDBIntfProject : public csioThreadBaseClass
{
public:

    CCrestDBIntfProject(int iId);
    ~CCrestDBIntfProject();

    void    DumpClassPara(int);
    virtual void* ThreadEntry();

    void sendEvent(csioEventQueueStruct* pEvntQ);

    void removeAllTasks();

    void lockProject(){if(mLock) mLock->lock();}
    void unlockProject(){if(mLock) mLock->unlock();}

    //void lockCresstoreDb(){if(m_csDBLock) m_csDBLock->lock();}
    //void unlockCresstoreDb(){if(m_csDBLock) m_csDBLock->unlock();}
    //void* m_takeCS_and_lock()
    //{
    //    lockCresstoreDb();
    //    return m_cresstoreDBPtr;
    //}

    virtual void setDebugLevel(int level);

    csioEventQueueListBase *m_projEvent;
    // CrestDBIntfEventRingBuffer *m_projEventQ;

    CCrestDBIntfManager** m_CrestDBIntfTaskObjList;
    
    std::string m_subscrUuidArray[MAX_XIO_SUBCRIPTION];
    std::string m_subscrUriArray[MAX_XIO_SUBCRIPTION];
private:
    int  m_projectID;
    Mutex* mLock;
   // Mutex* m_csDBLock;

    void* createCharArray(int size) { return new char [size]; }
    void* deleteCharArray(void* buf)
    {
        if(buf)
        {
            char* tmp = (char*)buf;
            delete [] tmp;
        }
    }

    static void cresstoreCallback(char * key, char * json, void * userptr);

    int  getEmptySlot();
    bool isThisUuidSubscribed(const std::string& uuid, int& index);
    int  storeUuidAndUri(const std::string& uuidStr,const std::string& uriStr);
    int  storeNewUri(int index,const std::string& uriStr);
    int  storeNewUuid(int index,const std::string& uuidStr);
    bool isUriChanged(int index,const std::string& uriStr);
    void deleteThisItem(int index);

    //void * m_cresstoreDBPtr;

    //void m_destroyCresDB()
    //{
    //    lockCresstoreDb();
    //    if(m_cresstoreDBPtr)
    //    {
    //        cresstore_destroy(m_cresstoreDBPtr);
    //        m_cresstoreDBPtr = NULL;
    //    }
    //    unlockCresstoreDb();
    //}

};



//functions will be called from external
#ifdef __cplusplus
extern "C"
{
#endif

//called by CSIO root


//functions defined in CSIO root

//all project functions
extern void CrestDBIntfProjectInit();
extern void CrestDBIntfProjectDeInit();
extern void CrestDBIntf_ProjectDumpClassPara(int level);
//extern void CrestDBIntf_add_rtspUrlToDB(const char*);

#ifdef __cplusplus
}
#endif

#endif /* _CRESTDBINTF_H_ */
