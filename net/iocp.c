
#include <Windows.h>


typedef int(*poller_callback_t)(struct ho_poller *);
struct ho_poller
{
	int type;
	char name[16];
	poller_callback_t new;
	poller_callback_t init;
	poller_callback_t post;
	poller_callback_t exit;
	void * data;
};

static int iocp_init()
{
	HANDLE h = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	int m_nProcessors = si.dwNumberOfProcessors;

	// 根据CPU数量，建立*2的线程
	int m_nThreads = 2 * m_nProcessors;
	HANDLE* m_phWorkerThreads = new HANDLE[m_nThreads];

	for (int i = 0; i < m_nThreads; i++)
	{
		m_phWorkerThreads[i] = ::CreateThread(0, 0, _WorkerThread, …);
	}

}


static int ipcp_poll()
{

	void *lpContext = NULL;
	OVERLAPPED        *pOverlapped = NULL;
	DWORD            dwBytesTransfered = 0;

	BOOL bReturn = GetQueuedCompletionStatus(
		pIOCPModel->m_hIOCompletionPort,
		&dwBytesTransfered,
		(LPDWORD)&lpContext,
		&pOverlapped,
		INFINITE);


}

