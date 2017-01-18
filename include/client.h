/* Copyright 2017 Google Inc. All Rights Reserved.
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

#ifndef MIXERCLIENT_CLIENT_H
#define MIXERCLIENT_CLIENT_H

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "google/protobuf/stubs/status.h"
#include "mixer/api/v1/service.pb.h"
#include "options.h"
#include "transport.h"

namespace istio {
namespace mixer_client {

// Defines a function prototype used when an asynchronous transport call
// is completed.
using DoneFunc = std::function<void(const ::google::protobuf::util::Status&)>;

// Defines the options to create an instance of MixerClient interface.
struct MixerClientOptions {
  // Default constructor with default values.
  MixerClientOptions() {}

  // Constructor with specified option values.
  MixerClientOptions(const CheckOptions& check_options,
                     const ReportOptions& report_options,
                     const QuotaOptions& quota_options)
      : check_options(check_options),
        report_options(report_options),
        quota_options(quota_options) {}

  // Check options.
  CheckOptions check_options;

  // Report options.
  ReportOptions report_options;

  // Quota options.
  QuotaOptions quota_options;

  // Transport object.
  TransportInterface* transport;
};

struct Attributes {
  // A structure to hold different types of value.
  struct Value {
    // Data type
    enum ValueType { STRING, INT64, DOUBLE, BOOL, TIME, BYTES } type;

    // Data value
    union {
      std::string str_v;  // for both STRING and BYTES
      int64_t int64_v;
      double double_v;
      bool bool_v;
      std::chrono::time_point<std::chrono::system_clock> time_v;
    } value;
  };

  std::map<std::string, Value> attributes;
};

class MixerClient {
 public:
  // Destructor
  virtual ~MixerClient() {}

  // Attribute based calls will be used.
  // The protobuf message based calls will be removed.
  // Callers should pass in the full set of attributes for the call.
  // The client will use the full set attributes to check cache. If cache
  // miss, an attribute context based on the underline gRPC stream will
  // be used to generate attribute_update and send that to Mixer server.
  // Callers don't need response data, they only need success or failure.
  // The response data from mixer will be consumed by mixer client.
  virtual void Check(const Attributes& attributes, DoneFunc on_done) = 0;
  virtual void Report(const Attributes& attributes, DoneFunc on_done) = 0;
  virtual void Quota(const Attributes& attributes, DoneFunc on_done) = 0;

  // The async call.
  // on_check_done is called with the check status after cached
  // check_response is returned in case of cache hit, otherwise called after
  // check_response is returned from the Controller service.
  //
  // check_response must be alive until on_check_done is called.
  virtual void Check(const ::istio::mixer::v1::CheckRequest& check_request,
                     ::istio::mixer::v1::CheckResponse* check_response,
                     DoneFunc on_check_done) = 0;

  // This is async call. on_report_done is always called when the
  // report request is finished.
  virtual void Report(const ::istio::mixer::v1::ReportRequest& report_request,
                      ::istio::mixer::v1::ReportResponse* report_response,
                      DoneFunc on_report_done) = 0;

  // This is async call. on_quota_done is always called when the
  // quota request is finished.
  virtual void Quota(const ::istio::mixer::v1::QuotaRequest& quota_request,
                     ::istio::mixer::v1::QuotaResponse* quota_response,
                     DoneFunc on_quota_done) = 0;
};

// Creates a MixerClient object.
std::unique_ptr<MixerClient> CreateMixerClient(MixerClientOptions& options);

}  // namespace mixer_client
}  // namespace istio

#endif  // MIXERCLIENT_CLIENT_H