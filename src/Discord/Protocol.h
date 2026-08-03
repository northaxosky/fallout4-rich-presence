#pragma once

#include "Presence/Activity.h"

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace Discord::Protocol
{
	enum class Opcode : std::uint32_t
	{
		kHandshake = 0,
		kFrame = 1,
		kClose = 2,
		kPing = 3,
		kPong = 4
	};

	struct Frame
	{
		Opcode      opcode;
		std::string payload;
	};

	struct Header
	{
		Opcode        opcode;
		std::uint32_t payloadSize;
	};

	inline constexpr std::size_t kHeaderSize = 8;
	inline constexpr std::size_t kMaxPayloadSize = 64 * 1024;
	inline constexpr std::size_t kTextLimit = 128;
	inline constexpr std::size_t kAssetKeyLimit = 32;

	[[nodiscard]] inline std::array<std::uint8_t, kHeaderSize> EncodeHeader(Opcode a_opcode, std::uint32_t a_payloadSize) noexcept
	{
		const auto encode = [](std::uint32_t a_value, std::uint8_t* a_output) {
			for (std::size_t i = 0; i < sizeof(a_value); ++i)
			{
				a_output[i] = static_cast<std::uint8_t>(a_value >> (i * 8));
			}
		};

		std::array<std::uint8_t, kHeaderSize> header{};
		encode(static_cast<std::uint32_t>(a_opcode), header.data());
		encode(a_payloadSize, header.data() + sizeof(std::uint32_t));
		return header;
	}

	[[nodiscard]] inline Header DecodeHeader(const std::array<std::uint8_t, kHeaderSize>& a_header) noexcept
	{
		const auto decode = [](const std::uint8_t* a_input) {
			std::uint32_t value = 0;
			for (std::size_t i = 0; i < sizeof(value); ++i)
			{
				value |= static_cast<std::uint32_t>(a_input[i]) << (i * 8);
			}
			return value;
		};

		return Header{
			.opcode = static_cast<Opcode>(decode(a_header.data())),
			.payloadSize = decode(a_header.data() + sizeof(std::uint32_t))
		};
	}

	[[nodiscard]] inline std::string TruncateUtf8(std::string_view a_value, std::size_t a_limit)
	{
		if (a_value.size() <= a_limit)
		{
			return std::string{ a_value };
		}

		auto size = a_limit;
		while (size > 0 && (static_cast<unsigned char>(a_value[size]) & 0xC0u) == 0x80u)
		{
			--size;
		}
		return std::string{ a_value.substr(0, size) };
	}

	[[nodiscard]] inline std::string_view StringField(const nlohmann::json& a_message, std::string_view a_key)
	{
		const auto field = a_message.find(a_key);
		return field != a_message.end() && field->is_string() ? field->get_ref<const std::string&>() : std::string_view{};
	}

	[[nodiscard]] inline std::string MakeHandshake(std::string_view a_applicationID)
	{
		const nlohmann::json handshake{
			{ "v", 1 },
			{ "client_id", a_applicationID }
		};
		return handshake.dump();
	}

	[[nodiscard]] inline std::string MakeSetActivity(const Presence::ActivityUpdate& a_update, std::uint32_t a_processID, std::string_view a_nonce)
	{
		nlohmann::json command{
			{ "cmd", "SET_ACTIVITY" },
			{ "args", nlohmann::json::object() },
			{ "nonce", a_nonce }
		};
		auto& args = command["args"];
		args["pid"] = a_processID;

		if (!a_update)
		{
			args["activity"] = nullptr;
			return command.dump();
		}

		const auto&    presence = *a_update;
		nlohmann::json activity = nlohmann::json::object();
		const auto     addString = [&activity](std::string_view a_key, const std::string& a_value, std::size_t a_limit) {
			if (!a_value.empty())
			{
				activity[a_key] = TruncateUtf8(a_value, a_limit);
			}
		};

		addString("details", presence.details, kTextLimit);
		addString("state", presence.state, kTextLimit);

		if (presence.startTimestamp > 0)
		{
			activity["timestamps"]["start"] = presence.startTimestamp;
		}

		nlohmann::json assets = nlohmann::json::object();
		const auto     addAsset = [&assets](std::string_view a_key, const std::string& a_value, std::size_t a_limit) {
			if (!a_value.empty())
			{
				assets[a_key] = TruncateUtf8(a_value, a_limit);
			}
		};
		addAsset("large_image", presence.largeImage, kAssetKeyLimit);
		addAsset("large_text", presence.largeText, kTextLimit);
		addAsset("small_image", presence.smallImage, kAssetKeyLimit);
		addAsset("small_text", presence.smallText, kTextLimit);
		if (!assets.empty())
		{
			activity["assets"] = std::move(assets);
		}

		args["activity"] = std::move(activity);
		return command.dump();
	}
}
