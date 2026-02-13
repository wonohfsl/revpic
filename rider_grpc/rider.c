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
