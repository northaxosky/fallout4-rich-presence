#pragma once

#include "Discord/Protocol.h"

#include <REX/W32/BASE.h>

#include <chrono>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>

namespace Discord
{
	inline constexpr auto kIoTimeout = std::chrono::seconds{ 10 };

	class IpcError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class IpcClient
	{
	public:
		IpcClient();
		~IpcClient();

		IpcClient(const IpcClient&) = delete;
		IpcClient(IpcClient&&) = delete;
		IpcClient& operator=(const IpcClient&) = delete;
		IpcClient& operator=(IpcClient&&) = delete;

		void Connect(std::string_view a_applicationID, std::chrono::milliseconds a_handshakeTimeout);
		void Close() noexcept;

		void Send(Protocol::Opcode a_opcode, std::string_view a_payload);

		[[nodiscard]] bool TryReceive(Protocol::Frame& a_frame);
		[[nodiscard]] int  PipeIndex() const noexcept;

	private:
		using Clock = std::chrono::steady_clock;
		using Deadline = std::optional<Clock::time_point>;

		void ReadExact(std::span<std::uint8_t> a_output, Deadline a_deadline);
		void WriteExact(std::span<const std::uint8_t> a_input, Deadline a_deadline);
		void Send(Protocol::Opcode a_opcode, std::string_view a_payload, Deadline a_deadline);
		void Handshake(std::string_view a_applicationID, std::chrono::milliseconds a_timeout);

		[[nodiscard]] Protocol::Frame Receive(Deadline a_deadline);
		[[nodiscard]] bool            IsConnected() const noexcept;
		[[nodiscard]] bool            Open(int a_index);

		REX::W32::HANDLE pipe_{ REX::W32::INVALID_HANDLE_VALUE };
		REX::W32::HANDLE ioEvent_{ nullptr };
		int              pipeIndex_{ -1 };
	};
}
