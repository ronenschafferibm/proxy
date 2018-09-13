/* Copyright 2018 Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "extensions/filters/listener/tls_inspector/tls_inspector.h"
#include "extensions/filters/network/network_level_sni_reader/network_level_sni_reader.h"
#include "extensions/filters/network/well_known_names.h"

namespace Envoy {
namespace Tcp {
namespace SniVerifier {

/**
 * Config registration for the network level SNI reader filter. @see
 * NamedNetworkFilterConfigFactory.
 */
class NetworkLevelSniReaderConfigFactory
    : public Server::Configuration::NamedNetworkFilterConfigFactory {
public:
  // NamedNetworkFilterConfigFactory
  Network::FilterFactoryCb
  createFilterFactory(const Json::Object&,
                      Server::Configuration::FactoryContext& context) override {
    return createFilterFactoryFromContext(context);
  }

  Network::FilterFactoryCb
  createFilterFactoryFromProto(const Protobuf::Message&,
                               Server::Configuration::FactoryContext& context) override {
    return createFilterFactoryFromContext(context);
  }

  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return ProtobufTypes::MessagePtr{new Envoy::ProtobufWkt::Empty()};
  }

  std::string name() override { return NetworkFilterNames::get().NetworkLevelSniReader; }

private:
  Network::FilterFactoryCb
  createFilterFactoryFromContext(Server::Configuration::FactoryContext& context) {
    Extensions::ListenerFilters::TlsInspector::ConfigSharedPtr filter_config(
        new Extensions::ListenerFilters::TlsInspector::Config(
            context.scope(), Extensions::ListenerFilters::TlsInspector::Config::TLS_MAX_CLIENT_HELLO,
            "network_level_sni_reader."));
    return [filter_config](Network::FilterManager& filter_manager) -> void {
      filter_manager.addReadFilter(std::make_shared<NetworkLevelSniReaderFilter>(filter_config));
    };
  }
};

/**
 * Static registration for the echo filter. @see RegisterFactory.
 */
static Registry::RegisterFactory<NetworkLevelSniReaderConfigFactory,
                                 Server::Configuration::NamedNetworkFilterConfigFactory>
    registered_;

} // namespace NetworkLevelSniReader
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
