#include "providers/store.hpp"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <sys/stat.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fs = std::filesystem;
namespace vibecomb {

ProviderStore::ProviderStore(const std::string& data_dir)
    : data_dir_(expand_home(data_dir)),
      providers_dir_(data_dir_ + "/providers"),
      keys_dir_(data_dir_ + "/providers/keys") {}

std::string ProviderStore::expand_home(const std::string& path) const {
    if (!path.empty() && path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            return std::string(home) + path.substr(1);
        }
    }
    return path;
}

void ProviderStore::ensure_dir() const {
    fs::create_directories(providers_dir_);
    fs::create_directories(keys_dir_);
}

std::string ProviderStore::provider_path(const std::string& name) const {
    return providers_dir_ + "/" + name + ".json";
}

std::string ProviderStore::key_path(const std::string& name) const {
    return keys_dir_ + "/" + name + ".key";
}

void ProviderStore::create(const Provider& provider) {
    ensure_dir();
    auto path = provider_path(provider.name);
    if (fs::exists(path)) {
        throw std::runtime_error("Provider already exists: " + provider.name);
    }

    // 隔离存储 API key
    // 时刻记住保护自己的密钥，不要暴露到公网
    auto key_path_str = key_path(provider.name);
    std::ofstream key_out(key_path_str);
    key_out << provider.api_key_ref;
    key_out.close();
    // Linux 下设置为 0600
    chmod(key_path_str.c_str(), 0600);

    json j;
    j["name"] = provider.name;
    j["type"] = provider.type;
    j["key_ref"] = "key:" + provider.name;
    if (provider.endpoint) {
        j["endpoint"] = *provider.endpoint;
    }
    std::ofstream out(path);
    out << j.dump(2) << "\n";
}

void ProviderStore::remove(const std::string& name) {
    auto path = provider_path(name);
    if (!fs::exists(path)) {
        throw std::runtime_error("Provider not found: " + name);
    }
    fs::remove(path);
    auto key_p = key_path(name);
    if (fs::exists(key_p)) {
        fs::remove(key_p);
    }
}

void ProviderStore::update(const Provider& provider) {
    auto path = provider_path(provider.name);
    if (!fs::exists(path)) {
        throw std::runtime_error("Provider not found: " + provider.name);
    }

    // Update API key file
    auto key_path_str = key_path(provider.name);
    std::ofstream key_out(key_path_str);
    key_out << provider.api_key_ref;
    key_out.close();
    chmod(key_path_str.c_str(), 0600);

    // Update provider JSON
    json j;
    j["name"] = provider.name;
    j["type"] = provider.type;
    j["key_ref"] = "key:" + provider.name;
    if (provider.endpoint) {
        j["endpoint"] = *provider.endpoint;
    }
    std::ofstream out(path);
    out << j.dump(2) << "\n";
}

std::optional<Provider> ProviderStore::get(const std::string& name) const {
    auto path = provider_path(name);
    if (!fs::exists(path)) {
        return std::nullopt;
    }
    std::ifstream in(path);
    json j;
    in >> j;
    Provider p;
    p.name = j.at("name").get<std::string>();
    p.type = j.at("type").get<std::string>();
    p.api_key_ref = j.at("key_ref").get<std::string>();
    if (j.contains("endpoint")) {
        p.endpoint = j.at("endpoint").get<std::string>();
    }
    return p;
}

std::vector<Provider> ProviderStore::list() const {
    std::vector<Provider> result;
    if (!fs::exists(providers_dir_)) {
        return result;
    }
    for (const auto& entry : fs::directory_iterator(providers_dir_)) {
        if (entry.path().extension() == ".json") {
            std::string name = entry.path().stem().string();
            if (auto provider = get(name)) {
                result.push_back(*provider);
            }
        }
    }
    return result;
}

std::vector<Provider> ProviderStore::search(const std::string& query) const {
    auto all = list();
    std::vector<Provider> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    for (const auto& p : all) {
        std::string lower_name = p.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        std::string lower_type = p.type;
        std::transform(lower_type.begin(), lower_type.end(), lower_type.begin(), ::tolower);
        if (lower_name.find(lower_query) != std::string::npos ||
            lower_type.find(lower_query) != std::string::npos) {
            result.push_back(p);
        }
    }
    return result;
}

std::optional<std::string> ProviderStore::resolve_api_key(const std::string& ref) const {
    if (ref.rfind("key:", 0) == 0) {
        // Internal key reference: key:<name>
        std::string key_name = ref.substr(4);
        std::string key_file_path = keys_dir_ + "/" + key_name + ".key";
        std::ifstream in(key_file_path);
        if (!in) return std::nullopt;
        std::string key;
        std::getline(in, key);
        if (!key.empty() && key.back() == '\r') key.pop_back();
        return key;
    }
    if (ref.rfind("env:", 0) == 0) {
        const char* val = std::getenv(ref.substr(4).c_str());
        if (val) return std::string(val);
        return std::nullopt;
    }
    if (ref.rfind("file:", 0) == 0) {
        std::string path = ref.substr(5);
        std::ifstream in(path);
        if (!in) return std::nullopt;
        std::string key;
        std::getline(in, key);
        if (!key.empty() && key.back() == '\r') key.pop_back();
        return key;
    }
    return std::nullopt;
}

}  // namespace vibecomb
