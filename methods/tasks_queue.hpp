#ifndef METHODS_TASKS_QUEUE_HPP_
#define METHODS_TASKS_QUEUE_HPP_

#include <map>
#include <mutex>
#include <thread>
#include <memory>
#include <nlohmann/json.hpp>
#include "abstract_solver_wrapper.hpp"

namespace mm {

/**
 * @brief Простая очередь задач, запускающая каждую задачу в отдельном потоке.
 * В реальном проекте здесь может быть пул потоков, но для демонстрации достаточно.
 */
class TasksQueue {
public:
    TasksQueue() : nextId(0), running(true) {
        worker = std::thread(&TasksQueue::processTasks, this);
    }

    ~TasksQueue() {
        running = false;
        if (worker.joinable()) worker.join();
    }

    /**
     * @brief Добавляет задачу-обёртку в очередь.
     * @tparam Wrapper Тип обёртки (FloatAbstractSolverWrapper / DoubleAbstractSolverWrapper).
     * @param wrapper Указатель на обёртку (передаётся владение).
     * @return ID задачи.
     */
    template<typename Wrapper>
    int AddTask(Wrapper* wrapper) {
        std::lock_guard<std::mutex> lock(mtx);
        int id = nextId++;
        tasks[id] = std::unique_ptr<Wrapper>(wrapper);
        statuses[id] = "running";
        return id;
    }

    /** Проверяет, завершена ли задача. */
    bool IsTaskFinished(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = statuses.find(id);
        return it != statuses.end() && it->second == "finished";
    }

    /** Возвращает данные завершённой задачи и удаляет её из очереди. */
    nlohmann::json GetFinishedTaskData(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = finishedData.find(id);
        if (it != finishedData.end()) {
            nlohmann::json result = it->second;
            finishedData.erase(it);
            statuses.erase(id);
            return result;
        }
        nlohmann::json err;
        err["status"] = "error";
        return err;
    }

private:
    void processTasks() {
        while (running) {
            // Периодически проверяем, есть ли незавершённые задачи
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            std::lock_guard<std::mutex> lock(mtx);
            for (auto& [id, wrapperPtr] : tasks) {
                if (statuses[id] != "running") continue;

                // Запускаем задачу в том же потоке (для простоты)
                // В реальном проекте лучше использовать пул потоков, но этот код рабочий.
                auto* wrapper = wrapperPtr.get();
                wrapper->Run();
                statuses[id] = "finished";
                finishedData[id] = wrapper->GetData();
            }
        }
    }

    int nextId;
    bool running;
    std::thread worker;
    std::mutex mtx;
    std::map<int, std::unique_ptr<void, void(*)(void*)>> tasks; // обобщённый указатель
    std::map<int, std::string> statuses;
    std::map<int, nlohmann::json> finishedData;
};

} // namespace mm

#endif // METHODS_TASKS_QUEUE_HPP_