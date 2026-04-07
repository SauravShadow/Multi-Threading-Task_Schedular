#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <nlohmann/json.hpp>

struct JobState {
    std::string id;
    std::string type;
    std::string status; // "Queued", "Running", "Completed", "Cancelled"
    int progress;       // 0 to 100
    std::string thread_id; 
    std::string result_data; // Holds the final real-life output!
    bool cancelled = false;  // Cooperative cancellation flag
};

class JobTracker {
private:
    std::vector<JobState> jobs_in_order;
    std::unordered_map<std::string, size_t> id_to_index;
    std::mutex mtx;

public:
    void add_job(const std::string& id, const std::string& type) {
        std::lock_guard<std::mutex> lock(mtx);
        jobs_in_order.push_back({id, type, "Queued", 0, "", "", false});
        id_to_index[id] = jobs_in_order.size() - 1;
    }

    void update_job(const std::string& id, const std::string& status, int progress, const std::string& thread_id, const std::string& result = "") {
        std::lock_guard<std::mutex> lock(mtx);
        if (id_to_index.find(id) != id_to_index.end()) {
            size_t idx = id_to_index[id];
            // Don't overwrite a cancelled job's status
            if (jobs_in_order[idx].cancelled) return;
            jobs_in_order[idx].status = status;
            jobs_in_order[idx].progress = progress;
            jobs_in_order[idx].thread_id = thread_id;
            if (!result.empty()) {
                jobs_in_order[idx].result_data = result;
            }
        }
    }

    // Returns true if the job has been externally cancelled (or no longer exists)
    bool is_cancelled(const std::string& id) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = id_to_index.find(id);
        if (it == id_to_index.end()) return true; // Job purged -> treat as cancelled
        return jobs_in_order[it->second].cancelled;
    }

    // Mark a queued or running job as cancelled
    bool cancel_job(const std::string& id) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = id_to_index.find(id);
        if (it == id_to_index.end()) return false;
        size_t idx = it->second;
        jobs_in_order[idx].cancelled = true;
        jobs_in_order[idx].status = "Cancelled";
        jobs_in_order[idx].result_data = "Cancelled by user";
        return true;
    }

    nlohmann::json get_all_jobs() {
        std::lock_guard<std::mutex> lock(mtx);
        nlohmann::json result = nlohmann::json::array();
        for (const auto& state : jobs_in_order) {
            result.push_back({
                {"id", state.id},
                {"type", state.type},
                {"status", state.status},
                {"progress", state.progress},
                {"thread_id", state.thread_id},
                {"result", state.result_data}
            });
        }
        return result;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx);
        jobs_in_order.clear();
        id_to_index.clear();
    }
};
