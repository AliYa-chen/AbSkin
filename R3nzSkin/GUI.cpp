#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "CheatManager.hpp"
#include "GUI.hpp"
#include "Memory.hpp"
#include "SkinDatabase.hpp"
#include "Utils.hpp"
#include "fnv_hash.hpp"
#include "imgui/imgui.h"

inline static void footer() noexcept
{
	using namespace std::string_literals;
	ImGui::Separator();
	ImGui::textUnformattedCentered(u8"R3大陆镜像站 r3.nbcnm.cn");
	ImGui::textUnformattedCentered(u8"Tik助手 tik.doii.cc");
}

static void changeTurretSkin(const std::int32_t skinId, const std::int32_t team) noexcept
{
	if (skinId == -1)
		return;

	const auto turrets{ cheatManager.memory->turretList };
	const auto playerTeam{ cheatManager.memory->localPlayer->get_team() };

	for (auto i{ 0u }; i < turrets->length; ++i) {
		const auto turret{ turrets->list[i] };
		if (turret->get_team() == team) {
			if (playerTeam == team) {
				turret->get_character_data_stack()->base_skin.skin = skinId * 2;
				turret->get_character_data_stack()->update(true);
			} else {
				turret->get_character_data_stack()->base_skin.skin = skinId * 2 + 1;
				turret->get_character_data_stack()->update(true);
			}
		}
	}
}

