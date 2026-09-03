#pragma once

#include <cstdint>
#include <string>

namespace Presence
{
	class Mailbox;
}

namespace Discord
{
	enum class ConnectionState : std::uint8_t
	{
		kDisabled,
		kConnecting,
		kConnected,
		kFailed
	};

	struct Status
	{
		ConnectionState state{ ConnectionState::kDisabled };
		int             pipeIndex{ -1 };
		std::string     lastError;
		std::uint64_t   sentCount{ 0 };
	};

	namespace Worker
	{
		[[nodiscard]] bool   Start(Presence::Mailbox& a_mailbox, std::string a_applicationID) noexcept;
		[[nodiscard]] Status GetStatus();
	}
}
