/**
 * @file tasks_queue.hpp
 * @author Mikhail Lozhnikov
 *
 * Класс очереди задач.
 */

#ifndef METHODS_TASKS_QUEUE_HPP_
#define METHODS_TASKS_QUEUE_HPP_

#include <map>
#include <mutex>
#include <queue>
#include <utility>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "abstract_solver_wrapper.hpp"
#include <abstract_solver_wrapper.hpp>

namespace mm {

/**
 * @brief Простая очередь задач, запускающая каждую задачу в отдельном потоке.
 * В реальном проекте здесь может быть пул потоков, но для демонстрации достаточно.
 * @brief Класс очереди задач.
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
 private:
  //! Очередь задач.
  std::queue<std::pair<int, AbstractSolverWrapper*>> tasks;
  //! Данные завершенныхзадач.
  std::unordered_map<int, nlohmann::json> finishedTasksData;
  //! Идентификатор последней задачи.
  int lastTaskId;
  //! Индикатор завершения очереди обработки задач.
  bool finished;
  //! Мьютекс для внутренних структур.
  std::mutex m;
  //! Условная переменная для начала обработки новой задачи.
  std::condition_variable condvar;
  //! Описатель потока - обработчика задач.
  std::thread queueThread;

    /** Проверяет, завершена ли задача. */
    bool IsTaskFinished(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = statuses.find(id);
        return it != statuses.end() && it->second == "finished";
    }
 public:
  /**
   * @brief Конструктор очереди задач.
   *
   * Запускает поток-обработчик очереди задач.
   */
  TasksQueue();

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
  /**
   * @brief Деструктор.
   *
   * Останавливает поток-обработчик очереди задач.
   */
  ~TasksQueue();

private:
    void processTasks() {
        while (running) {
            // Периодически проверяем, есть ли незавершённые задачи
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
  /**
   * @brief Добавить решалку в очередь задач.
   * @param task Решалка, добавляемая в очередь.
   * @return Идентификатор добавленной задачи.
   *
   * Функция добавляет задачу в очередь задач и присваивает ей уникальный
   * идентификатор. 
   */
  int AddTask(AbstractSolverWrapper* task);

            std::lock_guard<std::mutex> lock(mtx);
            for (auto& [id, wrapperPtr] : tasks) {
                if (statuses[id] != "running") continue;
  /**
   * @brief Проверить, завершена ли задача.
   * @param id Идентификатор задачи.
   * @return Функция возвращает true, если задача завершена, и false в противном
   * случае.
   */
  bool IsTaskFinished(int id);

                // Запускаем задачу в том же потоке (для простоты)
                // В реальном проекте лучше использовать пул потоков, но этот код рабочий.
                auto* wrapper = wrapperPtr.get();
                wrapper->Run();
                statuses[id] = "finished";
                finishedData[id] = wrapper->GetData();
            }
        }
    }
  /**
   * @brief Получить данные расчетов завершенной задачи.
   * @param id Идентификатор задачи.
   * @return Данный расчетов задачи в формате JSON.
   *
   * Функция возвращает данные расчетов задачи с указанным идентификатором.
   * Если данные задачи не найдены, то функция генерирует исключение типа
   * std::runtime_error. После выполнения функции информация об указанной
   * задаче, а также всё её данные удаляются. Задача также небудет числиться
   * в списке завершенных задач.
   */
  nlohmann::json GetFinishedTaskData(int id);

    int nextId;
    bool running;
    std::thread worker;
    std::mutex mtx;
    std::map<int, std::unique_ptr<void, void(*)(void*)>> tasks; // обобщённый указатель
    std::map<int, std::string> statuses;
    std::map<int, nlohmann::json> finishedData;
 private:
  /**
   * @brief Поток, в котором выполняется очередь задач.
   */
  void ThreadFunction();
};

} // namespace mm
}  // namespace mm

#endif // METHODS_TASKS_QUEUE_HPP_
