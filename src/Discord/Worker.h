#pragma once

#include <string>

namespace Presence
{
	class Mailbox;
}

namespace Discord
{
	class Worker final
	{
	public:
		[[nodiscard]] static bool Start(Presence::Mailbox& a_mailbox, std::string a_applicationID) noexcept;
	};
}
