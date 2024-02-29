//
// Created by czr on 24-2-29.
//

#include <stdexcept>
#include "Mutex.h"

namespace Server {
    Semaphore::Semaphore(uint32_t count) {
        if (sem_init(&m_semaphore, 0, count)) {
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore() {
        sem_close(&m_semaphore);
    }

    void Semaphore::wait() {
        //sem_wait，会将m_semaphore--，当m_semaphore为 0 时，就会等待
        if (sem_wait(&m_semaphore) < 0) {
            throw std::logic_error("sem wait error");
        }
    }

    void Semaphore::notify() {
        //sem_post 会将m_semaphore++
        if (sem_post(&m_semaphore) < 0) {
            throw std::logic_error("sem post error");
        }
    }
}