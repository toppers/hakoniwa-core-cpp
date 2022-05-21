#include <gtest/gtest.h>
#include "hako.hpp"

class HakoTest : public ::testing::Test {
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

TEST_F(HakoTest, IHakoMasterController_01)
{
    //prepare
    hako::init();

    HakoTimeType max_delay_usec = 100000ULL;
    HakoTimeType delta_usec = 10000ULL;

    //do
    std::shared_ptr<hako::IHakoMasterController> hako_master = hako::create_master();
    hako_master->set_config_simtime(max_delay_usec, delta_usec);

    EXPECT_EQ(hako_master->get_max_deltay_time_usec(), max_delay_usec);
    EXPECT_EQ(hako_master->get_delta_time_usec(), delta_usec);

    //done
    hako::destroy();
}
static std::shared_ptr<hako::IHakoSimulationEventController> sim_ctrl = nullptr;
static std::shared_ptr<hako::IHakoAssetController> hako_asset = nullptr;

TEST_F(HakoTest, IHakoAssetController_01)
{
    //prepare
    hako::init();

    HakoTimeType max_delay_usec = 100000ULL;
    HakoTimeType delta_usec = 10000ULL;
    std::shared_ptr<hako::IHakoMasterController> hako_master = hako::create_master();
    hako_master->set_config_simtime(max_delay_usec, delta_usec);

    //do
    hako_asset = hako::create_asset_controller();

    AssetCallbackType callback;
    callback.reset = nullptr;
    callback.start = nullptr;
    callback.stop = nullptr;
    auto ret = hako_asset->asset_register("TestAsset", callback);
    EXPECT_TRUE(ret);
    ret = hako_asset->asset_unregister("TestAsset");
    EXPECT_TRUE(ret);

    //done
    hako::destroy();
}