void GUI::render() noexcept
{
	const auto player{ cheatManager.memory->localPlayer };
	const auto heroes{ cheatManager.memory->heroList };
	static const auto my_team{ player ? player->get_team() : 100 };
	static int gear{ player ? player->get_character_data_stack()->base_skin.gear : 0 };

	static const auto vector_getter_skin = [](void* vec, std::int32_t idx, const char** out_text) noexcept {
		const auto& vector{ *static_cast<std::vector<SkinDatabase::skin_info>*>(vec) };
		if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
		*out_text = idx == 0 ? u8"默认" : vector.at(idx - 1).skin_name.c_str();
		return true;
	};

	static const auto vector_getter_ward_skin = [](void* vec, std::int32_t idx, const char** out_text) noexcept {
		const auto& vector{ *static_cast<std::vector<std::pair<std::int32_t, const char*>>*>(vec) };
		if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
		*out_text = idx == 0 ? u8"默认" : vector.at(idx - 1).second;
		return true;
	};

	static auto vector_getter_gear = [](void* vec, std::int32_t idx, const char** out_text) noexcept {
		const auto& vector{ *static_cast<std::vector<const char*>*>(vec) };
		if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
		*out_text = vector[idx];
		return true;
	};

	static auto vector_getter_default = [](void* vec, std::int32_t idx, const char** out_text) noexcept {
		const auto& vector{ *static_cast<std::vector<const char*>*>(vec) };
		if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
		*out_text = idx == 0 ? u8"默认" : vector.at(idx - 1);
		return true;
	};

	ImGui::Begin(u8"R3换肤", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::rainbowText();
		if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip)) {
			if (player) {
				if (ImGui::BeginTabItem(u8"皮肤")) {
					auto& values{ cheatManager.database->champions_skins[fnv::hash_runtime(player->get_character_data_stack()->base_skin.model.str)] };
					ImGui::Text(u8"友方皮肤设置:");

					if (ImGui::Combo(u8"当前皮肤", &cheatManager.config->current_combo_skin_index, vector_getter_skin, static_cast<void*>(&values), values.size() + 1))
						if (cheatManager.config->current_combo_skin_index > 0)
							player->change_skin(values[cheatManager.config->current_combo_skin_index - 1].model_name, values[cheatManager.config->current_combo_skin_index - 1].skin_id);
					
					const auto playerHash{ fnv::hash_runtime(player->get_character_data_stack()->base_skin.model.str) };
					if (const auto it{ std::find_if(cheatManager.database->specialSkins.begin(), cheatManager.database->specialSkins.end(),
						[&skin = player->get_character_data_stack()->base_skin.skin, &ph = playerHash](const SkinDatabase::specialSkin& x) noexcept -> bool
						{
							return x.champHash == ph && (x.skinIdStart <= skin && x.skinIdEnd >= skin);
						}) };
						it != cheatManager.database->specialSkins.end())
					{
						const auto stack{ player->get_character_data_stack() };
						gear = stack->base_skin.gear;

						if (ImGui::Combo(u8"当前皮肤", &gear, vector_getter_gear, static_cast<void*>(&it->gears), it->gears.size())) {
							player->get_character_data_stack()->base_skin.gear = static_cast<std::int8_t>(gear);
							player->get_character_data_stack()->update(true);
						}
						ImGui::Separator();
					}

					if (ImGui::Combo(u8"当前守卫", &cheatManager.config->current_combo_ward_index, vector_getter_ward_skin, static_cast<void*>(&cheatManager.database->wards_skins), cheatManager.database->wards_skins.size() + 1))
						cheatManager.config->current_ward_skin_index = cheatManager.config->current_combo_ward_index == 0 ? -1 : cheatManager.database->wards_skins.at(cheatManager.config->current_combo_ward_index - 1).first;
					
					ImGui::Separator();

					ImGui::Text(u8"敌方皮肤设置:");
					std::int32_t last_team{ 0 };
					for (auto i{ 0u }; i < heroes->length; ++i) {
						const auto hero{ heroes->list[i] };

						if (hero == player)
							continue;

						const auto champion_name_hash{ fnv::hash_runtime(hero->get_character_data_stack()->base_skin.model.str) };
						if (champion_name_hash == FNV("PracticeTool_TargetDummy"))
							continue;

						const auto hero_team{ hero->get_team() };
						const auto is_enemy{ hero_team != my_team };

						if (last_team == 0 || hero_team != last_team) {
							if (last_team != 0)
								ImGui::Separator();
							if (is_enemy)
								ImGui::Text(u8" 敌方英雄");
							else
								ImGui::Text(u8" 友方英雄");
							last_team = hero_team;
						}

						auto& config_array{ is_enemy ? cheatManager.config->current_combo_enemy_skin_index : cheatManager.config->current_combo_ally_skin_index };
						const auto config_entry{ config_array.insert({ champion_name_hash, 0 }) };

						std::snprintf(this->str_buffer, sizeof(this->str_buffer), cheatManager.config->heroName ? u8"英雄: [ %s ]##%X" : u8"玩家: [ %s ]##%X", cheatManager.config->heroName ? hero->get_character_data_stack()->base_skin.model.str : hero->get_name()->c_str(), reinterpret_cast<std::uintptr_t>(hero));

						auto& values{ cheatManager.database->champions_skins[champion_name_hash] };
						if (ImGui::Combo(str_buffer, &config_entry.first->second, vector_getter_skin, static_cast<void*>(&values), values.size() + 1))
							if (config_entry.first->second > 0)
								hero->change_skin(values[config_entry.first->second - 1].model_name, values[config_entry.first->second - 1].skin_id);
					}
					
					footer();
					ImGui::EndTabItem();



				}

			}

			//if (ImGui::BeginTabItem(u8"敌方")) {
			//	
			//	footer();
			//	ImGui::EndTabItem();
			//}

			if (ImGui::BeginTabItem(u8"地图")) {
				ImGui::Text(u8"地图皮肤设置:");
				if (ImGui::Combo(u8"小兵", &cheatManager.config->current_combo_minion_index, vector_getter_default, static_cast<void*>(&cheatManager.database->minions_skins), cheatManager.database->minions_skins.size() + 1))
					cheatManager.config->current_minion_skin_index = cheatManager.config->current_combo_minion_index - 1;
				ImGui::Separator();
				if (ImGui::Combo(u8"我方防御塔", &cheatManager.config->current_combo_order_turret_index, vector_getter_default, static_cast<void*>(&cheatManager.database->turret_skins), cheatManager.database->turret_skins.size() + 1))
					changeTurretSkin(cheatManager.config->current_combo_order_turret_index - 1, 100);
				if (ImGui::Combo(u8"敌方防御塔", &cheatManager.config->current_combo_chaos_turret_index, vector_getter_default, static_cast<void*>(&cheatManager.database->turret_skins), cheatManager.database->turret_skins.size() + 1))
					changeTurretSkin(cheatManager.config->current_combo_chaos_turret_index - 1, 200);
				ImGui::Separator();
				ImGui::Text(u8"野区野怪皮肤设置:");
				for (auto& it : cheatManager.database->jungle_mobs_skins) {
					std::snprintf(str_buffer, 256, u8"当前 %s 皮肤", it.name);
					const auto config_entry{ cheatManager.config->current_combo_jungle_mob_skin_index.insert({ it.name_hashes.front(), 0 }) };
					if (ImGui::Combo(str_buffer, &config_entry.first->second, vector_getter_default, static_cast<void*>(&it.skins), it.skins.size() + 1))
						for (const auto& hash : it.name_hashes)
							cheatManager.config->current_combo_jungle_mob_skin_index[hash] = config_entry.first->second;
				}
				footer();
				ImGui::EndTabItem();
			}

			/*if (ImGui::BeginTabItem("Logger")) {
				cheatManager.logger->draw();
				ImGui::EndTabItem();
			}*/

			if (ImGui::BeginTabItem(u8"杂项")) {
				ImGui::hotkey(u8"菜单按键", cheatManager.config->menuKey);
				ImGui::Checkbox(cheatManager.config->heroName ? u8"英雄名字" : "玩家名字", &cheatManager.config->heroName);
				ImGui::Checkbox(u8"炫彩文字", &cheatManager.config->rainbowText);
				ImGui::Checkbox(u8"快捷键换肤", &cheatManager.config->quickSkinChange);
				ImGui::hoverInfo(u8"快捷键换肤可以在不打开菜单的情况下快速更换皮肤.");

				if (cheatManager.config->quickSkinChange) {
					ImGui::Separator();
					ImGui::hotkey(u8"上一个皮肤按键", cheatManager.config->previousSkinKey);
					ImGui::hotkey(u8"下一个皮肤按键", cheatManager.config->nextSkinKey);
					ImGui::Separator();
				}

				if (player)
					ImGui::InputText(u8"修改昵称", player->get_name());

				if (ImGui::Button(u8"除了我方其他都没有皮肤")) {
					for (auto& enemy : cheatManager.config->current_combo_enemy_skin_index)
						enemy.second = 1;

					for (auto& ally : cheatManager.config->current_combo_ally_skin_index)
						ally.second = 1;

					for (auto i{ 0u }; i < heroes->length; ++i) {
						const auto hero{ heroes->list[i] };
						if (hero != player)
							hero->change_skin(hero->get_character_data_stack()->base_skin.model.str, 0);
					}
				} ImGui::hoverInfo(u8"将除我方玩家外的所有英雄的皮肤设置为默认皮肤.");

				if (ImGui::Button(u8"随机皮肤")) {
					for (auto i{ 0u }; i < heroes->length; ++i) {
						const auto hero{ heroes->list[i] };
						const auto championHash{ fnv::hash_runtime(hero->get_character_data_stack()->base_skin.model.str) };
						
						if (championHash == FNV("PracticeTool_TargetDummy"))
							continue;
						
						const auto skinCount{ cheatManager.database->champions_skins[championHash].size() };
						auto& skinDatabase{ cheatManager.database->champions_skins[championHash] };
						auto& config{ (hero->get_team() != my_team) ? cheatManager.config->current_combo_enemy_skin_index : cheatManager.config->current_combo_ally_skin_index };

						if (hero == player) {
							cheatManager.config->current_combo_skin_index = random(1ull, skinCount);
							hero->change_skin(skinDatabase[cheatManager.config->current_combo_skin_index - 1].model_name, skinDatabase[cheatManager.config->current_combo_skin_index - 1].skin_id);
						} else {
							auto& data{ config[championHash] };
							data = random(1ull, skinCount);
							hero->change_skin(skinDatabase[data - 1].model_name, skinDatabase[data - 1].skin_id);
						}
					}
				} ImGui::hoverInfo(u8"随机更改所有英雄的皮肤.");

				ImGui::SliderFloat(u8"字体比例", &cheatManager.config->fontScale, 1.0f, 2.0f, "%.3f");
				if (ImGui::GetIO().FontGlobalScale != cheatManager.config->fontScale) {
					ImGui::GetIO().FontGlobalScale = cheatManager.config->fontScale;
				} ImGui::hoverInfo(u8"更改菜单字体比例.");
				
				if (ImGui::Button(u8"强制关闭"))
					cheatManager.hooks->uninstall();
				ImGui::hoverInfo(u8"您将返回重新连接界面.");
				ImGui::Text("FPS: %.0f FPS", ImGui::GetIO().Framerate);
				footer();
				ImGui::EndTabItem();
			}
		}
	}
	ImGui::End();
}
