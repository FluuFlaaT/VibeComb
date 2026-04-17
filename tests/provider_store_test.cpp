#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include "providers/store.hpp"

namespace fs = std::filesystem;

class ProviderStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = (fs::temp_directory_path() / ("vibecomb_test_" + std::to_string(getpid()))).string();
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    std::string test_dir_;
};

TEST_F(ProviderStoreTest, ExpandHome) {
    // Test that ~ is expanded to HOME
    vibecomb::ProviderStore store("~/test");
    // The store should expand ~ to the home directory
}

TEST_F(ProviderStoreTest, CreateAndGet) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p;
    p.name = "test_provider";
    p.type = "anthropic";
    p.api_key_ref = "my_secret_key";
    p.endpoint = "https://api.anthropic.com";

    store.create(p);

    auto retrieved = store.get("test_provider");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "test_provider");
    EXPECT_EQ(retrieved->type, "anthropic");
    EXPECT_EQ(retrieved->api_key_ref, "key:test_provider");
    EXPECT_EQ(retrieved->endpoint, "https://api.anthropic.com");

    // Verify the actual key can be resolved
    auto resolved = store.resolve_api_key("key:test_provider");
    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, "my_secret_key");
}

TEST_F(ProviderStoreTest, CreateWithoutEndpoint) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p;
    p.name = "no_endpoint";
    p.type = "openai";
    p.api_key_ref = "another_secret_key";

    store.create(p);

    auto retrieved = store.get("no_endpoint");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "no_endpoint");
    EXPECT_EQ(retrieved->api_key_ref, "key:no_endpoint");
    EXPECT_FALSE(retrieved->endpoint.has_value());
}

TEST_F(ProviderStoreTest, CreateDuplicateThrows) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p;
    p.name = "dup";
    p.type = "test";
    p.api_key_ref = "env:KEY";

    store.create(p);

    EXPECT_THROW(store.create(p), std::runtime_error);
}

TEST_F(ProviderStoreTest, Remove) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p;
    p.name = "to_remove";
    p.type = "test";
    p.api_key_ref = "env:KEY";

    store.create(p);
    ASSERT_TRUE(store.get("to_remove").has_value());

    store.remove("to_remove");
    ASSERT_FALSE(store.get("to_remove").has_value());
}

TEST_F(ProviderStoreTest, RemoveNonexistentThrows) {
    vibecomb::ProviderStore store(test_dir_);
    EXPECT_THROW(store.remove("nonexistent"), std::runtime_error);
}

TEST_F(ProviderStoreTest, Update) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p;
    p.name = "update_test";
    p.type = "original_type";
    p.api_key_ref = "original_secret_key";

    store.create(p);

    p.type = "new_type";
    p.api_key_ref = "new_secret_key";
    p.endpoint = "https://new.endpoint.com";
    store.update(p);

    auto retrieved = store.get("update_test");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->type, "new_type");
    EXPECT_EQ(retrieved->api_key_ref, "key:update_test");
    EXPECT_EQ(retrieved->endpoint, "https://new.endpoint.com");

    // Verify the updated key can be resolved
    auto resolved = store.resolve_api_key("key:update_test");
    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, "new_secret_key");
}

TEST_F(ProviderStoreTest, UpdateNonexistentThrows) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p;
    p.name = "nonexistent";
    p.type = "test";
    p.api_key_ref = "env:KEY";

    EXPECT_THROW(store.update(p), std::runtime_error);
}

TEST_F(ProviderStoreTest, List) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p1{"p1", "type1", "env:K1", std::nullopt};
    vibecomb::Provider p2{"p2", "type2", "env:K2", std::nullopt};
    vibecomb::Provider p3{"p3", "type3", "env:K3", std::nullopt};

    store.create(p1);
    store.create(p2);
    store.create(p3);

    auto list = store.list();
    EXPECT_EQ(list.size(), 3);
}

