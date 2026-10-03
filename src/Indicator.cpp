#include "Indicator.h"

#include "Forms.h"
#include "Settings.h"
#include "Stance.h"
#include "Text.h"

#include "FlickApi.h"

namespace Indicator
{
	namespace
	{
		using Forms::Stance;

		// Diagonal of the diamond and label size at scale 1 on a 1080p screen.
		constexpr float kDiamondSize = 56.0F;
		constexpr float kFontSize = 15.0F;
		constexpr double kPulseSeconds = 0.35;
		// Safety net in case a stance change slips past the effect event; three effect lookups twice a second.
		constexpr double kCheckInterval = 0.5;

		ImVec4 StanceColor(Stance a_stance)
		{
			switch (a_stance) {
			case Stance::kBear:
				return { 0.84F, 0.27F, 0.22F, 1.0F };
			case Stance::kWolf:
				return { 0.58F, 0.70F, 0.84F, 1.0F };
			case Stance::kHawk:
				return { 0.90F, 0.70F, 0.30F, 1.0F };
			default:
				return { 0.70F, 0.70F, 0.70F, 1.0F };
			}
		}

		const char* LabelKey(Stance a_stance)
		{
			switch (a_stance) {
			case Stance::kBear:
				return "$SNGECE_Stance_Bear";
			case Stance::kWolf:
				return "$SNGECE_Stance_Wolf";
			case Stance::kHawk:
				return "$SNGECE_Stance_Hawk";
			default:
				return "$SNGECE_Stance_Neutral";
			}
		}

		ImVec4 WithAlpha(ImVec4 a_color, float a_alpha)
		{
			a_color.w = a_alpha;
			return a_color;
		}

		ImVec4 Lighten(const ImVec4& a_color, float a_amount)
		{
			return { a_color.x + (1.0F - a_color.x) * a_amount, a_color.y + (1.0F - a_color.y) * a_amount,
				a_color.z + (1.0F - a_color.z) * a_amount, a_color.w };
		}

		struct Diamond
		{
			ImVec2 top, right, bottom, left;
		};

		Diamond MakeDiamond(const ImVec2& a_center, float a_half)
		{
			return { { a_center.x, a_center.y - a_half }, { a_center.x + a_half, a_center.y },
				{ a_center.x, a_center.y + a_half }, { a_center.x - a_half, a_center.y } };
		}

		void Fill(const Diamond& a_d, const ImVec4& a_color) { FUCK::DrawQuadFilled(a_d.top, a_d.right, a_d.bottom, a_d.left, a_color); }
		void Outline(const Diamond& a_d, const ImVec4& a_color, float a_thickness) { FUCK::DrawQuad(a_d.top, a_d.right, a_d.bottom, a_d.left, a_color, a_thickness); }

		class Window final : public FUCK::IWindow
		{
		public:
			const char* Id() const override { return "StanceIndicator"; }
			// Not translated: FLICK may use the title as the ImGui window name, which must not change with the language.
			const char* Title() const override { return "Stance Indicator"; }

			bool IsOpen() const override
			{
				const char* reason = HiddenReason();
				// FLICK asks every frame; only a change is worth a log line.
				if (reason != _lastReason.exchange(reason)) {
					logger::debug("Indicator {}{}", reason ? "hidden: " : "shown", reason ? reason : "");
				}
				return reason == nullptr;
			}

			void SetOpen(bool) override {}

			// Pinned: FLICK applies GetDefaultPos every frame for kNoMove windows, so the position follows the
			// settings and never comes from a saved drag.
			FUCK::WindowFlags GetFlags() const override
			{
				using F = FUCK::WindowFlags;
				return F::kNoDecoration | F::kNoBackground | F::kPassInputToGame | F::kAutoResize | F::kCloseOnGameMenu |
				       F::kNoMove;
			}

			ImVec2 GetDefaultSize() const override { return { _size.x > 0.0F ? _size.x : 90.0F, _size.y > 0.0F ? _size.y : 90.0F }; }

