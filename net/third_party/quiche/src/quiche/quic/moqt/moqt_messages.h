// Copyright (c) 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Structured data for message types in draft-ietf-moq-transport-01.

#ifndef QUICHE_QUIC_MOQT_MOQT_MESSAGES_H_
#define QUICHE_QUIC_MOQT_MOQT_MESSAGES_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "quiche/quic/core/quic_time.h"
#include "quiche/quic/core/quic_types.h"
#include "quiche/common/platform/api/quiche_export.h"

namespace moqt {

enum class MoqtVersion : uint64_t {
  kDraft01 = 0xff000001,
  kUnrecognizedVersionForTests = 0xfe0000ff,
};

struct QUICHE_EXPORT MoqtSessionParameters {
  // TODO: support multiple versions.
  MoqtVersion version;
  quic::Perspective perspective;
  bool using_webtrans;
  std::string path;
};

// The maximum length of a message, excluding any OBJECT payload. This prevents
// DoS attack via forcing the parser to buffer a large message (OBJECT payloads
// are not buffered by the parser).
inline constexpr size_t kMaxMessageHeaderSize = 2048;

enum class QUICHE_EXPORT MoqtMessageType : uint64_t {
  kObjectWithPayloadLength = 0x00,
  kObjectWithoutPayloadLength = 0x02,
  kSubscribeRequest = 0x03,
  kSubscribeOk = 0x04,
  kSubscribeError = 0x05,
  kAnnounce = 0x06,
  kAnnounceOk = 0x7,
  kAnnounceError = 0x08,
  kUnannounce = 0x09,
  kUnsubscribe = 0x0a,
  kSubscribeFin = 0x0b,
  kSubscribeRst = 0x0c,
  kGoAway = 0x10,
  kClientSetup = 0x40,
  kServerSetup = 0x41,
};

enum class QUICHE_EXPORT MoqtRole : uint64_t {
  kIngestion = 0x1,
  kDelivery = 0x2,
  kBoth = 0x3,
};

enum class QUICHE_EXPORT MoqtSetupParameter : uint64_t {
  kRole = 0x0,
  kPath = 0x1,
};

enum class QUICHE_EXPORT MoqtTrackRequestParameter : uint64_t {
  // These two should have been deleted in draft-01.
  // kGroupSequence = 0x0,
  // kObjectSequence = 0x1,
  kAuthorizationInfo = 0x2,
};

struct QUICHE_EXPORT MoqtClientSetup {
  std::vector<MoqtVersion> supported_versions;
  absl::optional<MoqtRole> role;
  absl::optional<absl::string_view> path;
};

struct QUICHE_EXPORT MoqtServerSetup {
  MoqtVersion selected_version;
  absl::optional<MoqtRole> role;
};

struct QUICHE_EXPORT MoqtObject {
  uint64_t track_id;
  uint64_t group_sequence;
  uint64_t object_sequence;
  uint64_t object_send_order;
  absl::optional<uint64_t> payload_length;
  // Message also includes the object payload.
};

enum class QUICHE_EXPORT MoqtSubscribeLocationMode : uint64_t {
  kNone = 0x0,
  kAbsolute = 0x1,
  kRelativePrevious = 0x2,
  kRelativeNext = 0x3,
};

// kNone: absl::optional<MoqtSubscribeLocation> is nullopt.
// kAbsolute: absolute = true
// kRelativePrevious: absolute is false; relative_value is negative
// kRelativeNext: absolute is true; relative_value is positive
struct QUICHE_EXPORT MoqtSubscribeLocation {
  MoqtSubscribeLocation(bool is_absolute, uint64_t abs)
      : absolute(is_absolute), absolute_value(abs) {}
  MoqtSubscribeLocation(bool is_absolute, int64_t rel)
      : absolute(is_absolute), relative_value(rel) {}
  bool absolute;
  union {
    uint64_t absolute_value;
    int64_t relative_value;
  };
  bool operator==(const MoqtSubscribeLocation& other) const {
    return absolute == other.absolute &&
           ((absolute && absolute_value == other.absolute_value) ||
            (!absolute && relative_value == other.relative_value));
  }
};

struct QUICHE_EXPORT MoqtSubscribeRequest {
  absl::string_view full_track_name;
  // If the mode is kNone, the these are absl::nullopt.
  absl::optional<MoqtSubscribeLocation> start_group;
  absl::optional<MoqtSubscribeLocation> start_object;
  absl::optional<MoqtSubscribeLocation> end_group;
  absl::optional<MoqtSubscribeLocation> end_object;
  absl::optional<absl::string_view> authorization_info;
};

struct QUICHE_EXPORT MoqtSubscribeOk {
  absl::string_view track_namespace;
  absl::string_view track_name;
  uint64_t track_id;
  // The message uses ms, but expires is in us.
  quic::QuicTimeDelta expires = quic::QuicTimeDelta::FromMilliseconds(0);
};

struct QUICHE_EXPORT MoqtSubscribeError {
  absl::string_view track_namespace;
  absl::string_view track_name;
  uint64_t error_code;
  absl::string_view reason_phrase;
};

struct QUICHE_EXPORT MoqtUnsubscribe {
  absl::string_view track_namespace;
  absl::string_view track_name;
};

struct QUICHE_EXPORT MoqtSubscribeFin {
  absl::string_view track_namespace;
  absl::string_view track_name;
  uint64_t final_group;
  uint64_t final_object;
};

struct QUICHE_EXPORT MoqtSubscribeRst {
  absl::string_view track_namespace;
  absl::string_view track_name;
  uint64_t error_code;
  absl::string_view reason_phrase;
  uint64_t final_group;
  uint64_t final_object;
};

struct QUICHE_EXPORT MoqtAnnounce {
  absl::string_view track_namespace;
  absl::optional<absl::string_view> authorization_info;
};

struct QUICHE_EXPORT MoqtAnnounceOk {
  absl::string_view track_namespace;
};

struct QUICHE_EXPORT MoqtAnnounceError {
  absl::string_view track_namespace;
  uint64_t error_code;
  absl::string_view reason_phrase;
};

struct QUICHE_EXPORT MoqtUnannounce {
  absl::string_view track_namespace;
};

struct QUICHE_EXPORT MoqtGoAway {
  absl::string_view new_session_uri;
};

std::string MoqtMessageTypeToString(MoqtMessageType message_type);

}  // namespace moqt

#endif  // QUICHE_QUIC_MOQT_MOQT_MESSAGES_H_
