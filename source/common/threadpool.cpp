/*****************************************************************************
 * x265: singleton thread pool and interface classes
 *****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/

#include "threadpool.h"
#include "threading.h"
#include <assert.h>
#include <string.h>
#include <new>

#if MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

namespace x265 {
// x265 private namespace

class ThreadPoolImpl;
static int get_cpu_count();

class PoolThread : public Thread
{
private:

    ThreadPoolImpl &m_pool;

    PoolThread& operator =(const PoolThread&);

    bool           m_dirty;

    bool           m_idle;

    bool           m_exited;

public:

    PoolThread(ThreadPoolImpl& pool) : m_pool(pool), m_dirty(false), m_idle(false), m_exited(false) {}

    //< query if thread is still potentially walking provider list
    bool isDirty() const  { return !m_idle && m_dirty; }

    //< set m_dirty if the thread might be walking provider list
    void markDirty()      { m_dirty = !m_idle; }

    bool isExited() const { return m_exited; }

    virtual ~PoolThread() {}

    void threadMain();

    static volatile int s_sleepCount;
    static Event s_wakeEvent;
};

volatile int PoolThread::s_sleepCount = 0;

Event PoolThread::s_wakeEvent;

class ThreadPoolImpl : public ThreadPool
{
private:

    bool         m_ok;
    int          m_referenceCount;
    int          m_numThreads;
    PoolThread  *m_threads;

    /* Lock for write access to the provider lists.  Threads are
     * always allowed to read m_firstProvider and follow the
     * linked list.  Providers must zero their m_nextProvider
     * pointers before removing themselves from this list */
    Lock         m_writeLock;

public:

    static ThreadPoolImpl *instance;

    JobProvider *m_firstProvider;
    JobProvider *m_lastProvider;

public:

    ThreadPoolImpl(int numthreads);

    virtual ~ThreadPoolImpl();

    ThreadPoolImpl *AddReference()
    {
        m_referenceCount++;

        return this;
    }

    int getThreadCount() const { return m_numThreads; }

    void release();

    void Stop();

    bool IsValid() const
    {
        return m_ok;
    }

    void enqueueJobProvider(JobProvider &);

    void dequeueJobProvider(JobProvider &);

    void FlushProviderList();

    void pokeIdleThread();
};

void PoolThread::threadMain()
{
#if _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
    __attribute__((unused)) int val = nice(10);
#endif

    while (m_pool.IsValid())
    {
        /* Walk list of job providers, looking for work */
        JobProvider *cur = m_pool.m_firstProvider;
        while (cur)
        {
            // FindJob() may perform actual work and return true.  If
            // it does we restart the job search
            if (cur->findJob() == true)
                break;

            cur = cur->m_nextProvider;
        }

        m_dirty = false;
        if (cur == NULL)
        {
            m_idle = true;
            ATOMIC_INC(&s_sleepCount);
            s_wakeEvent.wait();
            ATOMIC_DEC(&s_sleepCount);
            m_idle = false;
        }
    }

    m_exited = true;
}

void ThreadPoolImpl::pokeIdleThread()
{
    PoolThread::s_wakeEvent.trigger();
}

ThreadPoolImpl *ThreadPoolImpl::instance;

/* static */
ThreadPool *ThreadPool::allocThreadPool(int numthreads)
{
    if (ThreadPoolImpl::instance)
        return ThreadPoolImpl::instance->AddReference();

    ThreadPoolImpl::instance = new ThreadPoolImpl(numthreads);
    return ThreadPoolImpl::instance;
}

ThreadPool *ThreadPool::getThreadPool()
{
    assert(ThreadPoolImpl::instance);
    return ThreadPoolImpl::instance;
}

void ThreadPoolImpl::release()
{
    if (--m_referenceCount == 0)
    {
        assert(this == ThreadPoolImpl::instance);
        ThreadPoolImpl::instance = NULL;
        this->Stop();
        delete this;
    }
}

