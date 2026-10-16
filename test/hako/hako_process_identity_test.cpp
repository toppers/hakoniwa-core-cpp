#include <gtest/gtest.h>

#include "utils/hako_share/impl/hako_process_identity.hpp"

TEST(HakoProcessIdentityTest, CurrentProcessTokenIsAlive)
{
    HakoProcessIdentity identity{};
    ASSERT_TRUE(hako_current_process_identity(&identity));
    EXPECT_GT(identity.pid, 0);
    EXPECT_NE(identity.start_token, 0U);
    EXPECT_EQ(
        hako_check_process_identity(identity),
        HakoProcessIdentityStatus::Alive);
}

TEST(HakoProcessIdentityTest, ReusedPidWithDifferentStartTokenIsDead)
{
    HakoProcessIdentity identity{};
    ASSERT_TRUE(hako_current_process_identity(&identity));
    ++identity.start_token;
    EXPECT_EQ(
        hako_check_process_identity(identity),
        HakoProcessIdentityStatus::Dead);
}
