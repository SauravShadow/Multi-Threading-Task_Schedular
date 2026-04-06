# 🚀 C++ Multi-Threading Task Scheduler & Visualizer

![C++](https://img.shields.io/badge/C++-23-blue.svg?style=flat&logo=c%2B%2B)
![CMake](https://img.shields.io/badge/CMake-3.14+-red.svg?style=flat&logo=cmake)
![SSE](https://img.shields.io/badge/Architecture-Server%20Sent%20Events-8b5cf6.svg?style=flat)
![Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg?style=flat)

A highly concurrent, low-latency **C++ Backend Engine** coupled with a modern dynamic **HTML5 Web Dashboard**. This project demonstrates OS-level hardware multithreading, thread-safe memory synchronization, and asynchronous job execution.

The visualizer simulates brute-forcing custom passwords in real-time, showing exactly how a Thread Pool distributes executing tasks across multiple CPU cores without UI polling lag!

---

## ⚡ Features
- **Custom Internal Thread Pool**: Manage a fixed block of persistently living `std::thread` workers instead of repeatedly experiencing OS-allocation overhead.
- **Dynamic Resizing**: Alter the number of active CPU threads processing the queue during runtime directly from the web interface.
- **SSE Native Streaming**: Operates exactly on a zero-lag Server-Sent Events architecture, eliminating WebSockets and Javascript polling bottlenecks.
- **Modern C++ Semantics**: Heavily leverages `<future>`, `<mutex>`, `std::condition_variable`, and `std::packaged_task`.

---

## 🏢 High Level Design (HLD)

The overarching architecture bridges a modern interactive frontend with a highly concurrent C++ backend.

The engine uses **`cpp-httplib`** to spawn a multi-threaded listener capable of handling concurrent REST API invocations (`/api/submit`). When jobs are posted, they completely bypass main-thread HTTP bottlenecks by dispatching straight into the C++ `TaskScheduler`. A unidirectional HTTP stream is opened at `/api/stream` where native HTML5 `EventSource` protocols observe real-time job execution states.

## ⚙️ Low Level Design (LLD)

Beneath the HTTP handler, execution is fully asynchronous. 
Job functions are wrapped into `std::packaged_task<std::string()>` functors and pushed inside a `std::queue<std::function<void()>>`.
Simultaneously, a massive globally synchronized `JobTracker` meticulously records the state, hash outputs, and iteration progress into a dynamically mapped `unordered_map`. This structure enforces strict concurrency control, ensuring that worker updates never collide with the 250ms HTTP stream serialization loop via **`nlohmann/json`**.

---

## 🏗️ Core Mechanisms

### 1. Thread-Safe Blocking Queue 🚧
C++ `std::queue` is fundamentally unsafe for concurrent read/writes. This system implements a generic `BlockingQueue` wrapper.
- **Producers**: Locks a `std::mutex` via `std::lock_guard` during `push()`.
- **Consumers**: Prevents CPU busy-waiting. Worker threads block efficiently utilizing `cond_var.wait(lock, []{ return stop || !queue.empty(); })`.

### 2. Asynchronous Futures & Promises 🚀
When heavy tasks (like brute-forcing passwords) enter the Scheduler, the API guarantees zero connection-dropping timeouts. The `TaskScheduler::submit()` method dispatches the lambda and immediately yields a `std::future<auto>`. The backend seamlessly accepts the request and pushes the calculation deep into the background, responding to the caller instantly.

---

## 🚀 Getting Started

### Prerequisites
- macOS / Linux environment
- **CMake** `v3.14+`
- **C++23** capable compiler (Apple Clang / GCC13)

### Build & Run
```bash
# 1. Clone the repository
git clone https://github.com/SauravShadow/Multi-Threading-Task_Schedular.git
cd Multi-Threading-Task_Schedular

# 2. Build via CMake
mkdir build && cd build
cmake ..
cmake --build .

# 3. Start the Server
./task_schedular
```
**Access the Web Visualizer** by visiting: [http://localhost:8081](http://localhost:8081) in your browser!

---

## 👨‍💻 Tech Stack
- **Backend Core**: C++23 
- **Web Server Core**: `yhirose/cpp-httplib`
- **JSON Serialization**: `nlohmann/json`
- **Frontend Dashboard**: HTML5, Vanilla Javascript, Vanilla CSS (Glassmorphism layout)
