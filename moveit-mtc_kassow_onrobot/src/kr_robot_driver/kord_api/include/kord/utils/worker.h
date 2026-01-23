/*********************************************************************
* Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Kassow Robots
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Kassow Robots nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#ifndef KR2_KORD_API_WORKER_H_
#define KR2_KORD_API_WORKER_H_

#include <atomic>
#include <thread>

class Worker {

public:
    virtual ~Worker() = default;

    /// @brief Start execution of defined job
    void start()
    {
        if (nullptr == t) {
            t = new std::thread(Worker::exec, this);
            // mark the thread as working
            this->running();
        }
        else // not new, but is it finished?
        {
            if (this->is_done()) {
                t = new std::thread(Worker::exec, this);
                // mark the thread as working
                this->running();
            }
        }
    }

    /// @brief Wait for the end of the executing
    void join()
    {
        if (nullptr != t) {
            t->join();
            delete t;
            t = nullptr;
        }
    }

    /// @brief Wait for the end of the executing
    bool joinable()
    {
        if (nullptr != t) {
            return t->joinable();
        }
        return false;
    }

    /// @brief  Is the task finished
    /// @return bool
    bool is_done() { return this->done; }

protected:
    /// @brief The method to overload in order to provide your functionality
    virtual void run() = 0;

private:
    // pointer to the thread
    std::thread *t = nullptr;
    std::atomic<bool> done = true;

    // static function which points back to the instance
    static void exec(Worker *Worker) { Worker->run(); }

    void running() { this->done = false; }

protected:
    void finished() { this->done = true; }
};

#endif // KR2_KORD_API_WORKER_H_
