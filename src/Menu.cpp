#include "Menu.h"

#include "Bonuses.h"
#include "FlickApi.h"
#include "Forms.h"
#include "RevertStance.h"
#include "Settings.h"
#include "Text.h"
#include "Visuals.h"

namespace Menu
{
	namespace
	{
		using RegisterToolFn = void (*)(FUCK::ITool*);

		// FUCK::Connect name: also the name of the translation files (Interface/Translations/StancesNGCombatExpansion_*.txt).
		constexpr auto kPluginName = "StancesNGCombatExpansion";
		// PluginName() of Stances NG's page (ingame-menu.h in its source).
		constexpr auto kStancesPluginName = "StancesNG"sv;
		// Same indent as Stances NG's own sections, so both halves of the page line up.
		constexpr float kIndent = 35.0F;

		bool           connected = false;
		RegisterToolFn originalRegisterTool = nullptr;
		bool           standaloneRegistered = false;
		bool           dirty = false;

		const char* T(const char* a_key) { return Text::Get(a_key); }

		std::string Label(const char* a_key, const char* a_id) { return std::format("{}##snga_{}", T(a_key), a_id); }

		void Tooltip(const char* a_key)
		{
			if (FUCK::IsItemHovered()) {
				FUCK::SetTooltip(T(a_key));
			}
		}

		void MarkChanged()
		{
			dirty = true;
			Bonuses::ApplyToForms();
		}

		void Slider(const char* a_key, const char* a_id, Settings::F32& a_setting, float a_max, const char* a_format, const char* a_tipKey = nullptr)
		{
			float value = a_setting.GetValue();
			if (FUCK::SliderFloat(Label(a_key, a_id).c_str(), &value, 0.0F, a_max, a_format)) {
				a_setting.SetValue(std::clamp(value, 0.0F, a_max));
				MarkChanged();
			}
			Tooltip(a_tipKey ? a_tipKey : std::format("{}_Tip", a_key).c_str());
		}

		// Collapsed by default: 13 sliders per stance would bury the stance's own values.
		void EnemyBonuses(std::size_t a_stance, const char* a_id)
		{
			FUCK::BeginDisabled(!Settings::enemyBonusesEnabled.GetValue());
			if (FUCK::TreeNode(Label("$SNGECE_EnemyBonusHeader", a_id).c_str())) {
				FUCK::TextDisabled("%s", T("$SNGECE_EnemyBonusNote"));
				for (std::size_t type = 0; type < Settings::kEnemyTypeCount; ++type) {
					const auto key = std::format("$SNGECE_Enemy_{}", Settings::kEnemyTypeNames[type]);
					const auto id = std::format("{}_vs_{}", a_id, Settings::kEnemyTypeNames[type]);
					Slider(key.c_str(), id.c_str(), Settings::EnemyBonus(a_stance, type), 100.0F, "%.0f%%", "$SNGECE_EnemyBonus_Tip");
				}
				FUCK::TreePop();
			}
			FUCK::EndDisabled();
		}

		bool Checkbox(const char* a_key, const char* a_id, Settings::Bool& a_setting)
		{
			bool value = a_setting.GetValue();
			const bool changed = FUCK::Checkbox(Label(a_key, a_id).c_str(), &value);
			Tooltip(std::format("{}_Tip", a_key).c_str());
			if (changed) {
				a_setting.SetValue(value);
				MarkChanged();
			}
			return changed;
		}

		void StanceTitle(const char* a_key, const ImVec4& a_color)
		{
			FUCK::Spacing();
			FUCK::TextColored(a_color, "%s", T(a_key));
			Tooltip(std::format("{}_Desc", a_key).c_str());
		}

		void ApplyToPlayer()
		{
			SKSE::GetTaskInterface()->AddTask([] {
				Visuals::Apply();
				Bonuses::SyncPlayer();
				Bonuses::Refresh();
				RevertStance::Check();
			});
		}

