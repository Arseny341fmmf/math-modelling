/**
 * @file methods/tasks_queue.hpp
 * @author Mikhail Lozhnikov
 *
 * Очередь задач для серверной части.
 */

#ifndef METHODS_TASKS_QUEUE_HPP_
#define METHODS_TASKS_QUEUE_HPP_

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <nlohmann/json.hpp>
#include "abstract_solver_wrapper.hpp"

namespace mm {

class TasksQueue {
 public:
  TasksQueue();
  ~TasksQueue();

  template<typename Wrapper>
  int AddTask(Wrapper* wrapper) {
    std::lock_guard<std::mutex> lock(mutex_);

    int id = nextId_++;

    tasks_[id] = std::unique_ptr<AbstractSolverWrapperBase>(wrapper);
    statuses_[id] = "running";

    condition_.notify_one();

    return id;
  }

  bool IsTaskFinished(int id);
  nlohmann::json GetFinishedTaskData(int id);

 private:
  void ProcessTasks();

  int nextId_;
  std::atomic<bool> stop_;
  std::thread worker_;

  std::mutex mutex_;
  std::condition_variable condition_;

  std::map<int, std::unique_ptr<AbstractSolverWrapperBase>> tasks_;
  std::map<int, std::string> statuses_;
  std::map<int, nlohmann::json> finishedData_;
};

}  // namespace mm

#endif  // METHODS_TASKS_QUEUE_HPP_


#endif  // METHODS_TASKS_QUEUE_HPP_
