#include "../plugin_sdk/plugin_sdk.hpp"
#include "quinn.h"

namespace quinn {

#define Q_DRAW_COLOR (MAKE_COLOR ( 62, 129, 237, 255 ))  //Red Green Blue Alpha
#define W_DRAW_COLOR (MAKE_COLOR ( 227, 203, 20, 255 ))  //Red Green Blue Alpha
#define E_DRAW_COLOR (MAKE_COLOR ( 235, 12, 223, 255 ))  //Red Green Blue Alpha

    script_spell* q = nullptr;
    script_spell* w = nullptr;
    script_spell* e = nullptr;
    script_spell* r = nullptr;
    script_spell* flash = nullptr;

    TreeTab* main_tab = nullptr;

    namespace settings
    {
        namespace draw_settings
        {
            TreeEntry* draw_range_q = nullptr;
            TreeEntry* draw_range_w = nullptr;
            TreeEntry* draw_range_e = nullptr;
        }

        namespace hitchance
        {
            TreeEntry* q = nullptr;
        }

        namespace combo
        {
            TreeEntry* use_q = nullptr;
            TreeEntry* use_w = nullptr;
            TreeEntry* use_e = nullptr;
        }

        namespace harass
        {
            TreeEntry* use_q = nullptr;
            TreeEntry* use_e = nullptr;
            TreeEntry* harass_mana_manager = nullptr;
        }

        namespace laneclear
        {
            TreeEntry* use_q = nullptr;
            TreeEntry* use_e = nullptr;
            TreeEntry* lane_mana_manager = nullptr;
        }

        namespace jungleclear
        {
            TreeEntry* use_q = nullptr;
            TreeEntry* use_e = nullptr;
            TreeEntry* jungle_mana_manager = nullptr;

        }

        namespace antigapclose
        {
            std::map<std::uint32_t, TreeEntry*> champ_gap;
            bool champ_use(game_object_script target)
            {
                auto it = champ_gap.find(target->get_network_id());
                if (it == champ_gap.end())
                    return false;

                return it->second->get_bool();
            }
            TreeEntry* e_anti_gapclose = nullptr;
        }

        hit_chance get_hitchance_by_config(TreeEntry* hit)
        {
            switch (hit->get_int())
            {
            case 0:
                return hit_chance::low;
                break;

            case 1:
                return hit_chance::medium;
                break;

            case 2:
                return hit_chance::high;
                break;

            case 3:
                return hit_chance::very_high;
                break;
            }
            return hit_chance::medium;
        }
    }

