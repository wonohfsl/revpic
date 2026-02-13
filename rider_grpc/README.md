Requirements:  
- multiple C control programs (`rotate.c`, `tilt.c`, `home.c`, `calibrate.c`)  
- a main daemon (`machine`)  
- a JS “rider” container  
- session commands  
- pause/resume/stop  
- E‑Stop monitoring  
- PLC‑style deterministic behavior  

---

# OVERALL SYSTEM ARCHITECTURE DIAGRAM

```
┌────────────────────────────────────────────────────────────────────────────┐
│                                rider (JS)                                  │
│                                                                            │
│  - Holds session info                                                      │
│  - Sends high-level commands:                                              │
│       Start, Pause, Resume, Stop, Home, Calibrate                          │
│  - Receives status:                                                        │
│       RUNNING, DONE, FAULT_ESTOP, FAULT_HW, PAUSED                         │
│                                                                            │
│  IPC: gRPC / UNIX socket / JSON-RPC                                        │
└───────────────────────────────▲────────────────────────────────────────────┘
                                │  High-level commands + status
                                │
                                │
┌───────────────────────────────┴────────────────────────────────────────────┐
│                         machine (C daemon)                                 │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         ControlAPI (State Machine)                   │  │
│  │                                                                      │  │
│  │  States:                                                             │  │
│  │    Idle                                                              │  │
│  │    Home                                                              │  │
│  │    Tilt_Running                                                      │  │
│  │    Rotate_Running                                                    │  │
│  │    Go_Homing → Go_Tilting → Go_Rotating                              │  │
│  │    CalibrateTilt / CalibrateRotate                                   │  │
│  │    Fault_EStop                                                       │  │
│  │    Fault_Other                                                       │  │
│  │    Paused                                                            │  │
│  │                                                                      │  │
│  │  Events:                                                             │  │
│  │    StartSession                                                      │  │
│  │    Pause / Resume / Stop                                             │  │
│  │    EStopPressed / EStopReleased                                      │  │
│  │                                                                      │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         Controllers (Non-blocking)                   │  │
│  │                                                                      │  │
│  │  HomeController                                                      │  │
│  │  RotateHomeController                                                │  │
│  │  TiltHomeController                                                  │  │
│  │  RotateOneController                                                 │  │
│  │  TiltToDegreeController                                              │  │
│  │  CalibrateRotateController                                           │  │
│  │  CalibrateTiltController                                             │  │
│  │                                                                      │  │
│  │  Each controller:                                                    │  │
│  │    start() → update() → done()                                       │  │
│  │    checks EStop every scan                                           │  │
│  │    can pause/resume                                                  │  │
│  │                                                                      │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         Device Layer                                 │  │
│  │                                                                      │  │
│  │  RotateAxis                                                          │  │
│  │     - rotateOne(dir)                                                 │  │
│  │     - stop()                                                         │  │
│  │     - isHome()                                                       │  │
│  │                                                                      │  │
│  │  TiltAxis                                                            │  │
│  │     - tiltOne(dir)                                                   │  │
│  │     - stop()                                                         │  │
│  │     - readPositionDegrees()                                          │  │
│  │                                                                      │  │
│  │  Safety                                                              │  │
│  │     - isEStopActive()                                                │  │
│  │                                                                      │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         IO Layer (Typed IO)                          │  │
│  │                                                                      │  │
│  │  DigitalInput: rotateHome, tiltHome, estop                           │  │
│  │  AnalogInput: tiltPosition                                           │  │
│  │  RelayOutput: rotateRelay, rotateDir, tiltRelay, tiltDir             │  │
│  │                                                                      │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         HAL (RevPi Process Image)                    │  │
│  │                                                                      │  │
│  │  scanInputs(): read DI/AI                                            │  │
│  │  scanOutputs(): write RO/DO/AO                                       │  │
│  │  forceAllOutputsSafe(): drop all relays                              │  │
│  │  readEStop(): DI3                                                    │  │
│  │                                                                      │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                            │
│  Main Loop (PLC-style):                                                    │
│    while(true):                                                            │
│       hal.scanInputs()                                                     │
│       if (estop):                                                          │
│           controlAPI.handleEStop()                                         │
│           hal.forceAllOutputsSafe()                                        │
│           continue                                                         │
│       controlAPI.update()                                                  │
│       hal.scanOutputs()                                                    │
│       sleep(10ms)                                                          │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

---
---

# What gRPC Actually Is (Explained Simply)

gRPC is:

- A **high‑performance RPC (Remote Procedure Call) framework**
- Created by Google
- Uses **Protocol Buffers** (protobuf) for message definitions
- Supports **bi‑directional streaming**
- Works across languages (C, C++, JS, Python, Go, etc.)
- Runs over HTTP/2

### Why it’s perfect for rider ↔ machine

| Requirement | gRPC Fit |
|------------|----------|
| Rider (JS) ↔ Machine (C) | ✔ Multi‑language support |
| Commands like Start, Pause, Resume | ✔ RPC calls map perfectly |
| Machine sends status updates | ✔ Server‑side streaming |
| Low latency | ✔ HTTP/2 + protobuf |
| Strong typing | ✔ Protobuf schemas |
| Easy to evolve | ✔ Versioned messages |

### Why it’s better than raw sockets + JSON

- **type safety**
- **code generation** (no manual parsing)
- **streaming** for status updates
- **automatic reconnection**
- **clean API definitions** in a `.proto` file

---
---

# Example

### Timeline

| Time | Action |
|------|--------|
| 0s | rider → machine: Start |
| 0–10s | machine timer runs |
| 10s | rider → machine: Pause |
| 10–15s | timer paused |
| 15s | rider → machine: Resume |
| 15–60s | timer continues |
| 60s | machine → rider: Done |


---

### Architecture for rider ↔ machine using gRPC

```
rider (JS)
   |
   | gRPC request: Start, Pause, Resume, Stop
   |
