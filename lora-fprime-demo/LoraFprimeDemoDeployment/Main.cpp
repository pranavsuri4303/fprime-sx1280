// ======================================================================
// \title  Main.cpp
// \brief main program for the F' application. Intended for CLI-based systems (Linux, macOS)
//
// ======================================================================
// Used to access topology functions
#include <LoraFprimeDemoDeployment/Top/LoraFprimeDemoDeploymentTopology.hpp>
// OSAL initialization
#include <Os/Os.hpp>
// Used for signal handling shutdown
#include <signal.h>
// Used for printf functions
#include <cstdlib>

static void signalHandler(int signum) {
    LoraFprimeDemoDeployment::stopRateGroups();
}

int main(int argc, char* argv[]) {
    Os::init();

    // Object for communicating state to the topology
    LoraFprimeDemoDeployment::TopologyState inputs;

    // Setup program shutdown via Ctrl-C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    (void)printf("Hit Ctrl-C to quit\n");

    // Setup, cycle, and teardown topology
    LoraFprimeDemoDeployment::setupTopology(inputs);
    LoraFprimeDemoDeployment::startRateGroups(Fw::TimeInterval(1,0));  // Program loop cycling rate groups at 1Hz
    LoraFprimeDemoDeployment::teardownTopology(inputs);
    (void)printf("Exiting...\n");
    return 0;
}
