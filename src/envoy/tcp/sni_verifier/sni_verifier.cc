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

#include "extensions/filters/network/network_level_sni_reader/network_level_sni_reader.h"

#include "envoy/buffer/buffer.h"
#include "envoy/common/exception.h"
#include "envoy/network/connection.h"
#include "envoy/stats/scope.h"

#include "common/common/assert.h"

#include "openssl/bytestring.h"
#include "openssl/ssl.h"

namespace Envoy {
namespace Tcp {
namespace SniVerifier {

thread_local uint8_t NetworkLevelSniReaderFilter::buf_
    [Extensions::ListenerFilters::TlsInspector::Config::TLS_MAX_CLIENT_HELLO];

NetworkLevelSniReaderFilter::NetworkLevelSniReaderFilter(
    const Extensions::ListenerFilters::TlsInspector::ConfigSharedPtr config)
    : config_(config), ssl_(config_->newSsl()) {
  Extensions::ListenerFilters::TlsInspector::Filter::initializeSsl(
      config->maxClientHelloSize(), sizeof(buf_), ssl_,
      static_cast<Extensions::ListenerFilters::TlsInspector::TlsFilterBase*>(this));
}

Network::FilterStatus NetworkLevelSniReaderFilter::onData(Buffer::Instance& data, bool) {
  ENVOY_CONN_LOG(trace, "NetworkLevelSniReader: got {} bytes", read_callbacks_->connection(),
                 data.length());
  if (done_) {
    return Network::FilterStatus::Continue;
  }

  size_t freeSpaceInBuf = sizeof(buf_) - read_;
  size_t lenToRead = (data.length() < freeSpaceInBuf) ? data.length() : freeSpaceInBuf;
  uint8_t* bufToParse = buf_ + read_;
  data.copyOut(0, lenToRead, bufToParse);
  read_ += lenToRead;
  Extensions::ListenerFilters::TlsInspector::Filter::parseClientHello(
      bufToParse, lenToRead, ssl_, read_, config_->maxClientHelloSize(), config_->stats(),
      [&](bool success) -> void { done(success); }, alpn_found_, clienthello_success_,
      []() -> void {});

  return done_ ? Network::FilterStatus::Continue : Network::FilterStatus::StopIteration;
}

void NetworkLevelSniReaderFilter::onServername(absl::string_view servername) {
  ENVOY_CONN_LOG(debug, "network level sni reader: servername: {}", read_callbacks_->connection(),
                 servername);
  Extensions::ListenerFilters::TlsInspector::Filter::doOnServername(
      servername, config_->stats(),
      [&](absl::string_view name) -> void {
        read_callbacks_->networkLevelRequestedServerName(name);
      },
      clienthello_success_);
}

void NetworkLevelSniReaderFilter::done(bool success) {
  ENVOY_LOG(trace, "network level sni reader: done: {}", success);
  done_ = true;
  if (success) {
    read_callbacks_->continueReading();
  }
}

} // namespace NetworkLevelSniReader
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