machine (C daemon)
   |
   | gRPC stream: Status updates (Running, Paused, Done, Fault)
   |
ControlAPI state machine
   |
Controllers (Rotate, Tilt, Home, etc.)
   |
HAL → RevPi hardware
```

---

### The `.proto` file (the contract)

This is the heart of gRPC.

```proto
syntax = "proto3";

package machinecontrol;

service MachineControl {
  rpc Start(StartRequest) returns (Ack);
  rpc Pause(PauseRequest) returns (Ack);
  rpc Resume(ResumeRequest) returns (Ack);
  rpc Stop(StopRequest) returns (Ack);

  // Rider subscribes to status updates
  rpc StatusStream(StatusRequest) returns (stream StatusMessage);
}

message StartRequest {}
message PauseRequest {}
message ResumeRequest {}
message StopRequest {}
message StatusRequest {}

message Ack {
  bool ok = 1;
  string message = 2;
}

message StatusMessage {
  string state = 1;   // "Running", "Paused", "Done", "Stopped"
  int32 elapsedSec = 2;
}
```

This defines:

- Commands: Start, Pause, Resume, Stop
- A streaming RPC: StatusStream()

---

## Code

### machine.c (C gRPC server)

This is a simplified conceptual example.  
It shows:

- A 60‑second timer
- Pause/resume logic
- Status streaming

```c
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <chrono>
#include <thread>
#include "machinecontrol.grpc.pb.h"

using namespace machinecontrol;

enum TimerState { IDLE, RUNNING, PAUSED, STOPPED, DONE };

class MachineService final : public MachineControl::Service {
public:
    TimerState state = IDLE;
    int elapsed = 0;

    grpc::Status Start(grpc::ServerContext*, const StartRequest*, Ack* reply) override {
        state = RUNNING;
        elapsed = 0;
        reply->set_ok(true);
        reply->set_message("Started");
        return grpc::Status::OK;
    }

    grpc::Status Pause(grpc::ServerContext*, const PauseRequest*, Ack* reply) override {
        if (state == RUNNING) state = PAUSED;
        reply->set_ok(true);
        reply->set_message("Paused");
        return grpc::Status::OK;
    }

    grpc::Status Resume(grpc::ServerContext*, const ResumeRequest*, Ack* reply) override {
        if (state == PAUSED) state = RUNNING;
        reply->set_ok(true);
        reply->set_message("Resumed");
        return grpc::Status::OK;
    }

    grpc::Status Stop(grpc::ServerContext*, const StopRequest*, Ack* reply) override {
        state = STOPPED;
        reply->set_ok(true);
        reply->set_message("Stopped");
        return grpc::Status::OK;
    }

