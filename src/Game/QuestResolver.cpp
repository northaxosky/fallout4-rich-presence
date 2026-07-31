#include "pch.h"

#include "Game/QuestResolver.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
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

	[[nodiscard]] std::string CopyString(const char* a_value)
	{
		return a_value ? std::string{ a_value } : std::string{};
	}

	[[nodiscard]] std::string ExpandObjective(const RE::BGSQuestObjective& a_objective, const RE::TESQuest& a_quest, std::uint32_t a_instanceID)
	{
		if (a_objective.displayText.QEmpty())
		{
			return {};
		}

		RE::BSString text;
		if (!text.Set(a_objective.displayText.QString(), a_objective.displayText.QLength()))
		{
			return {};
		}

		RE::BGSQuestInstanceText::ParseString(&text, &a_quest, a_instanceID);
		return text.data() ? std::string{ text.data(), text.size() } : std::string{};
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
				.title = CopyString(winner->quest->GetFullName()),
				.objective = objective ? ExpandObjective(*objective, *winner->quest, winner->instanceID) : std::string{},
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
