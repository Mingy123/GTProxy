#pragma once
#include <eventpp/utilities/argumentadapter.h>
#include <spdlog/spdlog.h>

#include "sub_server_switch.hpp"
#include "../parser/parser.hpp"
#include "../../core/core.hpp"
#include "../../client/client.hpp"

namespace extension::sub_server_switch {
class SubServerSwitchExtension final : public ISubServerSwitchExtension {
    core::Core* core_;

    std::string address_;
    int port_;

public:
    explicit SubServerSwitchExtension(core::Core* core)
        : core_{ core }
        , port_{ -1 }
    {

    }

    ~SubServerSwitchExtension() override = default;

    void init() override
    {
        auto ext { core_->query_extension<IParserExtension>() };
        if (!ext) {
            spdlog::error("SubServerSwitchExtension: IParserExtension not found");
            return;
        }

        core_->get_event_dispatcher().prependListener(
            core::EventType::Connection,
            eventpp::argumentAdapter<void(const core::EventConnection&)>([&](const core::EventConnection& evt)
            {
                if (evt.from != core::EventFrom::FromClient) {
                    return;
                }

                if (address_.empty() || port_ == -1) {
                    return;
                }

                std::ignore = core_->get_client()->connect(address_, port_);

                address_.clear();
                port_ = -1;

                evt.canceled = true;
            })
        );

        ext->get_event_dispatcher().appendListener(
            IParserExtension::EventType::CallFunction,
            eventpp::argumentAdapter<void(const IParserExtension::EventCallFunction&)>(
                [this](const IParserExtension::EventCallFunction& evt)
                {
                    if (evt.from != core::EventFrom::FromServer) {
                        return;
                    }

                    if (evt.get_function_name() != "OnSendToServer") {
                        return;
                    }

                    const packet::Variant evt_variant{ evt.get_args() };
                    std::vector tokenize{ TextParse::tokenize(evt_variant.get(4)) };

                    address_ = tokenize.at(0);
                    port_ = evt_variant.get<int32_t>(1);

                    packet::Variant variant{};
                    variant.add(evt_variant.get(0));
                    variant.add(core_->get_config().get<unsigned int>("server.port"));
                    variant.add(evt_variant.get<int32_t>(2));
                    variant.add(evt_variant.get<int32_t>(3));
                    variant.add(std::format(
                        "127.0.0.1|{}|{}",
                        tokenize.size() == 2
                            ? ""
                            : tokenize.at(1),
                        tokenize.size() == 2
                            ? tokenize.at(1)
                            : tokenize.at(2)
                    ));
                    variant.add(evt_variant.get<int32_t>(5));

                    for (size_t i{ 6 }; i < evt_variant.size(); ++i) {
                        spdlog::info("{}", evt_variant.get(i));
                    }

                    std::ignore = evt.get_target().send_packet(variant.serialize());
                    evt.canceled = true;
                }
            )
        );
    }

    void free() override
    {
        delete this;
    }
};
}