    void load()
    {
        q = plugin_sdk->register_spell(spellslot::q, 1010);
        w = plugin_sdk->register_spell(spellslot::w, 2100);
        e = plugin_sdk->register_spell(spellslot::e, 800);
        r = plugin_sdk->register_spell(spellslot::r, 550);

        q->set_skillshot(0.25f, 160, 1150, { collisionable_objects::minions, collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);

        if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
        else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

        main_tab = menu->create_tab("quinn", " FragQuinn");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {
            auto hitchance = main_tab->add_tab(myhero->get_model() + ".hitchance", "Hitchance Settings");
            {
                settings::hitchance::q = hitchance->add_combobox(myhero->get_model() + ".hitchanceQ", "Q Hitchance", { {"Low",nullptr},{"Medium",nullptr },{"High", nullptr},{"Very High",nullptr} }, ((int)(hit_chance::very_high)-3));
                settings::hitchance::q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
            }

            auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo Settings");
            {
                settings::combo::use_q = combo->add_checkbox(myhero->get_model() + ".comboUseQ", "Use Q", true);
                settings::combo::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                settings::combo::use_w = combo->add_checkbox(myhero->get_model() + ".comboUseW", "Use W", true);
                settings::combo::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                settings::combo::use_e = combo->add_checkbox(myhero->get_model() + ".comboUseE", "Use E", true);
                settings::combo::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }

            auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Harass Settings");
            {
                settings::harass::use_q = harass->add_checkbox(myhero->get_model() + ".harassUseQ", "Use Q", true);
                settings::harass::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                settings::harass::use_e = harass->add_checkbox(myhero->get_model() + ".harassUseE", "Use E", true);
                settings::harass::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                auto harass_config = harass->add_tab(myhero->get_model() + ".harassManaManager", "Harass Mana Manager");
                {
                    settings::harass::harass_mana_manager = harass_config->add_slider(myhero->get_model() + ".harassWhenManaPercent", "Use Harass When Mana Is Above in %", 65, 0, 100);
                }
            }

            auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneClear", "Laneclear Settings");
            {
                settings::laneclear::use_q = laneclear->add_checkbox(myhero->get_model() + ".laneClearUseQ", "Use Q", true);
                settings::laneclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                settings::laneclear::use_e = laneclear->add_checkbox(myhero->get_model() + ".laneClearUseE", "Use E", true);
                settings::laneclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                auto lane_mana_config = laneclear->add_tab(myhero->get_model() + ".laneClearManaManager", "Laneclear Mana Manager");
                {
                    settings::laneclear::lane_mana_manager = lane_mana_config->add_slider(myhero->get_model() + ".laneClearSpells", "Use Spellfarm When Mana Is Above in %", 50, 0, 100);
                }
            }

            auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleClear", "Jungleclear Settings");
            {
                settings::jungleclear::use_q = jungleclear->add_checkbox(myhero->get_model() + ".jungleClearUseQ", "Use Q", true);
                settings::jungleclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                settings::jungleclear::use_e = jungleclear->add_checkbox(myhero->get_model() + ".jungleClearUseE", "Use E", true);
                settings::jungleclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                auto jungle_mana_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearManaManager", "Jungleclear Mana Manager");
                {
                    settings::jungleclear::jungle_mana_manager = jungle_mana_config->add_slider(myhero->get_model() + ".jungleClearSpells", "Use Spellfarm When Mana Is Above in %", 50, 0, 100);
                }
            }

            auto antigapclose = main_tab->add_tab(myhero->get_model() + "antiGapClose", "Anti Gapclose Settings");
            {
                settings::antigapclose::e_anti_gapclose = antigapclose->add_checkbox(myhero->get_model() + ".jungleClearUseE", "Use E", true);
                settings::antigapclose::e_anti_gapclose->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                auto champlist = antigapclose->add_tab("champ.list", "Champions Settings"); {
                    for (auto&& enemy : entitylist->get_enemy_heroes())
                    {
                        settings::antigapclose::champ_gap[enemy->get_network_id()] = champlist->add_checkbox(std::to_string(enemy->get_network_id()), enemy->get_model(), true, false);
                        settings::antigapclose::champ_gap[enemy->get_network_id()]->set_texture(enemy->get_square_icon_portrait());
                    }
                }
            }

            auto draw_settings = main_tab->add_tab(myhero->get_model() + ".drawings", "Drawings Settings");
            {
                settings::draw_settings::draw_range_q = draw_settings->add_checkbox(myhero->get_model() + ".drawingQ", "Draw Q range", true);
                settings::draw_settings::draw_range_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                settings::draw_settings::draw_range_w = draw_settings->add_checkbox(myhero->get_model() + ".drawingW", "Draw W range", true);
                settings::draw_settings::draw_range_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                settings::draw_settings::draw_range_e = draw_settings->add_checkbox(myhero->get_model() + ".drawingE", "Draw E range", true);
                settings::draw_settings::draw_range_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());

            }
        }

        antigapcloser::add_event_handler(on_gapcloser);
        event_handler<events::on_update>::add_callback(on_update);
        event_handler<events::on_draw>::add_callback(on_draw);

    }

    void unload()
    {
        plugin_sdk->remove_spell(q);
        plugin_sdk->remove_spell(w);
        plugin_sdk->remove_spell(e);
        plugin_sdk->remove_spell(r);

        if (flash)
            plugin_sdk->remove_spell(flash);

        antigapcloser::remove_event_handler(on_gapcloser);
        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
    }

    std::vector<game_object_script> getMinions()
    {
        auto lane_minions = entitylist->get_enemy_minions();
        lane_minions.erase(std::remove_if(lane_minions.begin(), lane_minions.end(), [](game_object_script x)
            {
                return !x->is_valid_target(q->range());
            }), lane_minions.end());
        std::sort(lane_minions.begin(), lane_minions.end(), [](game_object_script a, game_object_script b)
            {
                return a->get_position().distance(myhero->get_position()) < b->get_position().distance(myhero->get_position());
            });

        std::vector<game_object_script> result;
        if (!lane_minions.empty())
        {
            for (auto& minion : lane_minions)
            {
                result.push_back(minion);
            }
        }
        return result;
    }

