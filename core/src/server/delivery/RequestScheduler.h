// Copyright (C) 2019-2020 Zilliz. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied. See the License for the specific language governing permissions and limitations under the License.

#pragma once

#include "server/delivery/RequestQueue.h"
#include "utils/Status.h"

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace milvus {
namespace server {

using ThreadPtr = std::shared_ptr<std::thread>;

class RequestScheduler {
 public:
    static RequestScheduler&
    GetInstance() {
        static RequestScheduler scheduler;
        return scheduler;
    }

    void
    Start();

    void
    Stop();

    Status
    ExecuteRequest(const BaseRequestPtr& request_ptr);

    static void
    ExecRequest(BaseRequestPtr& request_ptr);

 protected:
    RequestScheduler();

    virtual ~RequestScheduler();

    void
    TakeToExecute(RequestQueuePtr request_queue);

    Status
    PutToQueue(const BaseRequestPtr& request_ptr);

 private:
    mutable std::mutex queue_mtx_;

    std::map<std::string, RequestQueuePtr> request_groups_;

    std::vector<ThreadPtr> execute_threads_;

    bool stopped_;
};

}  // namespace server
}  // namespace milvus