ThreadPoolImpl::ThreadPoolImpl(int numThreads)
    : m_ok(false)
    , m_referenceCount(1)
    , m_firstProvider(NULL)
    , m_lastProvider(NULL)
{
    if (numThreads == 0)
        numThreads = get_cpu_count();

    char *buffer = new char[sizeof(PoolThread) * numThreads];
    m_threads = reinterpret_cast<PoolThread*>(buffer);
    m_numThreads = numThreads;

    if (m_threads)
    {
        m_ok = true;
        for (int i = 0; i < numThreads; i++)
        {
            new (buffer)PoolThread(*this);
            buffer += sizeof(PoolThread);
            m_ok = m_ok && m_threads[i].start();
        }

        // Wait for threads to spin up and idle
        while (PoolThread::s_sleepCount < m_numThreads)
        {
            GIVE_UP_TIME();
        }
    }
}

void ThreadPoolImpl::Stop()
{
    if (m_ok)
    {
        // wait for all threads to idle
        while (PoolThread::s_sleepCount < m_numThreads)
        {
            GIVE_UP_TIME();
        }

        // set invalid flag, then wake them up so they exit their main func
        m_ok = false;
        int exited_count;
        do
        {
            pokeIdleThread();
            GIVE_UP_TIME();
            exited_count = 0;
            for (int i = 0; i < m_numThreads; i++)
            {
                exited_count += m_threads[i].isExited() ? 1 : 0;
            }
        }
        while (exited_count < m_numThreads);

        // join each thread to cleanup resources
        for (int i = 0; i < m_numThreads; i++)
        {
            m_threads[i].stop();
        }
    }
}

ThreadPoolImpl::~ThreadPoolImpl()
{
    if (m_threads)
    {
        // cleanup thread handles
        for (int i = 0; i < m_numThreads; i++)
        {
            m_threads[i].~PoolThread();
        }

        delete[] reinterpret_cast<char*>(m_threads);
    }
}

void ThreadPoolImpl::enqueueJobProvider(JobProvider &p)
{
    // only one list writer at a time
    ScopedLock l(m_writeLock);

    p.m_nextProvider = NULL;
    p.m_prevProvider = m_lastProvider;
    m_lastProvider = &p;

    if (p.m_prevProvider)
        p.m_prevProvider->m_nextProvider = &p;
    else
        m_firstProvider = &p;
}

void ThreadPoolImpl::dequeueJobProvider(JobProvider &p)
{
    // only one list writer at a time
    ScopedLock l(m_writeLock);

    // update pool entry pointers first
    if (m_firstProvider == &p)
        m_firstProvider = p.m_nextProvider;

    if (m_lastProvider == &p)
        m_lastProvider = p.m_prevProvider;

    // extract self from doubly linked lists
    if (p.m_nextProvider)
        p.m_nextProvider->m_prevProvider = p.m_prevProvider;

    if (p.m_prevProvider)
        p.m_prevProvider->m_nextProvider = p.m_nextProvider;

    p.m_nextProvider = NULL;
    p.m_prevProvider = NULL;
}

/* Ensure all threads are either idle, or have made a full
 * pass through the provider list, ensuring dequeued providers
 * are safe for deletion. */
void ThreadPoolImpl::FlushProviderList()
{
    for (int i = 0; i < m_numThreads; i++)
    {
        m_threads[i].markDirty();
    }

    int i;
    do
    {
        for (i = 0; i < m_numThreads; i++)
        {
            if (m_threads[i].isDirty())
            {
                GIVE_UP_TIME();
                break;
            }
        }
    }
    while (i < m_numThreads);
}

void JobProvider::flush()
{
    if (m_nextProvider || m_prevProvider)
        dequeue();
    dynamic_cast<ThreadPoolImpl*>(m_pool)->FlushProviderList();
}

void JobProvider::enqueue()
{
    // Add this provider to the end of the thread pool's job provider list
    assert(!m_nextProvider && !m_prevProvider && m_pool);
    m_pool->enqueueJobProvider(*this);
    m_pool->pokeIdleThread();
}

void JobProvider::dequeue()
{
    // Remove this provider from the thread pool's job provider list
    m_pool->dequeueJobProvider(*this);
    // Ensure no jobs were missed while the provider was being removed
    m_pool->pokeIdleThread();
}

static int get_cpu_count()
{
#if WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif __unix__
    return sysconf(_SC_NPROCESSORS_ONLN);
#elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;

    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if (count < 1)
    {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        if (count < 1)
            count = 1;
    }

    return count;
#else // if WIN32
    return 2; // default to 2 threads, everywhere else
#endif // if WIN32
}
} // end namespace x265
