#include <gtest/gtest.h>
#include "core/simulation/time/hako_time.hpp"

class HakoTimeTest : public ::testing::Test {
protected:
    static void SetUpTestCase()
    {
    }
    static void TearDownTestCase()
    {
    }
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }

};

TEST_F(HakoTimeTest, TheWorld_01)
{
    HakoTimeType max_delay_time_usec = 10000;
    HakoTimeType delta_time_usec = 1000;
    hako::core::simulation::time::TheWorld theWorld(max_delay_time_usec, delta_time_usec);

    EXPECT_EQ(0, theWorld.get_world_time_usec());
    EXPECT_EQ(0, theWorld.get_wait_time_usec());
}

TEST_F(HakoTimeTest, TheWorld_02)
{
    HakoTimeType max_delay_time_usec = 10000;
    HakoTimeType delta_time_usec = 1000;
    hako::core::simulation::time::TheWorld theWorld(max_delay_time_usec, delta_time_usec);

    std::vector<HakoTimeType> asset_times;
    asset_times.push_back(0);
    asset_times.push_back(0);

    HakoTimeType prev_time = theWorld.get_world_time_usec();

    HakoTimeType world_time = theWorld.time_begins_to_move(asset_times);

    EXPECT_EQ(prev_time + delta_time_usec, world_time);
}

TEST_F(HakoTimeTest, TheWorld_03)
{
    HakoTimeType max_delay_time_usec = 10000;
    HakoTimeType delta_time_usec = 1000;
    hako::core::simulation::time::TheWorld theWorld(max_delay_time_usec, delta_time_usec);

    std::vector<HakoTimeType> asset_times;
    asset_times.push_back(0);
    asset_times.push_back(0);

    for (int i = 0; i < 10; i++) {
        HakoTimeType prev_time = theWorld.get_world_time_usec();

        asset_times[0] += delta_time_usec; 
        asset_times[1] += delta_time_usec; 
        HakoTimeType world_time = theWorld.time_begins_to_move(asset_times);

        EXPECT_EQ(prev_time + delta_time_usec, world_time);
    }
}

TEST_F(HakoTimeTest, TheWorld_04)
{
    HakoTimeType max_delay_time_usec = 10000;
    HakoTimeType delta_time_usec = 1000;
    hako::core::simulation::time::TheWorld theWorld(max_delay_time_usec, delta_time_usec);

    std::vector<HakoTimeType> asset_times;
    asset_times.push_back(0);
    asset_times.push_back(0);

    // delaying assets...
    for (int i = 0; i < 10; i++) {
        HakoTimeType prev_time = theWorld.get_world_time_usec();
        HakoTimeType world_time = theWorld.time_begins_to_move(asset_times);
        EXPECT_EQ(prev_time + delta_time_usec, world_time);
    }
    // the world time steps to max
    HakoTimeType world_time = theWorld.get_world_time_usec();
    EXPECT_EQ(max_delay_time_usec, world_time);

    // then can not step foward
    HakoTimeType prev_time = theWorld.get_world_time_usec();
    world_time = theWorld.time_begins_to_move(asset_times);
    EXPECT_EQ(prev_time, world_time);
    EXPECT_EQ(delta_time_usec, theWorld.get_wait_time_usec());
}


TEST_F(HakoTimeTest, TheWorld_05)
{
    HakoTimeType max_delay_time_usec = 10000;
    HakoTimeType delta_time_usec = 1000;
    hako::core::simulation::time::TheWorld theWorld(max_delay_time_usec, delta_time_usec);

    std::vector<HakoTimeType> asset_times;
    asset_times.push_back(0);
    asset_times.push_back(0);

    // delaying assets...
    for (int i = 0; i < 10; i++) {
        HakoTimeType prev_time = theWorld.get_world_time_usec();
        HakoTimeType world_time = theWorld.time_begins_to_move(asset_times);
        EXPECT_EQ(prev_time + delta_time_usec, world_time);
    }
    // the world time steps to max
    HakoTimeType world_time = theWorld.get_world_time_usec();
    EXPECT_EQ(max_delay_time_usec, world_time);

    // then can not step foward
    HakoTimeType prev_time = theWorld.get_world_time_usec();
    world_time = theWorld.time_begins_to_move(asset_times);
    EXPECT_EQ(prev_time, world_time);
    EXPECT_EQ(delta_time_usec, theWorld.get_wait_time_usec());

    // only one asset stepped
    asset_times[0] = delta_time_usec;

    // but can not step foward
    prev_time = theWorld.get_world_time_usec();
    world_time = theWorld.time_begins_to_move(asset_times);
    EXPECT_EQ(prev_time, world_time);
    EXPECT_EQ(delta_time_usec * 2, theWorld.get_wait_time_usec());
}


TEST_F(HakoTimeTest, TheWorld_06)
{
    HakoTimeType max_delay_time_usec = 10000;
    HakoTimeType delta_time_usec = 1000;
    hako::core::simulation::time::TheWorld theWorld(max_delay_time_usec, delta_time_usec);

    std::vector<HakoTimeType> asset_times;
    asset_times.push_back(0);
    asset_times.push_back(0);

    // delaying assets...
    for (int i = 0; i < 10; i++) {
        HakoTimeType prev_time = theWorld.get_world_time_usec();
        HakoTimeType world_time = theWorld.time_begins_to_move(asset_times);
        EXPECT_EQ(prev_time + delta_time_usec, world_time);
    }
    // the world time steps to max
    HakoTimeType world_time = theWorld.get_world_time_usec();
    EXPECT_EQ(max_delay_time_usec, world_time);

    // then can not step foward
    HakoTimeType prev_time = theWorld.get_world_time_usec();
    world_time = theWorld.time_begins_to_move(asset_times);
    EXPECT_EQ(prev_time, world_time);
    EXPECT_EQ(delta_time_usec, theWorld.get_wait_time_usec());

    // two assets stepped
    asset_times[0] = delta_time_usec;
    asset_times[1] = delta_time_usec;

    // then step foward
    prev_time = theWorld.get_world_time_usec();
    world_time = theWorld.time_begins_to_move(asset_times);
    EXPECT_EQ(prev_time + delta_time_usec, world_time);
    EXPECT_EQ(delta_time_usec, theWorld.get_wait_time_usec());
}