    std::vector<game_object_script> getMonster()
    {
        auto monsters = entitylist->get_jugnle_mobs_minions();
        monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
            {
                return !x->is_valid_target(q->range());
            }), monsters.end());
        std::sort(monsters.begin(), monsters.end(), [](game_object_script a, game_object_script b)
            {
                return a->get_max_health() > b->get_max_health();
            });

        std::vector<game_object_script> result;
        if (!monsters.empty())
        {
            for (auto& minion : monsters)
            {
                result.push_back(minion);
            }
        }
        return result;
    }

    void on_draw()
    {
        if (myhero->is_dead()) return;

        // Draw Q range
        if (q->is_ready() && settings::draw_settings::draw_range_q->get_bool())
            draw_manager->add_circle(myhero->get_position(), q->range(), Q_DRAW_COLOR);

        // Draw W range
        if (w->is_ready() && settings::draw_settings::draw_range_w->get_bool())
            draw_manager->add_circle(myhero->get_position(), w->range(), W_DRAW_COLOR);

        // Draw E range
        if (e->is_ready() && settings::draw_settings::draw_range_e->get_bool())
            draw_manager->add_circle(myhero->get_position(), e->range(), E_DRAW_COLOR);

    }

    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
    {
        if (!settings::antigapclose::champ_use(sender)) return;
        if (sender == nullptr) return;
        if (settings::antigapclose::e_anti_gapclose->get_bool() && e->is_ready())
        {
            if (myhero->get_distance(sender) <= e->range())
            {
                if (e->cast(sender)) return;
            }
        }
    }

    void on_update()
    {
        if (myhero->is_dead()) return;
        if (!orbwalker->can_move(0.05f)) return;

        if (orbwalker->harass())
        {
            harass();
        }

        if (orbwalker->combo_mode())
        {
            combo();
        }

        if (orbwalker->lane_clear_mode())
        {
            farm();
        }
    }
  
    void harass()
    {
        if (q->is_ready() && settings::harass::use_q->get_bool() && myhero->get_mana_percent() >= settings::harass::harass_mana_manager->get_int())
        {
            auto target = target_selector->get_target(q->range(), damage_type::physical);
            auto qPrediction = q->get_prediction(target);

            if (target != nullptr)
            {
                q->cast(target, settings::get_hitchance_by_config(settings::hitchance::q));
            }
        }

        if (e->is_ready() && settings::harass::use_e->get_bool() && myhero->get_mana_percent() >= settings::harass::harass_mana_manager->get_int())
        {
            auto target = target_selector->get_target(e->range(), damage_type::physical);

            if (target != nullptr)
            {
                e->cast(target);
            }
        }
    }

    void combo()
    {
        if (q->is_ready() && settings::combo::use_q->get_bool())
        {
            auto target = target_selector->get_target(q->range(), damage_type::physical);
            if (target != nullptr)
            {
                if (!target->has_buff(buff_hash("quinnw")))
                {
                    q->cast(target, settings::get_hitchance_by_config(settings::hitchance::q));
                }
            }
        }

        if (w->is_ready() && settings::combo::use_w->get_bool())
        {
            for (auto& enemy : entitylist->get_enemy_heroes())
            {
                if (!enemy->is_visible())
                {
                    auto pred = w->get_prediction_no_collision(enemy, true, w->range());
                    if (myhero->get_distance(enemy) <= e->range())
                    {
                        if (pred.get_cast_position().is_wall_of_grass())
                        {
                            if (w->cast()) return;
                        }
                        else
                        {
                            if (w->cast()) return;
                        }
                    }
                }
            }
        }

        if (e->is_ready() && settings::combo::use_e->get_bool())
        {
            auto target = target_selector->get_target(e->range(), damage_type::physical);

            if (target != nullptr)
            {
                if (!target->has_buff(buff_hash("quinnw")))
                {
                    const auto slot_items = myhero->has_item(ItemId::Prowlers_Claw);
                    if (slot_items == spellslot::invalid) { e->cast(target); }
                    else if (const auto item_id = myhero->get_item(slot_items)->get_item_id(); myhero->is_item_ready(item_id) && e->is_ready()) {
                        myhero->cast_spell(slot_items, target);
                        scheduler->delay_action(0.100, [target]() {
                            if (e->cast(target)) return;
                            });
                    }
                    else {
                        if (e->cast(target)) return;
                    }
                }
            }
        }
    }

    void farm()
    {
        auto lane_minion = getMinions();
        auto monsters = getMonster();

        for (auto minion : lane_minion)
        {
            if (orbwalker->lane_clear_mode())
            {
                if (settings::laneclear::use_q->get_bool() && q->is_ready())
                {
                    if (myhero->get_distance(minion) <= q->range())
                    {
                        if (q->cast_on_best_farm_position(1)) return;
                    }
                }

                if (settings::laneclear::use_e->get_bool() && e->is_ready())
                {
                    if (myhero->get_distance(minion) <= e->range())
                    {
                        if (e->cast(minion)) return;
                    }
                }
            }
        }

        for (auto monster : monsters)
        {
            if (orbwalker->lane_clear_mode())
            {
                if (settings::jungleclear::use_q->get_bool())
                {
                    if (myhero->get_distance(monster) <= q->range() && q->is_ready())
                    {
                        if (q->cast_on_best_farm_position(1, true)) return;
                    }
                }

                if (settings::jungleclear::use_e->get_bool())
                {
                    if (myhero->get_distance(monster) <= e->range() && e->is_ready())
                    {
                        if (e->cast(monster)) return;
                    }
                }
            }
        }
    }

};