			ImVec2 GetDefaultPos() const override
			{
				const auto  display = FUCK::GetDisplaySize();
				const float x = display.x * std::clamp(Settings::indicatorPosX.GetValue(), 0.0F, 100.0F) / 100.0F;
				const float bottom = display.y * std::clamp(Settings::indicatorPosY.GetValue(), 0.0F, 100.0F) / 100.0F;
				return { x, bottom - GetDefaultSize().y };
			}

			void Draw() override
			{
				const double now = FUCK::GetTime();
				CheckStance(now);
				LoadIcons();

				const auto stance = StanceState::GetCurrent();
				if (const auto count = StanceState::GetChangeCount(); count != _lastChangeCount) {
					_lastChangeCount = count;
					_changedAt = now;
				}
				const float fresh = 1.0F - static_cast<float>(std::clamp((now - _changedAt) / kPulseSeconds, 0.0, 1.0));

				const float scale = std::clamp(Settings::indicatorScale.GetValue(), 0.5F, 4.0F) * FUCK::GetGlobalScale();
				const float size = kDiamondSize * scale;
				const float half = size * 0.5F;
				const auto  color = StanceColor(stance);
				const char* label = Text::Get(LabelKey(stance));

				FUCK::PushFont(FUCK::GetFont(FUCK::Font::kRegular), kFontSize * scale);
				const auto  textSize = FUCK::CalcTextSize(label);
				const float width = std::max(size, textSize.x) + 6.0F * scale;
				const float height = size + 4.0F * scale + textSize.y;

				const auto origin = FUCK::GetCursorScreenPos();
				const ImVec2 center{ origin.x + width * 0.5F, origin.y + half + 1.0F * scale };

				Fill(MakeDiamond(center, half), { 0.05F, 0.05F, 0.06F, 0.72F });
				Outline(MakeDiamond(center, half), WithAlpha(color, 0.95F), 2.0F * scale);
				Outline(MakeDiamond(center, half - 4.5F * scale), WithAlpha(color, 0.35F), 1.0F * scale);
				if (fresh > 0.0F) {
					Outline(MakeDiamond(center, half + 4.0F * scale * (1.0F - fresh)), WithAlpha(Lighten(color, 0.4F), 0.7F * fresh), 2.0F * scale);
				}

				if (void* icon = stance == Stance::kNeutral ? nullptr : _icons[Forms::Index(stance)]) {
					const float iconHalf = half * 0.6F;
					FUCK::AddImage((ImTextureID)icon, { center.x - iconHalf, center.y - iconHalf }, { center.x + iconHalf, center.y + iconHalf },
						{ 0, 0 }, { 1, 1 }, WithAlpha(Lighten(color, 0.25F), 1.0F - 0.5F * fresh));
				} else {
					// Neutral, or the icon could not be loaded: a small hollow diamond keeps the badge from looking empty.
					Outline(MakeDiamond(center, half * 0.28F), WithAlpha(color, 0.85F), 1.5F * scale);
				}

				const ImVec2 textPos{ origin.x + (width - textSize.x) * 0.5F, origin.y + size + 4.0F * scale };
				FUCK::SetCursorScreenPos({ textPos.x + scale, textPos.y + scale });
				FUCK::TextColored({ 0.0F, 0.0F, 0.0F, 0.8F }, "%s", label);
				FUCK::SetCursorScreenPos(textPos);
				FUCK::TextColored(Lighten(color, 0.35F), "%s", label);
				FUCK::PopFont();

				FUCK::SetCursorScreenPos(origin);
				FUCK::Dummy({ width, height });
				// Window padding on top of the content; only used to keep the pinned position on screen.
				_size = { width + 16.0F * scale, height + 16.0F * scale };
			}

