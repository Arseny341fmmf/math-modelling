#ifndef INCLUDE_THREE_BODY_SOLVER_HPP_
#define INCLUDE_THREE_BODY_SOLVER_HPP_

#include <array>
#include <vector>
#include <mutex>
#include <future>
#include <nlohmann/json.hpp>
#include "abstract_solver.hpp"

namespace mm {

/**
 * @brief Решалка задачи трёх тел с кулоновским взаимодействием.
 * @tparam T Тип данных (float/double).
 */
template<typename T>
class ThreeBodySolver : public AbstractSolver<T> {
public:
    /**
     * @brief Конструктор.
     * @param tau Шаг по времени.
     * @param finishTime Конечное время.
     * @param exportPeriod Период сохранения.
     * @param masses Массы частиц [m1,m2,m3].
     * @param charges Заряды частиц [q1,q2,q3].
     * @param c Константа взаимодействия (кулоновская).
     * @param initialPositions Начальные позиции [ [x1,y1,z1], ... ].
     * @param initialVelocities Начальные скорости [ [vx1,vy1,vz1], ... ].
     */
    ThreeBodySolver(T tau, T finishTime, T exportPeriod,
                    const std::array<T, 3>& masses,
                    const std::array<T, 3>& charges,
                    T c,
                    const std::vector<std::array<T, 3>>& initialPositions,
                    const std::vector<std::array<T, 3>>& initialVelocities);

    bool MakeStep() override;
    void ExportData(nlohmann::json* output) override;

private:
    std::array<T, 3> masses;      ///< Массы
    std::array<T, 3> charges;     ///< Заряды
    T c;                          ///< Константа взаимодействия

    std::vector<std::array<T, 3>> positions;  ///< Текущие позиции
    std::vector<std::array<T, 3>> velocities; ///< Текущие скорости

    // Массивы для коэффициентов Рунге-Кутты
    std::vector<std::array<T, 3>> k1_pos, k1_vel;
    std::vector<std::array<T, 3>> k2_pos, k2_vel;
    std::vector<std::array<T, 3>> k3_pos, k3_vel;
    std::vector<std::array<T, 3>> k4_pos, k4_vel;

    std::mutex mtx;   ///< Мьютекс для synchronize()

    /// Вычисляет ускорения всех частиц по заданным позициям.
    std::vector<std::array<T, 3>> computeAccelerations(
        const std::vector<std::array<T, 3>>& pos) const;

    /// Синхронизирует потоки (захват и освобождение мьютекса).
    void synchronize();
};

} // namespace mm

#include "three_body_solver_impl.hpp"

#endif // INCLUDE_THREE_BODY_SOLVER_HPP_