#pragma once

// Settings UI inside Stances NG's own FLICK page.
//
// Stances NG registers its page ("StancesNG" tool) with FLICK at kDataLoaded and has no extension point. FLICK passes
// every registration through a function table that plugins share, so at kPostPostLoad (before any kDataLoaded
// handler runs) the mod puts itself in front of RegisterTool: when Stances NG registers its page, FLICK gets a thin
// wrapper that draws Stances NG's page unchanged and this mod's sections below it. Every other tool goes through
// untouched. If Stances NG's page never shows up (FLICK without Stances NG's menu, a renamed page), the mod
// registers a page of its own instead.
namespace Menu
{
	// kPostPostLoad: installs the RegisterTool hook. Returns false without FLICK.
	bool InstallHook();
	// kDataLoaded: connects to FLICK (translations are read now that the game language is known).
	bool Connect();
	// After the first game load: falls back to a standalone page if Stances NG's page was not seen.
	void EnsurePage();
	[[nodiscard]] bool IsConnected();
}
