#pragma once

#include <optional>
#include <string>
#include <vector>

namespace vibecomb {

struct Provider {
    std::string name;
    std::string type;
    std::string api_key_ref;
    std::optional<std::string> endpoint;
};

struct ProviderStore {
    explicit ProviderStore(const std::string& data_dir);

    // CRUD operations
    void create(const Provider& provider);
    void remove(const std::string& name);
    void update(const Provider& provider);
    std::optional<Provider> get(const std::string& name) const;
    std::vector<Provider> list() const;
    std::vector<Provider> search(const std::string& query) const;

    // Resolve API key from reference (key:<name>, env:VAR, or file:/path/to/key.txt)
    std::optional<std::string> resolve_api_key(const std::string& ref) const;

private:
    std::string data_dir_;
    std::string providers_dir_;
    std::string keys_dir_;

    void ensure_dir() const;
    std::string provider_path(const std::string& name) const;
    std::string key_path(const std::string& name) const;
    std::string expand_home(const std::string& path) const;
};

}  // namespace vibecomb
