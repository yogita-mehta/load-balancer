#include "MiniTest.h"
#include "Strategies.h"
#include "Server.h"
#include <vector>
#include <memory>

namespace {
std::vector<std::unique_ptr<Server>> makeServers(int count, int capacity = 10) {
    std::vector<std::unique_ptr<Server>> servers;
    for (int i = 0; i < count; ++i) servers.push_back(std::make_unique<Server>(i, 1, capacity));
    return servers;
}
} // namespace

TEST(RoundRobin, CyclesThroughAllServers) {
    auto servers = makeServers(3);
    RoundRobinStrategy strat;
    std::vector<int> picks;
    for (int i = 0; i < 6; ++i) picks.push_back(strat.pickServer(servers)->id());
    EXPECT_EQ(picks[0], 0);
    EXPECT_EQ(picks[1], 1);
    EXPECT_EQ(picks[2], 2);
    EXPECT_EQ(picks[3], 0);
}

TEST(RoundRobin, SkipsDownServers) {
    auto servers = makeServers(3);
    servers[1]->setHealth(ServerHealth::DOWN);
    RoundRobinStrategy strat;
    for (int i = 0; i < 4; ++i) {
        Server* s = strat.pickServer(servers);
        EXPECT_TRUE(s != nullptr);
        EXPECT_NE(s->id(), 1);
    }
}

TEST(RoundRobin, ReturnsNullWhenAllDown) {
    auto servers = makeServers(2);
    servers[0]->setHealth(ServerHealth::DOWN);
    servers[1]->setHealth(ServerHealth::DOWN);
    RoundRobinStrategy strat;
    EXPECT_TRUE(strat.pickServer(servers) == nullptr);
}

TEST(LeastConnections, PicksServerWithFewestConnections) {
    auto servers = makeServers(3);
    servers[0]->beginRequest();
    servers[0]->beginRequest();
    servers[1]->beginRequest();
    LeastConnectionsStrategy strat;
    Server* picked = strat.pickServer(servers);
    EXPECT_EQ(picked->id(), 2); // server 2 has 0 active connections
}

TEST(WeightedRoundRobin, HigherWeightGetsMoreTraffic) {
    std::vector<std::unique_ptr<Server>> servers;
    servers.push_back(std::make_unique<Server>(0, 1, 100));
    servers.push_back(std::make_unique<Server>(1, 3, 100)); // 3x weight
    WeightedRoundRobinStrategy strat;
    int counts[2] = {0, 0};
    for (int i = 0; i < 400; ++i) {
        Server* s = strat.pickServer(servers);
        counts[s->id()]++;
    }
    // Server 1 should receive roughly 3x the traffic of server 0.
    EXPECT_GT(counts[1], counts[0] * 2);
}

TEST(PowerOfTwoChoices, NeverPicksUnavailableServer) {
    auto servers = makeServers(4);
    servers[0]->setHealth(ServerHealth::DOWN);
    servers[2]->setHealth(ServerHealth::DOWN);
    PowerOfTwoChoicesStrategy strat;
    for (int i = 0; i < 20; ++i) {
        Server* s = strat.pickServer(servers);
        EXPECT_TRUE(s != nullptr);
        EXPECT_TRUE(s->id() == 1 || s->id() == 3);
    }
}

TEST(LeastResponseTime, PrefersFasterServer) {
    auto servers = makeServers(2);
    servers[0]->beginRequest();
    servers[0]->endRequest(100.0, false); // slow history
    servers[1]->beginRequest();
    servers[1]->endRequest(10.0, false);  // fast history
    LeastResponseTimeStrategy strat;
    Server* picked = strat.pickServer(servers);
    EXPECT_EQ(picked->id(), 1);
}

TEST(Server, TracksActiveConnectionsCorrectly) {
    Server s(0, 1, 10);
    EXPECT_EQ(s.activeConnections(), 0);
    s.beginRequest();
    s.beginRequest();
    EXPECT_EQ(s.activeConnections(), 2);
    s.endRequest(5.0, false);
    EXPECT_EQ(s.activeConnections(), 1);
}

TEST(Server, CapacityGatesAvailability) {
    Server s(0, 1, 2);
    s.beginRequest();
    s.beginRequest();
    EXPECT_FALSE(s.isAvailable()); // at capacity
    s.endRequest(1.0, false);
    EXPECT_TRUE(s.isAvailable());
}

int main() {
    return minitest::runAll();
}
