#include "../include/task_scheduler.h"
#include "../include/job_tracker.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <atomic>
#include <cstdlib>
#include <memory>

using json = nlohmann::json;

JobTracker global_job_tracker;
std::unique_ptr<TaskScheduler> global_scheduler;
std::atomic<bool> server_running{true};

// Grab a shorter representation of the Thread ID
std::string get_thread_id() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str().substr(ss.str().length() - 4); 
}

// Generate a random alpha-numeric string
std::string generate_random_string(int length) {
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string tmp_s;
    tmp_s.reserve(length);
    for (int i = 0; i < length; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return tmp_s;
}

// An interactive visual Brute Force simulation taking exactly 3.5 seconds
void execute_real_task(std::string job_id, std::string target_pwd) {
    std::string t_id = get_thread_id();
    global_job_tracker.update_job(job_id, "Running", 1, t_id, "Initializing Dictionary Attack...");
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Initial warmup delay
    
    // Simulate 100 chunks of hashes
    for (int percent = 1; percent <= 100; ++percent) {
        std::string current_attempt = generate_random_string(target_pwd.length());
        std::string intermediate = "Testing Hash: " + current_attempt + " (" + std::to_string(percent) + "%)";
        global_job_tracker.update_job(job_id, "Running", percent, t_id, intermediate);
        
        // Artificial delay makes it immensely satisfying to watch the thread process!
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
    }

    std::string result_str = "🔥 Hacked! Password is '" + target_pwd + "'";
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Final pause so running text settles
    global_job_tracker.update_job(job_id, "Completed", 100, t_id, result_str);
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    // 1. Initialize our existing task scheduler
    global_scheduler = std::make_unique<TaskScheduler>(4);
    
    // 2. Initialize the Web Server
    httplib::Server svr;

    // Serve Static UI Files (our frontend!)
    svr.set_mount_point("/", "./frontend/public");

    // Dynamic thread count endpoint
    svr.Post("/api/set_threads", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("count")) {
            int new_count = std::stoi(req.get_param_value("count"));
            if (new_count > 0 && new_count <= 64) {
                global_scheduler->shutdown(); // Safely close existing threads
                global_job_tracker.clear();   // Clear the UI
                global_scheduler = std::make_unique<TaskScheduler>(new_count); // Re-launch
            }
        }
        res.set_content("{\"status\":\"ok\"}", "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
    });

    svr.Post("/api/submit", [&](const httplib::Request& req, httplib::Response& res) {
        std::string job_id = generate_random_string(6);
        std::string target_pwd = "COOL"; 
        
        if (req.has_param("target_password")) {
            std::string provided_pwd = req.get_param_value("target_password");
            if (!provided_pwd.empty()) {
                target_pwd = provided_pwd;
            }
        }

        global_job_tracker.add_job(job_id, "Password Cracking");

        // Submitting to the Thread Pool!
        global_scheduler->submit([job_id, target_pwd]() {
            execute_real_task(job_id, target_pwd);
        });

        json responseData = {{"status", "queued"}, {"job_id", job_id}};
        res.set_content(responseData.dump(), "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
    });

    svr.Get("/api/stream", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_chunked_content_provider("text/event-stream",
            [&](size_t offset, httplib::DataSink &sink) {
                while (server_running) {
                    json data = global_job_tracker.get_all_jobs();
                    std::string event = "data: " + data.dump() + "\n\n";
                    if (!sink.write(event.data(), event.size())) {
                        return false; // Connection closed by client
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(250)); // 4 frames per second
                }
                return true;
            }
        );
    });

    // 3. Start the server on port 8081
    std::cout << "🚀 Starting Backend Tracker API on http://localhost:8081" << std::endl;
    svr.listen("0.0.0.0", 8081);

    // 4. Shutdown scheduler cleanly when server stops
    server_running = false;
    if (global_scheduler) {
        global_scheduler->shutdown();
    }
    return 0;
}