#include <CLI/CLI.hpp>
#include <iostream>
#include <optional>
#include <string>
#include "providers/store.hpp"

namespace vc = vibecomb;

int main(int argc, char** argv) {
    CLI::App app{"VibeComb - AI Coding Tools Config Manager", "vibecomb"};

    std::string data_dir;
    app.add_option("-d,--data-dir", data_dir, "Data directory")->default_val("~/.vibecomb");

    // provider subcommand
    auto provider_cmd = app.add_subcommand("provider", "Manage providers");

    // provider create <name> -t <type> -k <api-key-ref> [-e <endpoint>]
    std::string create_name, create_type, create_api_key_ref, create_endpoint;
    auto create_cmd = provider_cmd->add_subcommand("create", "Create a new provider");
    create_cmd->add_option("name", create_name, "Provider name")->required(true);
    create_cmd->add_option("-t,--type", create_type, "Provider type")->required(true);
    create_cmd->add_option("-k,--api-key", create_api_key_ref, "API key reference (env:VAR or file:/path/to/key.txt)")->required(true);
    create_cmd->add_option("-e,--endpoint", create_endpoint, "Custom endpoint");
    create_cmd->callback([&]() {
        vc::ProviderStore store(data_dir);
        vc::Provider p;
        p.name = create_name;
        p.type = create_type;
        p.api_key_ref = create_api_key_ref;
        if (!create_endpoint.empty()) p.endpoint = create_endpoint;
        store.create(p);
        std::cout << "Provider '" << create_name << "' created.\n";
    });

    // provider remove <name>
    std::string remove_name;
    auto remove_cmd = provider_cmd->add_subcommand("remove", "Remove a provider");
    remove_cmd->add_option("name", remove_name, "Provider name")->required(true);
    remove_cmd->callback([&]() {
        vc::ProviderStore store(data_dir);
        store.remove(remove_name);
        std::cout << "Provider '" << remove_name << "' removed.\n";
    });

    // provider list
    auto list_cmd = provider_cmd->add_subcommand("list", "List all providers");
    list_cmd->callback([&]() {
        vc::ProviderStore store(data_dir);
        auto providers = store.list();
        if (providers.empty()) {
            std::cout << "No providers found.\n";
        } else {
            for (const auto& p : providers) {
                std::cout << p.name << " (" << p.type << ")\n";
            }
        }
    });

    // provider show <name>
    std::string show_name;
    auto show_cmd = provider_cmd->add_subcommand("show", "Show provider details");
    show_cmd->add_option("name", show_name, "Provider name")->required(true);
    show_cmd->callback([&]() {
        vc::ProviderStore store(data_dir);
        auto p = store.get(show_name);
        if (!p) {
            std::cerr << "Provider not found: " << show_name << "\n";
            exit(1);
        }
        std::cout << "Name: " << p->name << "\n";
        std::cout << "Type: " << p->type << "\n";
        std::cout << "API Key Ref: " << p->api_key_ref << "\n";
        if (p->endpoint) {
            std::cout << "Endpoint: " << *p->endpoint << "\n";
        }
    });

    // provider search [query]
    std::string search_query;
    auto search_cmd = provider_cmd->add_subcommand("search", "Search providers");
    search_cmd->add_option("query", search_query, "Search query");
    search_cmd->callback([&]() {
        vc::ProviderStore store(data_dir);
        auto providers = store.search(search_query);
        if (providers.empty()) {
            std::cout << "No providers found matching '" << search_query << "'.\n";
        } else {
            for (const auto& p : providers) {
                std::cout << p.name << " (" << p.type << ")\n";
            }
        }
    });

    // provider edit <name> [-t <type>] [-k <api-key>] [-e <endpoint>]
    std::string edit_name, edit_type, edit_api_key_ref, edit_endpoint;
    auto edit_cmd = provider_cmd->add_subcommand("edit", "Edit a provider");
    edit_cmd->add_option("name", edit_name, "Provider name")->required(true);
    edit_cmd->add_option("-t,--type", edit_type, "Provider type");
    edit_cmd->add_option("-k,--api-key", edit_api_key_ref, "API key reference");
    edit_cmd->add_option("-e,--endpoint", edit_endpoint, "Custom endpoint");
    edit_cmd->callback([&]() {
        vc::ProviderStore store(data_dir);
        auto p = store.get(edit_name);
        if (!p) {
            std::cerr << "Provider not found: " << edit_name << "\n";
            exit(1);
        }
        if (!edit_type.empty()) p->type = edit_type;
        if (!edit_api_key_ref.empty()) p->api_key_ref = edit_api_key_ref;
        if (!edit_endpoint.empty()) p->endpoint = edit_endpoint;
        store.update(*p);
        std::cout << "Provider '" << edit_name << "' updated.\n";
    });

    CLI11_PARSE(app, argc, argv);

    return 0;
}
