#include "util/FileHash.hpp"
#include "support/TempDir.hpp"
#include <fstream>
#include <gtest/gtest.h>

TEST(FileHash, Sha256OfKnownContent)
{
    TempDir dir;
    const auto path = dir / "f.txt";
    std::ofstream(path) << "abc";
    EXPECT_EQ(FileHash::sha256File(path.string()),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(FileHash, EmptyFile)
{
    TempDir dir;
    const auto path = dir / "empty";
    std::ofstream{path};
    EXPECT_EQ(FileHash::sha256File(path.string()),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(FileHash, MissingFileReturnsEmpty)
{
    EXPECT_EQ(FileHash::sha256File("/nonexistent/shareframe/file"), "");
}
