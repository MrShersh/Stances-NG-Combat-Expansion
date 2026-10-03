#include "Visuals.h"

#include "Forms.h"
#include "Settings.h"

namespace Visuals
{
	namespace
	{
		struct Original
		{
			RE::TESEffectShader* shader = nullptr;
			RE::BGSArtObject*    iconArt = nullptr;
		};

		std::array<Original, 3> originals{};
		bool                    captured = false;
		std::optional<bool>     applied;
	}

	void Apply()
	{
		if (!Forms::IsLoaded()) {
			return;
		}
		if (!captured) {
			for (std::size_t i = 0; i < 3; ++i) {
				originals[i].shader = Forms::stanceEffects[i]->data.effectShader;
				originals[i].iconArt = Forms::iconEffects[i] ? Forms::iconEffects[i]->data.hitEffectArt : nullptr;
			}
			captured = true;
		}

		const bool hide = Settings::hideStanceVisuals.GetValue();
		if (applied == hide) {
			return;
		}
		applied = hide;
		for (std::size_t i = 0; i < 3; ++i) {
			Forms::stanceEffects[i]->data.effectShader = hide ? nullptr : originals[i].shader;
			if (Forms::iconEffects[i]) {
				Forms::iconEffects[i]->data.hitEffectArt = hide ? nullptr : originals[i].iconArt;
			}
		}
		logger::info("Stances NG switch visuals (body shader, icon above the head) {}", hide ? "hidden" : "shown");
	}
}
