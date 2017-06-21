#include "Module.h"

// static
std::map<const std::string, Module::modrec_t> Module::registry;

Module::Module(const char* grp, const char* inst) : group_name(grp), instance_name(inst)
{
    added = add(grp, inst);
}

Module::Module(const char* grp) : group_name(grp)
{
    added = add(grp);
}

Module::~Module()
{
    // remove from the registry
    auto g = registry.find(group_name);
    if(g != registry.end()) {
        if(single) {
            registry.erase(g);
        } else {
            auto i = g->second.map->find(instance_name);
            if(i != g->second.map->end()) {
                g->second.map->erase(i);
            }
        }
    }
}

bool Module::add(const char* group, const char* instance)
{
    auto g = registry.find(group);
    // we have the group entry if it exists
    if(single) {
        if(g == registry.end()) {
            modrec_t m;
            m.module = this;
            m.map = nullptr;
            registry.insert(registry_t::value_type(group, m));
        } else {
            // TODO if it is a duplicate that is an error
            return false;
        }

    } else {
        if(g == registry.end()) {
            // new group entry
            modrec_t m;
            m.map = new instance_map_t;
            m.map->insert(instance_map_t::value_type(instance, this));
            m.module = nullptr;
            registry.insert(registry_t::value_type(group, m));

        } else if(g->second.map != nullptr) {
            // add instance to existing group map
            g->second.map->insert(instance_map_t::value_type(instance, this));

        } else {
            // TODO error was not a map
            return false;
        }
    }
    return true;
}

void Module::broadcast_halt(bool flg)
{
    for(auto& i : registry) {
        // foreach entry in the registry
        auto& x = i.second;
        if(x.map != nullptr) {
            // it is a map of modules in a group
            for(auto& j : *x.map) {
                j.second->on_halt(flg);
            }

        } else if(x.module != nullptr) {
            // it is a single module
            x.module->on_halt(flg);

        } else {
            // TODO something bad happened neoither map nor module is set
        }
    }
}

void Module::broadcast_in_commmand_ctx()
{
    for(auto& i : registry) {
        // foreach entry in the registry
        auto& x = i.second;
        if(x.map != nullptr) {
            // it is a map of modules in a group
            for(auto& j : *x.map) {
                if(j.second->want_command_ctx) {
                    j.second->in_command_ctx();
                }
            }

        } else if(x.module != nullptr) {
            // it is a single module
            if(x.module->want_command_ctx) {
                x.module->in_command_ctx();
            }

        } else {
            // TODO something bad happened neither map nor module is set
        }
    }
}

Module* Module::lookup(const char *group, const char *instance)
{
    auto g = registry.find(group);
    if(g == registry.end()) return nullptr;

    if(g->second.map != nullptr) {
        // it is a group so find the instance in that group
        if(instance != nullptr) {
            auto i = g->second.map->find(instance);
            if(i == g->second.map->end()) return nullptr;
            return i->second;
        }
        return nullptr;

    } else if(g->second.module != nullptr) {
        return g->second.module;
    }

    return nullptr;
}

std::vector<Module*> Module::lookup_group(const char *group)
{
    std::vector<Module*> results;

    auto g = registry.find(group);
    if(g != registry.end() && g->second.map != nullptr) {
        for(auto& i : *g->second.map) {
            // add each module in this group
            results.push_back(i.second);
        }
    }

    return results;
}

std::vector<std::string> Module::print_modules()
{
    std::vector<std::string> l;
    for(auto& i : registry) {
        // foreach entry in the registry
        std::string r(i.first); // group name
        auto& x = i.second;
        if(x.map != nullptr) {
            // it is a map of modules in a group
            r.append(": ");
            for(auto& j : *x.map) {
                r.append(j.first).append(",");
            }
            r.pop_back();
        }
        l.push_back(r);
    }

    return l;
}