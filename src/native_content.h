#pragma once
namespace carryon::native {
bool initialize(void(__cdecl* log)(const char*));
void configureTransport(bool moveChests,bool inspectObjects=false,bool moveObjects=true,bool moveInteractive=true) noexcept;
}
