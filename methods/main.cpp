#include <httplib.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include "tasks_queue.hpp"
#include "methods.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc >= 2) {
        if (std::sscanf(argv[1], "%d", &port) != 1) return -1;
    }

    std::cerr << "Listening on port " << port << "..." << std::endl;
    httplib::Server svr;
    mm::TasksQueue tasksQueue;

    // Остановка сервера
    svr.Get("/stop", [&](const httplib::Request&, httplib::Response& res) {
        svr.stop();
        res.status = 200;
    });

    // Проверка статуса задачи
    svr.Post("/CheckTaskStatus", [&](const httplib::Request& req, httplib::Response& res) {
        auto input = json::parse(req.body);
        json output;
        int taskId = input["id"];
        output["id"] = taskId;
        output["status"] = tasksQueue.IsTaskFinished(taskId) ? "finished" : "unknown";
        res.set_content(output.dump(), "application/json");
    });

    // Скачивание данных завершённой задачи
    svr.Post("/DownloadTaskData", [&](const httplib::Request& req, httplib::Response& res) {
        auto input = json::parse(req.body);
        json output;
        int taskId = input["id"];
        if (tasksQueue.IsTaskFinished(taskId)) {
            output = tasksQueue.GetFinishedTaskData(taskId);
        } else {
            output["status"] = "unknown";
        }
        output["id"] = taskId;
        res.set_content(output.dump(), "application/json");
    });

    // Обработчик для нашего алгоритма
    svr.Post("/ThreeBodySolver", [&](const httplib::Request& req, httplib::Response& res) {
        auto input = json::parse(req.body);
        json output;
        if (mm::ThreeBodySolverMethod(input, &output, &tasksQueue) < 0) {
            res.status = 400;
        }
        res.set_content(output.dump(), "application/json");
    });

    svr.listen("0.0.0.0", port);
    return 0;
}