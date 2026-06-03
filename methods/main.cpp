/**
 * @file methods/main.cpp
 * @author Mikhail Lozhnikov
 *
 * Файл с функцией main() для серверной части программы.
 */

#include <httplib.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <nlohmann/json.hpp>
#include "tasks_queue.hpp"
#include "methods.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
  int port = 8080;
  if (argc >= 2) {
    if (std::sscanf(argv[1], "%d", &port) != 1)
      return -1;
  }

  std::cerr << "Listening on port " << port << "..." << std::endl;
  httplib::Server svr;
  mm::TasksQueue tasksQueue;

  svr.Get("/stop", [&](const httplib::Request&, httplib::Response& res) {
    svr.stop();
    res.status = 200;
  });

  svr.Post("/CheckTaskStatus", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json input = nlohmann::json::parse(req.body);
    nlohmann::json output;
    int taskId = input["id"];
    output["id"] = taskId;
    output["status"] = tasksQueue.IsTaskFinished(taskId) ? "finished" : "unknown";
    res.set_content(output.dump(), "application/json");
  });

  svr.Post("/DownloadTaskData", [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json input = nlohmann::json::parse(req.body);
    nlohmann::json output;
    int taskId = input["id"];
    if (tasksQueue.IsTaskFinished(taskId)) {
      output = tasksQueue.GetFinishedTaskData(taskId);
    } else {
      output["status"] = "unknown";
    }
    output["id"] = taskId;
    res.set_content(output.dump(), "application/json");
  });

  // Обработчик для эталонного примера теплопроводности (имя исправлено)
  svr.Post("/HeatConductionReferenceExampleSolver",
      [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json input = nlohmann::json::parse(req.body);
    nlohmann::json output;
    if (mm::HeatConductionReferenceExampleSolverMethod(input, &output, &tasksQueue) < 0)
      res.status = 400;
    res.set_content(output.dump(), "application/json");
  });

  // Обработчик для задачи трёх тел
  svr.Post("/ThreeBodySolver",
      [&](const httplib::Request& req, httplib::Response& res) {
    nlohmann::json input = nlohmann::json::parse(req.body);
    nlohmann::json output;
    if (mm::ThreeBodySolverMethod(input, &output, &tasksQueue) < 0)
      res.status = 400;
    res.set_content(output.dump(), "application/json");
  });

  svr.listen("0.0.0.0", port);
  return 0;
}