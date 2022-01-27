#include "gtest/gtest.h"

#include "RenderGraph/Utils/CommandLineFlag.hpp"


int main (int argc, char** argv)
{
    Utils::CommandLineFlag::MatchAll (argc, argv, false);

    ::testing::InitGoogleTest (&argc, argv);
    return RUN_ALL_TESTS ();
}
