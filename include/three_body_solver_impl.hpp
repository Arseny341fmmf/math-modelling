#ifndef INCLUDE_THREE_BODY_SOLVER_IMPL_HPP_
#define INCLUDE_THREE_BODY_SOLVER_IMPL_HPP_

#include <cmath>
#include <future>
#include "three_body_solver.hpp"

namespace mm {

template<typename T>
ThreeBodySolver<T>::ThreeBodySolver(
    T tau, T finishTime, T exportPeriod,
    const std::array<T, 3>& masses,
    const std::array<T, 3>& charges,
    T c,
    const std::vector<std::array<T, 3>>& initialPositions,
    const std::vector<std::array<T, 3>>& initialVelocities)
    : AbstractSolver<T>(tau, finishTime, exportPeriod),
      masses(masses), charges(charges), c(c),
      positions(initialPositions), velocities(initialVelocities),
      k1_pos(3), k1_vel(3), k2_pos(3), k2_vel(3),
      k3_pos(3), k3_vel(3), k4_pos(3), k4_vel(3) {}

template<typename T>
std::vector<std::array<T, 3>>
ThreeBodySolver<T>::computeAccelerations(
    const std::vector<std::array<T, 3>>& pos) const {
    std::vector<std::array<T, 3>> acc(3, {0,0,0});
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i == j) continue;
            T dx = pos[j][0] - pos[i][0];
            T dy = pos[j][1] - pos[i][1];
            T dz = pos[j][2] - pos[i][2];
            T r2 = dx*dx + dy*dy + dz*dz;
            T r = std::sqrt(r2);
            // Сила Кулона: F = c * q_i*q_j / r^2, ускорение = F/m_i
            T force_magnitude = c * charges[i] * charges[j] / r2;
            T ax = force_magnitude / masses[i] * (dx / r);
            T ay = force_magnitude / masses[i] * (dy / r);
            T az = force_magnitude / masses[i] * (dz / r);
            acc[i][0] += ax;
            acc[i][1] += ay;
            acc[i][2] += az;
        }
    }
    return acc;
}

template<typename T>
void ThreeBodySolver<T>::synchronize() {
    // Захват и немедленное освобождение мьютекса заставляет потоки
    // выстроиться в очередь и гарантирует, что все достигли этой точки.
    std::lock_guard<std::mutex> lock(mtx);
}

template<typename T>
bool ThreeBodySolver<T>::MakeStep() {
    T tau = this->tau;

    // ---------- k1 ----------
    synchronize();
    {
        std::vector<std::future<void>> futures;
        for (int p = 0; p < 3; ++p) {
            futures.push_back(std::async(std::launch::async, [this, p] {
                k1_pos[p] = velocities[p];   // производная позиции = скорость
                auto acc = computeAccelerations(positions);
                k1_vel[p] = acc[p];
            }));
        }
        for (auto& f : futures) f.get(); // ждём завершения всех потоков
    }

    // ---------- k2 ----------
    std::vector<std::array<T,3>> pos_mid(3), vel_mid(3);
    for (int p = 0; p < 3; ++p) {
        for (int d = 0; d < 3; ++d) {
            pos_mid[p][d] = positions[p][d] + (tau/2) * k1_pos[p][d];
            vel_mid[p][d] = velocities[p][d] + (tau/2) * k1_vel[p][d];
        }
    }
    synchronize();
    {
        std::vector<std::future<void>> futures;
        for (int p = 0; p < 3; ++p) {
            futures.push_back(std::async(std::launch::async, [this, p, &pos_mid] {
                k2_pos[p] = vel_mid[p];
                auto acc = computeAccelerations(pos_mid);
                k2_vel[p] = acc[p];
            }));
        }
        for (auto& f : futures) f.get();
    }

    // ---------- k3 ----------
    for (int p = 0; p < 3; ++p) {
        for (int d = 0; d < 3; ++d) {
            pos_mid[p][d] = positions[p][d] + (tau/2) * k2_pos[p][d];
            vel_mid[p][d] = velocities[p][d] + (tau/2) * k2_vel[p][d];
        }
    }
    synchronize();
    {
        std::vector<std::future<void>> futures;
        for (int p = 0; p < 3; ++p) {
            futures.push_back(std::async(std::launch::async, [this, p, &pos_mid] {
                k3_pos[p] = vel_mid[p];
                auto acc = computeAccelerations(pos_mid);
                k3_vel[p] = acc[p];
            }));
        }
        for (auto& f : futures) f.get();
    }

    // ---------- k4 ----------
    std::vector<std::array<T,3>> pos_full(3), vel_full(3);
    for (int p = 0; p < 3; ++p) {
        for (int d = 0; d < 3; ++d) {
            pos_full[p][d] = positions[p][d] + tau * k3_pos[p][d];
            vel_full[p][d] = velocities[p][d] + tau * k3_vel[p][d];
        }
    }
    synchronize();
    {
        std::vector<std::future<void>> futures;
        for (int p = 0; p < 3; ++p) {
            futures.push_back(std::async(std::launch::async, [this, p, &pos_full] {
                k4_pos[p] = vel_full[p];
                auto acc = computeAccelerations(pos_full);
                k4_vel[p] = acc[p];
            }));
        }
        for (auto& f : futures) f.get();
    }

    // ---------- финальное обновление ----------
    for (int p = 0; p < 3; ++p) {
        for (int d = 0; d < 3; ++d) {
            positions[p][d] += (tau/6.0) * (k1_pos[p][d] + 2*k2_pos[p][d] + 2*k3_pos[p][d] + k4_pos[p][d]);
            velocities[p][d] += (tau/6.0) * (k1_vel[p][d] + 2*k2_vel[p][d] + 2*k3_vel[p][d] + k4_vel[p][d]);
        }
    }

    return true;
}

template<typename T>
void ThreeBodySolver<T>::ExportData(nlohmann::json* output) {
    (*output)["positions"] = nlohmann::json::array();
    for (const auto& p : positions) {
        (*output)["positions"].push_back({p[0], p[1], p[2]});
    }
}

} // namespace mm

#endif // INCLUDE_THREE_BODY_SOLVER_IMPL_HPP_