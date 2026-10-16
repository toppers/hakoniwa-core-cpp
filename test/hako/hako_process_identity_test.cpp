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

TEST(HakoProcessIdentityTest, PermissionOrLookupFailureRemainsUnknown)
{
    using hako::utils::process_identity::detail::classify_lookup;
    EXPECT_EQ(
        classify_lookup(false, false, false, 123, 0),
        HakoProcessIdentityStatus::Unknown);
}

TEST(HakoProcessIdentityTest, OnlyPositiveMissingEvidenceIsDead)
{
    using hako::utils::process_identity::detail::classify_lookup;
    EXPECT_EQ(
        classify_lookup(true, false, false, 123, 0),
        HakoProcessIdentityStatus::Dead);
    EXPECT_EQ(
        classify_lookup(false, true, false, 123, 123),
        HakoProcessIdentityStatus::Alive);
    EXPECT_EQ(
        classify_lookup(false, true, false, 123, 124),
        HakoProcessIdentityStatus::Dead);
}
