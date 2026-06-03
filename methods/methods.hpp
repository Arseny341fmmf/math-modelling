#ifndef METHODS_METHODS_HPP_
#define METHODS_METHODS_HPP_

#include <nlohmann/json.hpp>

namespace mm {
class TasksQueue;

/**
 * @brief Обработчик POST /ThreeBodySolver.
 */
int ThreeBodySolverMethod(const nlohmann::json& input,
                          nlohmann::json* output,
                          TasksQueue* tasksQueue);

} // namespace mm

#endif // METHODS_METHODS_HPP_