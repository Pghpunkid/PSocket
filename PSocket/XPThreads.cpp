/*******************************************************************************
** FILE:			XPThreads.cpp

** DESCRIPTION: 

** AUTHOR:			Amit Gupta

** CREATED:		
********************************************************************************/

#include "XPThreads.h"


/******************************************************************************
** FUNCTION:	Constructor 
*******************************************************************************/
XPThreads::XPThreads(ThreadProc ptrCallback, long nTimeout)
: m_Callback(ptrCallback)
, m_Threadid(0)
#ifdef WIN32
, m_nTimeout(nTimeout)
, m_ptrThread(NULL)
#endif
{
}

/******************************************************************************
** FUNCTION:	Destructor 
*******************************************************************************/
XPThreads::~XPThreads() throw()
{
	Stop();	
}


/******************************************************************************
** FUNCTION:	Stop 
*******************************************************************************/
void XPThreads::Stop()
{
#ifdef WIN32
	//waiting for the thread to terminate
	if (m_ptrThread) 
	{
		if(WAIT_TIMEOUT == ::WaitForSingleObject (m_ptrThread, m_nTimeout))
			::TerminateThread (m_ptrThread, 1);

		::CloseHandle (m_ptrThread);
	}
#endif
}

/******************************************************************************
** FUNCTION:	Run 
*******************************************************************************/
bool XPThreads::Run()
{
	bool bRetVal = false;
	
	do
	{		
		if(IsThreadCreated())
			break;

#ifdef WIN32

		m_ptrThread = ::CreateThread (NULL, 0, 
			(unsigned long (__stdcall *)(void *))m_Callback, 
			NULL, 0, &m_Threadid);
		if(NULL == m_ptrThread)
			break;
	
#else

	    pthread_attr_t attr;
	
		/* Initialize and set thread detached attribute */
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);				
	
		if(!pthread_create(&m_Threadid, &attr, m_Callback, (void *)this))
			break;

	   /* Free attribute and wait  */
		pthread_attr_destroy(&attr);

#endif

		bRetVal = true;
	} while(false);
	
	return bRetVal;
}

/******************************************************************************
** FUNCTION:	SetThreadProc
*******************************************************************************/
void XPThreads::SetThreadProc(ThreadProc ptrCallback)
{
	m_Callback = ptrCallback;
}

/******************************************************************************
** FUNCTION:	IsThreadCreated
*******************************************************************************/
bool XPThreads::IsThreadCreated()
{
	return m_Threadid ? true : false;
}


void thr_sleep( unsigned int ms )
{
#ifdef WIN32
	Sleep( ms );
#else
    usleep( ms * 1000 );
#endif
}
