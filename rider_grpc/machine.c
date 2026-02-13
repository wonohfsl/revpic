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