		private:
			static const char* HiddenReason()
			{
				if (!Settings::showIndicator.GetValue()) {
					return "turned off in the settings";
				}
				if (!Forms::IsLoaded()) {
					return "Stances NG - Combat Expansion.esp or StancesNG.esp is not loaded";
				}
				const auto player = RE::PlayerCharacter::GetSingleton();
				if (!player || !player->Is3DLoaded() || !player->GetParentCell()) {
					return "no game loaded";
				}
				// FLICK hides kCloseOnGameMenu windows while one of these is open (s_closeOnOpen in FUCK-Man.cpp);
				// checked here too only so the log can say why the badge is not on screen.
				static constexpr std::array kGameMenus{ RE::ContainerMenu::MENU_NAME, RE::JournalMenu::MENU_NAME,
					RE::InventoryMenu::MENU_NAME, RE::MapMenu::MENU_NAME, RE::DialogueMenu::MENU_NAME, RE::MagicMenu::MENU_NAME,
					RE::StatsMenu::MENU_NAME, RE::TweenMenu::MENU_NAME, RE::FavoritesMenu::MENU_NAME, RE::MainMenu::MENU_NAME,
					RE::TrainingMenu::MENU_NAME, RE::MessageBoxMenu::MENU_NAME, RE::SleepWaitMenu::MENU_NAME,
					RE::TutorialMenu::MENU_NAME, RE::LoadingMenu::MENU_NAME, RE::LockpickingMenu::MENU_NAME,
					RE::BookMenu::MENU_NAME, RE::RaceSexMenu::MENU_NAME };
				if (const auto ui = RE::UI::GetSingleton()) {
					for (const auto& menu : kGameMenus) {
						if (ui->IsMenuOpen(menu)) {
							return menu.data();
						}
					}
				}
				// Always shown while the FLICK menu is open, so its settings can be tried out.
				if (FUCK::IsMenuOpen()) {
					return nullptr;
				}
				if (Settings::indicatorWeaponDrawnOnly.GetValue() && !player->AsActorState()->IsWeaponDrawn()) {
					return "weapon sheathed (option)";
				}
				if (!Settings::indicatorShowNeutral.GetValue() && StanceState::GetCurrent() == Stance::kNeutral) {
					return "Neutral stance (option)";
				}
				return nullptr;
			}

			void CheckStance(double a_now)
			{
				if (a_now - _lastCheck < kCheckInterval) {
					return;
				}
				_lastCheck = a_now;
				if (StanceState::Query(RE::PlayerCharacter::GetSingleton()) != StanceState::GetCurrent()) {
					StanceState::RequestResync("indicator check");
				}
			}

			// Textures need the renderer, so they are created on the first frame rather than at data load.
			void LoadIcons()
			{
				if (_iconsTried) {
					return;
				}
				_iconsTried = true;
				// This mod's own artwork (art/icons, built by tools/icons): white with alpha, tinted per stance.
				constexpr std::array kPaths{ "Data/SKSE/Plugins/StancesNGCombatExpansion/Icons/bear.png",
					"Data/SKSE/Plugins/StancesNGCombatExpansion/Icons/wolf.png", "Data/SKSE/Plugins/StancesNGCombatExpansion/Icons/hawk.png" };
				for (std::size_t i = 0; i < kPaths.size(); ++i) {
					// Kept for the whole session; released with the process rather than racing FLICK on unload.
					_icons[i] = FUCK::GetInterface()->LoadImage(kPaths[i], false);
					if (!_icons[i]) {
						logger::warn("Could not load {}, the indicator shows the stance name only", kPaths[i]);
					}
				}
			}

			mutable std::atomic<const char*> _lastReason{ "" };
			ImVec2                           _size{};
			void*         _icons[3]{};
			bool          _iconsTried = false;
			std::uint32_t _lastChangeCount = 0;
			double        _changedAt = -1.0;
			double        _lastCheck = 0.0;
		};

		Window window;
	}

	void Register()
	{
		FUCK::RegisterWindow(&window);
		logger::info("Stance indicator registered with FLICK");
	}

}