TEST_F(ProviderStoreTest, ListEmpty) {
    vibecomb::ProviderStore store(test_dir_);
    auto list = store.list();
    EXPECT_TRUE(list.empty());
}

TEST_F(ProviderStoreTest, Search) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p1{"anthropic_provider", "type1", "env:K1", std::nullopt};
    vibecomb::Provider p2{"openai_provider", "type2", "env:K2", std::nullopt};
    vibecomb::Provider p3{"claude_provider", "type3", "env:K3", std::nullopt};

    store.create(p1);
    store.create(p2);
    store.create(p3);

    // Search by name - exact match
    auto results = store.search("anthropic");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].name, "anthropic_provider");

    // Search by name partial match
    results = store.search("claude");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].name, "claude_provider");

    // Case insensitive search
    results = store.search("ANTHROPIC");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].name, "anthropic_provider");

    // No results
    results = store.search("nonexistent");
    EXPECT_TRUE(results.empty());
}

TEST_F(ProviderStoreTest, SearchByType) {
    vibecomb::ProviderStore store(test_dir_);

    vibecomb::Provider p1{"provider1", "anthropic", "env:K1", std::nullopt};
    vibecomb::Provider p2{"provider2", "openai", "env:K2", std::nullopt};
    vibecomb::Provider p3{"provider3", "google", "env:K3", std::nullopt};

    store.create(p1);
    store.create(p2);
    store.create(p3);

    auto results = store.search("anthropic");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].type, "anthropic");

    results = store.search("openai");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].type, "openai");
}

TEST_F(ProviderStoreTest, SearchEmpty) {
    vibecomb::ProviderStore store(test_dir_);
    auto results = store.search("test");
    EXPECT_TRUE(results.empty());
}

TEST_F(ProviderStoreTest, ResolveApiKeyEnv) {
    vibecomb::ProviderStore store(test_dir_);
    // Set up test environment variable
    setenv("VIBECOMB_TEST_KEY", "test_secret_key_123", 1);

    auto key = store.resolve_api_key("env:VIBECOMB_TEST_KEY");
    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(*key, "test_secret_key_123");

    unsetenv("VIBECOMB_TEST_KEY");
}

TEST_F(ProviderStoreTest, ResolveApiKeyEnvNotFound) {
    vibecomb::ProviderStore store(test_dir_);
    unsetenv("VIBECOMB_NONEXISTENT_KEY");
    auto key = store.resolve_api_key("env:VIBECOMB_NONEXISTENT_KEY");
    EXPECT_FALSE(key.has_value());
}

TEST_F(ProviderStoreTest, ResolveApiKeyFile) {
    vibecomb::ProviderStore store(test_dir_);
    // Create a temporary file with a test key
    auto temp_file = fs::path(test_dir_) / "test_key.txt";
    std::ofstream out(temp_file);
    out << "file_based_key_456\n";
    out.close();

    auto key = store.resolve_api_key("file:" + temp_file.string());
    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(*key, "file_based_key_456");
}

TEST_F(ProviderStoreTest, ResolveApiKeyFileNotFound) {
    vibecomb::ProviderStore store(test_dir_);
    auto key = store.resolve_api_key("file:/nonexistent/path/key.txt");
    EXPECT_FALSE(key.has_value());
}

TEST_F(ProviderStoreTest, ResolveApiKeyInvalid) {
    vibecomb::ProviderStore store(test_dir_);
    // Invalid format should return nullopt
    auto key = store.resolve_api_key("invalid_key_ref");
    EXPECT_FALSE(key.has_value());
}

TEST_F(ProviderStoreTest, ResolveApiKeyInternal) {
    vibecomb::ProviderStore store(test_dir_);

    // Create a provider with internal key storage
    vibecomb::Provider p;
    p.name = "internal_key_test";
    p.type = "test";
    p.api_key_ref = "my_secret_api_key";
    store.create(p);

    // Resolve the internal key reference
    auto key = store.resolve_api_key("key:internal_key_test");
    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(*key, "my_secret_api_key");
}