		void DrawSettings()
		{
			FUCK::Spacing(2);
			FUCK::SeparatorText(T("$SNGECE_Title"));
			if (Text::NeedsOtherFont()) {
				FUCK::TextDisabled("%s", T("$SNGECE_FontHint"));
			}

			if (!Forms::IsLoaded()) {
				FUCK::TextColored({ 0.9F, 0.4F, 0.3F, 1.0F }, "%s", T("$SNGECE_NotLoaded"));
				return;
			}

			if (FUCK::CollapsingHeader(Label("$SNGECE_Header_Bonuses", "bonuses").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				FUCK::Indent(kIndent);
				if (Checkbox("$SNGECE_Enabled", "enabled", Settings::bonusesEnabled)) {
					ApplyToPlayer();
				}
				if (Checkbox("$SNGECE_MeleeOnly", "melee", Settings::bonusesMeleeOnly)) {
					ApplyToPlayer();
				}
				Checkbox("$SNGECE_EnemyBonuses", "enemy", Settings::enemyBonusesEnabled);
				FUCK::BeginDisabled(!Settings::bonusesEnabled.GetValue());

				StanceTitle("$SNGECE_Stance_Bear", { 0.84F, 0.27F, 0.22F, 1.0F });
				Slider("$SNGECE_AttackDamageBuff", "bear_dmg", Settings::bearAttackDamage, 100.0F, "%.0f%%");
				Slider("$SNGECE_PowerAttackDamageBuff", "bear_pdmg", Settings::bearPowerAttackDamage, 100.0F, "%.0f%%");
				Slider("$SNGECE_IncomingDamageDebuff", "bear_inc", Settings::bearIncomingDamage, 100.0F, "%.0f%%");
				Slider("$SNGECE_AttackSpeedDebuff", "bear_spd", Settings::bearAttackSpeed, Settings::kMaxDebuff, "%.0f%%");
				Slider("$SNGECE_MoveSpeedDebuff", "bear_move", Settings::bearMoveSpeed, Settings::kMaxDebuff, "%.0f");
				Slider("$SNGECE_StaminaRegenDebuff", "bear_stam", Settings::bearStaminaRegen, 100.0F, "%.0f%%");
				EnemyBonuses(0, "bear");

				StanceTitle("$SNGECE_Stance_Wolf", { 0.58F, 0.70F, 0.84F, 1.0F });
				Slider("$SNGECE_AttackDamageDebuff", "wolf_dmg", Settings::wolfAttackDamage, Settings::kMaxDebuff, "%.0f%%");
				Slider("$SNGECE_BlockBuff", "wolf_block", Settings::wolfBlock, 100.0F, "%.0f%%");
				Slider("$SNGECE_StaggerBuff", "wolf_stagger", Settings::wolfStagger, 100.0F, "%.0f%%");
				Slider("$SNGECE_DamageResistBuff", "wolf_res", Settings::wolfDamageResist, 25.0F, "%.1f%%");
				EnemyBonuses(1, "wolf");

				StanceTitle("$SNGECE_Stance_Hawk", { 0.90F, 0.70F, 0.30F, 1.0F });
				Slider("$SNGECE_AttackDamageDebuff", "hawk_dmg", Settings::hawkAttackDamage, Settings::kMaxDebuff, "%.0f%%");
				Slider("$SNGECE_PowerAttackStaminaBuff", "hawk_pstam", Settings::hawkPowerAttackStamina, 100.0F, "%.0f%%");
				Slider("$SNGECE_AttackSpeedBuff", "hawk_spd", Settings::hawkAttackSpeed, 100.0F, "%.0f%%");
				Slider("$SNGECE_MoveSpeedBuff", "hawk_move", Settings::hawkMoveSpeed, 100.0F, "%.0f");
				Slider("$SNGECE_StaminaRegenBuff", "hawk_stam", Settings::hawkStaminaRegen, 100.0F, "%.0f%%");
				EnemyBonuses(2, "hawk");

				FUCK::EndDisabled();
				FUCK::Spacing();
				FUCK::TextDisabled("%s", T("$SNGECE_AppliesOnClose"));
				FUCK::Unindent(kIndent);
			}

			FUCK::Spacing(2);
			if (FUCK::CollapsingHeader(Label("$SNGECE_Header_Options", "options").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				FUCK::Indent(kIndent);
				if (Checkbox("$SNGECE_RevertStance", "revert", Settings::revertStance)) {
					ApplyToPlayer();
				}
				FUCK::Indent(kIndent);
				FUCK::BeginDisabled(!Settings::revertStance.GetValue());
				Checkbox("$SNGECE_RevertLock", "revertlock", Settings::revertStanceLock);
				FUCK::EndDisabled();
				FUCK::Unindent(kIndent);
				if (Checkbox("$SNGECE_FixBaseAttackSpeed", "speedfix", Settings::fixBaseAttackSpeed)) {
					ApplyToPlayer();
				}
				if (Checkbox("$SNGECE_HideStanceVisuals", "hidevis", Settings::hideStanceVisuals)) {
					ApplyToPlayer();
				}
				if (Checkbox("$SNGECE_DebugLog", "debug", Settings::debugLog)) {
					Settings::ApplyLogLevel();
					// Repeat the checks now that their lines get through.
					Bonuses::ApplyToForms();
					SKSE::GetTaskInterface()->AddTask([] { Bonuses::LogApplied(); });
				}
				FUCK::Unindent(kIndent);
			}

			FUCK::Spacing(2);
			if (FUCK::CollapsingHeader(Label("$SNGECE_Header_Indicator", "indicator").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				FUCK::Indent(kIndent);
				Checkbox("$SNGECE_ShowIndicator", "show", Settings::showIndicator);
				FUCK::BeginDisabled(!Settings::showIndicator.GetValue());
				Checkbox("$SNGECE_IndicatorWeaponDrawn", "drawn", Settings::indicatorWeaponDrawnOnly);
				Checkbox("$SNGECE_IndicatorShowNeutral", "neutral", Settings::indicatorShowNeutral);
				float scale = Settings::indicatorScale.GetValue();
				if (FUCK::SliderFloat(Label("$SNGECE_IndicatorScale", "scale").c_str(), &scale, 0.5F, 4.0F, "%.2f")) {
					Settings::indicatorScale.SetValue(std::clamp(scale, 0.5F, 4.0F));
					dirty = true;
				}
				Tooltip("$SNGECE_IndicatorScale_Tip");
				Slider("$SNGECE_IndicatorPosX", "posx", Settings::indicatorPosX, 100.0F, "%.1f%%");
				Slider("$SNGECE_IndicatorPosY", "posy", Settings::indicatorPosY, 100.0F, "%.1f%%");
				FUCK::EndDisabled();
				FUCK::Unindent(kIndent);
			}

			FUCK::Spacing(2);
			if (FUCK::Button(Label("$SNGECE_Reset", "reset").c_str())) {
				Settings::ResetBonuses();
				MarkChanged();
				ApplyToPlayer();
			}
			Tooltip("$SNGECE_Reset_Tip");
		}

		void OnSettingsClose()
		{
			if (!dirty) {
				return;
			}
			dirty = false;
			Settings::Save();
			ApplyToPlayer();
			logger::info("Settings saved to {}", Settings::kFileBase);
		}

		// Stances NG's page with this mod's sections appended; everything else is passed through as is, so FLICK
		// keeps treating it as Stances NG's page (same name, group, sidebar position and user overrides).
		class StancesPage final : public FUCK::ITool
		{
		public:
			FUCK::ITool* inner = nullptr;

			const char* PluginName() const override { return inner->PluginName(); }
			const char* Name() const override { return inner->Name(); }
			const char* Group() const override { return inner->Group(); }
			void        RenderOverlay() override { inner->RenderOverlay(); }
			void OnOpen() override
			{
				inner->OnOpen();
				Text::Refresh();
			}
			bool        OnAsyncInput(const void* a_event) override { return inner->OnAsyncInput(a_event); }
			bool        ShowInSidebar() const override { return inner->ShowInSidebar(); }

			void Draw() override
			{
				// Stances NG draws its page inside a tab bar without tabs, and ImGui's EndTabBar then puts the cursor
				// back at the top of the page. A group measures what was actually drawn and continues below it.
				FUCK::BeginGroup();
				inner->Draw();
				FUCK::EndGroup();
				DrawSettings();
			}

			void OnClose() override
			{
				inner->OnClose();
				OnSettingsClose();
			}
		};

		// Used only when Stances NG's page never registered.
		class ExtensionsPage final : public FUCK::ITool
		{
		public:
			const char* Name() const override { return "Stances NG - Combat Expansion"; }
			void        Draw() override { DrawSettings(); }
			void        OnOpen() override { Text::Refresh(); }
			void        OnClose() override { OnSettingsClose(); }
		};

		StancesPage stancesPage;
		ExtensionsPage   extensionsPage;

		void HookedRegisterTool(FUCK::ITool* a_tool)
		{
			if (a_tool && !stancesPage.inner && a_tool->PluginName() && a_tool->PluginName() == kStancesPluginName) {
				stancesPage.inner = a_tool;
				logger::info("Stances NG page '{}' registered; settings are drawn inside it", a_tool->Name());
				originalRegisterTool(&stancesPage);
				return;
			}
			originalRegisterTool(a_tool);
		}
	}

	bool InstallHook()
	{
		// FUCK::Connect is not used here: it also loads the translations, and FLICK keeps the first value it reads
		// for a key, so loading before the game language is known would pin the English text.
		const auto module = GetModuleHandleW(L"FUCK.dll");
		const auto request = module ? reinterpret_cast<void* (*)()>(GetProcAddress(module, "RequestFUCK")) : nullptr;
		auto*      iface = request ? static_cast<FUCK_Interface*>(request()) : nullptr;
		if (!iface || iface->version < FUCK_API_VERSION) {
			logger::warn("FLICK (FUCK.dll, API {}+) is not available: no settings page and no stance indicator", FUCK_API_VERSION);
			return false;
		}
		originalRegisterTool = iface->RegisterTool;
		const RegisterToolFn hook = &HookedRegisterTool;
		REL::safe_write(reinterpret_cast<std::uintptr_t>(&iface->RegisterTool), &hook, sizeof(hook));
		logger::info("FLICK API {} found, waiting for Stances NG to register its page", iface->version);
		return true;
	}

	bool Connect()
	{
		if (!originalRegisterTool || !FUCK::Connect(kPluginName)) {
			return false;
		}
		connected = true;
		return true;
	}

	void EnsurePage()
	{
		if (!connected || stancesPage.inner || standaloneRegistered) {
			return;
		}
		standaloneRegistered = true;
		FUCK::RegisterTool(&extensionsPage);
		logger::warn("Stances NG's FLICK page was not registered; using a separate 'Stances NG - Combat Expansion' page");
	}

	bool IsConnected()
	{
		return connected;
	}
}
