#ifndef INCLUDE_ABSTRACT_SOLVER_WRAPPER_HPP_
#define INCLUDE_ABSTRACT_SOLVER_WRAPPER_HPP_

#include <nlohmann/json.hpp>
#include "abstract_solver.hpp"

namespace mm {

/**
 * @brief Обёртка над AbstractSolver для хранения в очереди задач.
 * @tparam T Тип данных (float/double).
 */
template<typename T>
class AbstractSolverWrapper {
public:
    explicit AbstractSolverWrapper(AbstractSolver<T>* solver)
        : solver(solver) {}

    virtual ~AbstractSolverWrapper() {
        delete solver;
    }

   
    void Run() {
        solver->Run();
    }

    /** Возвращает все экспортированные данные. */
    nlohmann::json GetData() {
        nlohmann::json dataArray = nlohmann::json::array();
        nlohmann::json frame;
        frame["time"] = solver->GetCurrentTime();
        solver->ExportData(&frame);
        dataArray.push_back(frame);
        nlohmann::json result;
        result["status"] = "ok";
        result["data"] = dataArray;
        return result;
    }

private:
    AbstractSolver<T>* solver;
};

using FloatAbstractSolverWrapper = AbstractSolverWrapper<float>;
using DoubleAbstractSolverWrapper = AbstractSolverWrapper<double>;

} // namespace mm

#endif // INCLUDE_ABSTRACT_SOLVER_WRAPPER_HPP_