#include "pch.h"

#include "Game/QuestResolver.h"
#include "Presence/EngineText.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
	inline constexpr auto kNoObjectiveIndex = std::numeric_limits<std::uint16_t>::max();

	struct QuestCandidate
	{
		RE::TESQuest*                quest;
		const RE::BGSQuestObjective* displayedObjective;
		std::uint32_t                formID;
		std::uint32_t                instanceID;
		std::int8_t                  priority;
	};

	// the engine writes this in place of an Alias/Global tag it cannot resolve
	[[nodiscard]] std::string_view InvalidTagPlaceholder()
	{
		const auto settings = RE::GameSettingCollection::GetSingleton();
		const auto setting = settings ? settings->GetSetting("sInvalidTagString") : nullptr;
		return setting && setting->GetType() == RE::Setting::SETTING_TYPE::kString ? setting->GetString() : std::string_view{};
	}

	// same substitution the Pip-Boy and HUD apply to quest names and objectives
	[[nodiscard]] std::string ExpandQuestText(std::string_view a_source, const RE::TESQuest& a_quest, std::uint32_t a_instanceID)
	{
		RE::BSString text;
		if (a_source.empty() || !text.Set(a_source.data(), a_source.size()))
		{
			return {};
		}

		RE::BGSQuestInstanceText::ParseString(&text, &a_quest, a_instanceID);
		auto result = text.data() ? std::string{ text.data(), text.size() } : std::string{};
		Presence::StripUnresolvedTags(result, InvalidTagPlaceholder());
		return result;
	}

	[[nodiscard]] std::string_view ToView(const char* a_value) noexcept
	{
		return a_value ? std::string_view{ a_value } : std::string_view{};
	}
}

namespace Game
{
	QuestDetails QuestResolver::Resolve(RE::PlayerCharacter* a_player)
	{
		std::vector<QuestCandidate> candidates;
		if (a_player)
		{
			for (const auto& info : a_player->objectives)
			{
				const auto objective = info.objective;
				if (!objective || !objective->ownerQuest)
				{
					continue;
				}

				const auto quest = objective->ownerQuest;
				if (!quest->GetActive() || quest->currentInstanceID != info.instanceID)
				{
					continue;
				}

				const auto formID = quest->GetFormID();
				auto       candidate = std::find_if(candidates.begin(), candidates.end(), [formID, instanceID = info.instanceID](const auto& a_candidate) {
					return a_candidate.formID == formID && a_candidate.instanceID == instanceID;
				});

				if (candidate == candidates.end())
				{
					candidate = candidates.emplace(candidates.end(), QuestCandidate{
																		 .quest = quest,
																		 .displayedObjective = nullptr,
																		 .formID = formID,
																		 .instanceID = info.instanceID,
																		 .priority = quest->data.priority });
				}
				if (info.enstanceState.get() == RE::QUEST_OBJECTIVE_STATE::kDisplayed &&
					(!candidate->displayedObjective || objective->index < candidate->displayedObjective->index))
				{
					candidate->displayedObjective = objective;
				}
			}
		}

		const QuestCandidate* winner = nullptr;
		for (const auto& candidate : candidates)
		{
			if (!candidate.displayedObjective)
			{
				continue;
			}

			if (!winner || static_cast<int>(candidate.priority) > static_cast<int>(winner->priority))
			{
				winner = &candidate;
			}
		}

		if (!winner)
		{
			Invalidate();
			return cachedDetails_;
		}

		const auto     objective = winner->displayedObjective;
		const Identity identity{
			.questFormID = winner->formID,
			.instanceID = winner->instanceID,
			.objectiveIndex = objective ? objective->index : kNoObjectiveIndex
		};

		if (!cachedIdentity_ || *cachedIdentity_ != identity)
		{
			cachedDetails_ = QuestDetails{
				.title = ExpandQuestText(ToView(winner->quest->GetFullName()), *winner->quest, winner->instanceID),
				.objective = objective ?
				                 ExpandQuestText({ objective->displayText.QString(), objective->displayText.QLength() }, *winner->quest, winner->instanceID) :
				                 std::string{},
				.formID = winner->formID,
				.instanceID = winner->instanceID,
				.objectiveIndex = objective ? objective->index : kNoObjectiveIndex,
				.priority = winner->priority,
				.hasQuest = true,
				.hasObjective = objective != nullptr
			};
			cachedIdentity_ = identity;
		}
		else
		{
			cachedDetails_.priority = winner->priority;
		}

		return cachedDetails_;
	}

	void QuestResolver::Invalidate()
	{
		cachedIdentity_.reset();
		cachedDetails_ = {};
	}
}