    grpc::Status StatusStream(grpc::ServerContext*,
                              const StatusRequest*,
                              grpc::ServerWriter<StatusMessage>* writer) override {

        while (true) {
            StatusMessage msg;

            if (state == RUNNING) {
                elapsed++;
                if (elapsed >= 60) {
                    state = DONE;
                }
            }

            msg.set_state(StateToString(state));
            msg.set_elapsedsec(elapsed);
            writer->Write(msg);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return grpc::Status::OK;
    }

private:
    std::string StateToString(TimerState s) {
        switch (s) {
            case RUNNING: return "Running";
            case PAUSED: return "Paused";
            case STOPPED: return "Stopped";
            case DONE: return "Done";
            default: return "Idle";
        }
    }
};

int main() {
    MachineService service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
}
```

This is a minimal working server.

---

### rider.c (C gRPC client)

This client:

- Sends Start
- Waits 10 seconds
- Sends Pause
- Waits 5 seconds
- Sends Resume
- Reads status stream

```c
#include <grpcpp/grpcpp.h>
#include <chrono>
#include <thread>
#include "machinecontrol.grpc.pb.h"

using namespace machinecontrol;

int main() {
    auto channel = grpc::CreateChannel("localhost:50051",
                                       grpc::InsecureChannelCredentials());
    std::unique_ptr<MachineControl::Stub> stub = MachineControl::NewStub(channel);

    grpc::ClientContext ctx1;
    StartRequest startReq;
    Ack startAck;
    stub->Start(&ctx1, startReq, &startAck);

    std::cout << "Sent Start" << std::endl;

    // Start reading status stream
    grpc::ClientContext ctxStream;
    StatusRequest req;
    std::unique_ptr<grpc::ClientReader<StatusMessage>> reader(
        stub->StatusStream(&ctxStream, req));

    std::thread statusThread([&]() {
        StatusMessage msg;
        while (reader->Read(&msg)) {
            std::cout << "Status: " << msg.state()
                      << " elapsed=" << msg.elapsedsec() << std::endl;
        }
    });

    // After 10 sec → Pause
    std::this_thread::sleep_for(std::chrono::seconds(10));
    grpc::ClientContext ctx2;
    PauseRequest pauseReq;
    Ack pauseAck;
    stub->Pause(&ctx2, pauseReq, &pauseAck);
    std::cout << "Sent Pause" << std::endl;

    // After 5 sec → Resume
    std::this_thread::sleep_for(std::chrono::seconds(5));
    grpc::ClientContext ctx3;
    ResumeRequest resumeReq;
    Ack resumeAck;
    stub->Resume(&ctx3, resumeReq, &resumeAck);
    std::cout << "Sent Resume" << std::endl;

    statusThread.join();
}
```

---
---

# Need to Install

To build and run the example:

### **Required packages**
Inside the container (or host):

```
sudo apt-get update
sudo apt-get install -y build-essential autoconf libtool pkg-config \
    cmake git protobuf-compiler libprotobuf-dev
```

### **Install gRPC C/C++**
The official method:

Build Abseil (required for gRPC ≥ 1.50)
```
cd ~/grpc/third_party/abseil-cpp
mkdir build
cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ..
make -j1
sudo make install
sudo ldconfig
```

Build gRPC 
```
git clone --recurse-submodules -b v1.63.0 https://github.com/grpc/grpc
cd grpc
mkdir -p cmake/build
cd cmake/build
cmake ../..
make -j4
sudo make install
sudo ldconfig
```

This installs:

- `libgrpc`
- `libgrpc++`
- `protoc` plugins for gRPC
- protobuf libraries


Additional required packages
```
sudo apt-get install -y libssl-dev zlib1g-dev

```

---

## Build Steps for the Example

Assume you have:

```
machinecontrol.proto
machine.c
rider.c
```

### Generate gRPC code from .proto

```
protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` machinecontrol.proto
protoc --cpp_out=. machinecontrol.proto
```

This produces:

- `machinecontrol.pb.cc`
- `machinecontrol.pb.h`
- `machinecontrol.grpc.pb.cc`
- `machinecontrol.grpc.pb.h`

---

### Build machine.c (the server)

```
g++ machine.c machinecontrol.pb.cc machinecontrol.grpc.pb.cc \
    -lgrpc++ -lgrpc -lprotobuf -lpthread -o machine
```

---

### Build rider.c (the client)

```
g++ rider.c machinecontrol.pb.cc machinecontrol.grpc.pb.cc \
    -lgrpc++ -lgrpc -lprotobuf -lpthread -o rider
```

---

## Run the test

### Terminal 1 — run the machine daemon

```
./machine
```

It will start listening on port 50051.

### Terminal 2 — run the rider

```
./rider
```

You should see:

```
Sent Start
Status: Running elapsed=1
Status: Running elapsed=2
...
Sent Pause
Status: Paused elapsed=10
...
Sent Resume
Status: Running elapsed=15
...
Status: Done elapsed=60
```

---

## How to test rider ↔ machine communication

If test across containers:

- Put both containers on the same Docker network
- Expose port 50051 from machine container
- Rider container connects to `machine:50051`

---

# Install in the container

### Run inside BalenaOS containers:

- gRPC C/C++ libraries
- protobuf compiler
- build tools

Add this to *Dockerfile*:

```
RUN apt-get update && apt-get install -y \
    build-essential autoconf libtool pkg-config cmake git \
    protobuf-compiler libprotobuf-dev
```

Then clone and build gRPC inside the container.
