#include "../include/task_scheduler.h"
#include "../include/job_tracker.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include <random>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;

JobTracker global_job_tracker;

// global_scheduler is swapped at runtime by /api/set_threads while other request
// threads are calling into it. A shared_ptr guarded by a mutex lets in-flight
// callers keep the old instance alive until they finish, avoiding a use-after-free.
std::shared_ptr<TaskScheduler> global_scheduler;
std::mutex scheduler_mtx;
std::atomic<bool> server_running{true};

// Grab a stable reference to the current scheduler for the duration of a call.
std::shared_ptr<TaskScheduler> get_scheduler() {
    std::lock_guard<std::mutex> lock(scheduler_mtx);
    return global_scheduler;
}

// Grab a shorter representation of the Thread ID
std::string get_thread_id() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str().substr(ss.str().length() - 4); 
}

// Generate a random alpha-numeric string.
// Uses a thread_local RNG so concurrent worker threads never share generator state
// (rand() is not guaranteed thread-safe by POSIX).
std::string generate_random_string(int length) {
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    thread_local std::mt19937 rng(std::random_device{}());
    thread_local std::uniform_int_distribution<int> dist(0, sizeof(alphanum) - 2);
    std::string tmp_s;
    tmp_s.reserve(length);
    for (int i = 0; i < length; ++i) {
        tmp_s += alphanum[dist(rng)];
    }
    return tmp_s;
}

// An interactive visual Brute Force simulation taking exactly 3.5 seconds
void execute_real_task(std::string job_id, std::string target_pwd) {
    std::string t_id = get_thread_id();

    // Check if already cancelled before even starting
    if (global_job_tracker.is_cancelled(job_id)) return;

    global_job_tracker.update_job(job_id, "Running", 1, t_id, "Initializing Dictionary Attack...");
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Initial warmup delay
    
    // Simulate 100 chunks of hashes
    for (int percent = 1; percent <= 100; ++percent) {
        // Cooperative cancellation check each iteration
        if (global_job_tracker.is_cancelled(job_id)) return;

        std::string current_attempt = generate_random_string(target_pwd.length());
        std::string intermediate = "Testing Hash: " + current_attempt + " (" + std::to_string(percent) + "%)";
        global_job_tracker.update_job(job_id, "Running", percent, t_id, intermediate);
        
        // Artificial delay makes it immensely satisfying to watch the thread process!
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
    }

    if (global_job_tracker.is_cancelled(job_id)) return;
    std::string result_str = "Hacked! Password is '" + target_pwd + "'";
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Final pause so running text settles
    global_job_tracker.update_job(job_id, "Completed", 100, t_id, result_str);
}

int main() {
    // 1. Initialize our existing task scheduler
    global_scheduler = std::make_shared<TaskScheduler>(4);
    
    // 2. Initialize the Web Server
    httplib::Server svr;

    // Serve Static UI Files (our frontend!)
    auto base_dir = std::filesystem::current_path().string();
    std::string public_dir = base_dir + "/frontend/public";
    svr.set_mount_point("/", public_dir);
    
    // Explicitly handle the root to serve index.html
    svr.Get("/", [public_dir](const httplib::Request& req, httplib::Response& res) {
        std::ifstream file(public_dir + "/index.html");
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            res.set_content(content, "text/html");
        } else {
            res.status = 404;
            res.set_content("UI File Not Found", "text/plain");
        }
    });

    // Dynamic thread count endpoint
    svr.Post("/api/set_threads", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        if (req.has_param("count")) {
            int new_count = 0;
            try {
                new_count = std::stoi(req.get_param_value("count"));
            } catch (const std::exception&) {
                res.status = 400;
                res.set_content("{\"error\":\"count must be an integer\"}", "application/json");
                return;
            }
            if (new_count > 0 && new_count <= 64) {
                std::shared_ptr<TaskScheduler> old;
                {
                    std::lock_guard<std::mutex> lock(scheduler_mtx);
                    old = std::move(global_scheduler);
                    global_scheduler = std::make_shared<TaskScheduler>(new_count);
                }
                // Clear first so any in-flight workers of the old scheduler see the
                // cancellation and exit quickly, then drop our reference to it. The
                // old instance is destroyed once no request thread still holds it.
                global_job_tracker.clear();
                old.reset();
            }
        }
        res.set_content("{\"status\":\"ok\"}", "application/json");
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

        // Submitting to the Thread Pool! Take a stable reference in case a
        // /api/set_threads resize is swapping the scheduler concurrently.
        get_scheduler()->submit([job_id, target_pwd]() {
            execute_real_task(job_id, target_pwd);
        });

        json responseData = {{"status", "queued"}, {"job_id", job_id}};
        res.set_content(responseData.dump(), "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
    });

    // Cancel a single job by ID
    svr.Post("/api/cancel", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        if (req.has_param("job_id")) {
            std::string job_id = req.get_param_value("job_id");
            bool ok = global_job_tracker.cancel_job(job_id);
            json resp = {{"status", ok ? "cancelled" : "not_found"}, {"job_id", job_id}};
            res.set_content(resp.dump(), "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"error\":\"job_id param required\"}", "application/json");
        }
    });

    // Cancel ALL queued and running jobs
    svr.Post("/api/cancel_all", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        global_job_tracker.clear();
        json resp = {{"status", "all_cleared"}};
        res.set_content(resp.dump(), "application/json");
    });

    svr.Get("/api/stream", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("X-Accel-Buffering", "no"); // Disable Nginx/proxy buffering
        res.set_chunked_content_provider("text/event-stream",
            [&](size_t offset, httplib::DataSink &sink) {
                int tick = 0;
                while (server_running) {
                    // Every 30 seconds send an SSE comment as a keep-alive heartbeat
                    // This prevents Cloudflare's 100-second proxy timeout from killing the stream
                    if (tick % 120 == 0 && tick > 0) {
                        std::string ping = ": ping\n\n";
                        if (!sink.write(ping.data(), ping.size())) return false;
                    }

                    json data = global_job_tracker.get_all_jobs();
                    std::string event = "data: " + data.dump() + "\n\n";
                    if (!sink.write(event.data(), event.size())) {
                        return false; // Connection closed by client
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(250)); // 4 frames per second
                    tick++;
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