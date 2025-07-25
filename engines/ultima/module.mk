MODULE := engines/ultima

MODULE_OBJS := \
	metaengine.o \
	shared/conf/xml_node.o \
	shared/conf/xml_tree.o \
	shared/engine/data_archive.o \
	shared/engine/events.o \
	shared/std/string.o

ifdef ENABLE_ULTIMA1
MODULE_OBJS += \
	shared/actions/action.o \
	shared/actions/huh.o \
	shared/actions/pass.o \
	shared/core/base_object.o \
	shared/core/character.o \
	shared/core/file.o \
	shared/core/lzw.o \
	shared/core/map.o \
	shared/core/message_target.o \
	shared/core/mouse_cursor.o \
	shared/core/named_item.o \
	shared/core/party.o \
	shared/core/str.o \
	shared/core/tree_item.o \
	shared/core/utils.o \
	shared/core/widgets.o \
	shared/early/font_resources.o \
	shared/early/game.o \
	shared/early/game_base.o \
	shared/early/ultima_early.o \
	shared/engine/input_handler.o \
	shared/engine/input_translator.o \
	shared/engine/messages.o \
	shared/engine/resources.o \
	shared/gfx/bitmap.o \
	shared/gfx/character_input.o \
	shared/gfx/dungeon_surface.o \
	shared/gfx/font.o \
	shared/gfx/info.o \
	shared/gfx/popup.o \
	shared/gfx/screen.o \
	shared/gfx/sprites.o \
	shared/gfx/text_cursor.o \
	shared/gfx/text_input.o \
	shared/gfx/viewport_dungeon.o \
	shared/gfx/viewport_map.o \
	shared/gfx/visual_container.o \
	shared/gfx/visual_item.o \
	shared/gfx/visual_surface.o \
	shared/maps/map.o \
	shared/maps/map_base.o \
	shared/maps/map_tile.o \
	shared/maps/map_widget.o \
	shared/maps/creature.o \
	ultima0/core/resources.o \
	ultima0/game.o \
	ultima0/resources.o \
	ultima1/actions/action.o \
	ultima1/actions/attack.o \
	ultima1/actions/move.o \
	ultima1/actions/quit.o \
	ultima1/actions/ready.o \
	ultima1/actions/stats.o \
	ultima1/core/debugger.o \
	ultima1/core/party.o \
	ultima1/core/quests.o \
	ultima1/core/resources.o \
	ultima1/maps/map.o \
	ultima1/maps/map_base.o \
	ultima1/maps/map_city_castle.o \
	ultima1/maps/map_dungeon.o \
	ultima1/maps/map_overworld.o \
	ultima1/maps/map_tile.o \
	ultima1/spells/blink.o \
	ultima1/spells/create.o \
	ultima1/spells/destroy.o \
	ultima1/spells/kill_magic_missile.o \
	ultima1/spells/ladder_down.o \
	ultima1/spells/ladder_up.o \
	ultima1/spells/open_unlock.o \
	ultima1/spells/prayer.o \
	ultima1/spells/spell.o \
	ultima1/spells/steal.o \
	ultima1/u1dialogs/armoury.o \
	ultima1/u1dialogs/buy_sell_dialog.o \
	ultima1/u1dialogs/combat.o \
	ultima1/u1dialogs/dialog.o \
	ultima1/u1dialogs/drop.o \
	ultima1/u1dialogs/full_screen_dialog.o \
	ultima1/u1dialogs/grocery.o \
	ultima1/u1dialogs/king.o \
	ultima1/u1dialogs/magic.o \
	ultima1/u1dialogs/ready.o \
	ultima1/u1dialogs/stats.o \
	ultima1/u1dialogs/tavern.o \
	ultima1/u1dialogs/transports.o \
	ultima1/u1dialogs/weaponry.o \
	ultima1/u1gfx/drawing_support.o \
	ultima1/u1gfx/info.o \
	ultima1/u1gfx/sprites.o \
	ultima1/u1gfx/status.o \
	ultima1/u1gfx/text_cursor.o \
	ultima1/u1gfx/view_char_gen.o \
	ultima1/u1gfx/view_game.o \
	ultima1/u1gfx/view_title.o \
	ultima1/u1gfx/viewport_dungeon.o \
	ultima1/u1gfx/viewport_map.o \
	ultima1/u6gfx/game_view.o \
	ultima1/widgets/attack_effect.o \
	ultima1/widgets/bard.o \
	ultima1/widgets/dungeon_chest.o \
	ultima1/widgets/dungeon_coffin.o \
	ultima1/widgets/dungeon_item.o \
	ultima1/widgets/dungeon_monster.o \
	ultima1/widgets/dungeon_player.o \
	ultima1/widgets/dungeon_widget.o \
	ultima1/widgets/guard.o \
	ultima1/widgets/king.o \
	ultima1/widgets/merchant.o \
	ultima1/widgets/merchant_armour.o \
	ultima1/widgets/merchant_grocer.o \
	ultima1/widgets/merchant_magic.o \
	ultima1/widgets/merchant_tavern.o \
	ultima1/widgets/merchant_transport.o \
	ultima1/widgets/merchant_weapons.o \
	ultima1/widgets/overworld_monster.o \
	ultima1/widgets/overworld_widget.o \
	ultima1/widgets/person.o \
	ultima1/widgets/princess.o \
	ultima1/widgets/transport.o \
	ultima1/widgets/urban_player.o \
	ultima1/widgets/urban_widget.o \
	ultima1/widgets/wench.o \
	ultima1/game.o
endif

ifdef ENABLE_ULTIMA4
MODULE_OBJS += \
	ultima4/controllers/alpha_action_controller.o \
	ultima4/controllers/camp_controller.o \
	ultima4/controllers/combat_controller.o \
	ultima4/controllers/controller.o \
	ultima4/controllers/game_controller.o \
	ultima4/controllers/inn_controller.o \
	ultima4/controllers/intro_controller.o \
	ultima4/controllers/key_handler_controller.o \
	ultima4/controllers/menu_controller.o \
	ultima4/controllers/read_choice_controller.o \
	ultima4/controllers/read_dir_controller.o \
	ultima4/controllers/read_int_controller.o \
	ultima4/controllers/read_player_controller.o \
	ultima4/controllers/read_string_controller.o \
	ultima4/controllers/reagents_menu_controller.o \
	ultima4/controllers/wait_controller.o \
	ultima4/controllers/ztats_controller.o \
	ultima4/conversation/conversation.o \
	ultima4/conversation/dialogueloader.o \
	ultima4/conversation/dialogueloader_hw.o \
	ultima4/conversation/dialogueloader_lb.o \
	ultima4/conversation/dialogueloader_tlk.o \
	ultima4/core/lzw/hash.o \
	ultima4/core/lzw/lzw.o \
	ultima4/core/lzw/u4decode.o \
	ultima4/core/config.o \
	ultima4/core/debugger.o \
	ultima4/core/debugger_actions.o \
	ultima4/core/settings.o \
	ultima4/core/utils.o \
	ultima4/events/event_handler.o \
	ultima4/events/timed_event_mgr.o \
	ultima4/filesys/rle.o \
	ultima4/filesys/savegame.o \
	ultima4/filesys/u4file.o \
	ultima4/game/armor.o \
	ultima4/game/aura.o \
	ultima4/game/codex.o \
	ultima4/game/context.o \
	ultima4/game/creature.o \
	ultima4/game/death.o \
	ultima4/game/game.o \
	ultima4/game/item.o \
	ultima4/game/moongate.o \
	ultima4/game/names.o \
	ultima4/game/object.o \
	ultima4/game/person.o \
	ultima4/game/player.o \
	ultima4/game/portal.o \
	ultima4/game/script.o \
	ultima4/game/spell.o \
	ultima4/game/weapon.o \
	ultima4/gfx/image.o \
	ultima4/gfx/imageloader.o \
	ultima4/gfx/imageloader_fmtowns.o \
	ultima4/gfx/imageloader_u4.o \
	ultima4/gfx/imagemgr.o \
	ultima4/gfx/scale.o \
	ultima4/gfx/screen.o \
	ultima4/map/annotation.o \
	ultima4/map/city.o \
	ultima4/map/direction.o \
	ultima4/map/dungeon.o \
	ultima4/map/map_tile.o \
	ultima4/map/movement.o \
	ultima4/map/shrine.o \
	ultima4/map/location.o \
	ultima4/map/map.o \
	ultima4/map/maploader.o \
	ultima4/map/mapmgr.o \
	ultima4/map/tile.o \
	ultima4/map/tileanim.o \
	ultima4/map/tilemap.o \
	ultima4/map/tileset.o \
	ultima4/sound/music.o \
	ultima4/sound/sound.o \
	ultima4/views/dungeonview.o \
	ultima4/views/imageview.o \
	ultima4/views/menu.o \
	ultima4/views/menuitem.o \
	ultima4/views/stats.o \
	ultima4/views/textview.o \
	ultima4/views/tileview.o \
	ultima4/views/view.o \
	ultima4/metaengine.o \
	ultima4/ultima4.o
endif

ifdef ENABLE_ULTIMA6
MODULE_OBJS += \
	nuvie/metaengine.o \
	nuvie/nuvie.o \
	nuvie/actors/actor.o \
	nuvie/actors/actor_manager.o \
	nuvie/actors/md_actor.o \
	nuvie/actors/se_actor.o \
	nuvie/actors/u6_actor.o \
	nuvie/actors/wou_actor.o \
	nuvie/conf/configuration.o \
	nuvie/conf/misc.o \
	nuvie/core/anim_manager.o \
	nuvie/core/book.o \
	nuvie/core/converse.o \
	nuvie/core/converse_interpret.o \
	nuvie/core/converse_speech.o \
	nuvie/core/cursor.o \
	nuvie/core/debug.o \
	nuvie/core/effect.o \
	nuvie/core/effect_manager.o \
	nuvie/core/egg_manager.o \
	nuvie/core/events.o \
	nuvie/core/game.o \
	nuvie/core/game_clock.o \
	nuvie/core/look.o \
	nuvie/core/magic.o \
	nuvie/core/map.o \
	nuvie/core/nuvie_defs.o \
	nuvie/core/obj.o \
	nuvie/core/obj_manager.o \
	nuvie/core/party.o \
	nuvie/core/player.o \
	nuvie/core/tile_manager.o \
	nuvie/core/timed_event.o \
	nuvie/core/weather.o \
	nuvie/files/utils.o \
	nuvie/files/nuvie_bmp_file.o \
	nuvie/files/nuvie_file_list.o \
	nuvie/files/nuvie_io.o \
	nuvie/files/nuvie_io_file.o \
	nuvie/files/tmx_map.o \
	nuvie/files/u6_bmp.o \
	nuvie/files/u6_lib_n.o \
	nuvie/files/u6_lzw.o \
	nuvie/files/u6_shape.o \
	nuvie/fonts/bmp_font.o \
	nuvie/fonts/conv_font.o \
	nuvie/fonts/font.o \
	nuvie/fonts/font_manager.o \
	nuvie/fonts/u6_font.o \
	nuvie/fonts/wou_font.o \
	nuvie/gui/gui.o \
	nuvie/gui/gui_area.o \
	nuvie/gui/gui_button.o \
	nuvie/gui/gui_console.o \
	nuvie/gui/gui_dialog.o \
	nuvie/gui/gui_drag_manager.o \
	nuvie/gui/gui_font.o \
	nuvie/gui/gui_load_image.o \
	nuvie/gui/gui_scroller.o \
	nuvie/gui/gui_scroll_bar.o \
	nuvie/gui/gui_text.o \
	nuvie/gui/gui_text_input.o \
	nuvie/gui/gui_text_toggle_button.o \
	nuvie/gui/gui_yes_no_dialog.o \
	nuvie/gui/widgets/gui_widget.o \
	nuvie/gui/widgets/background.o \
	nuvie/gui/widgets/command_bar.o \
	nuvie/gui/widgets/command_bar_new_ui.o \
	nuvie/gui/widgets/console.o \
	nuvie/gui/widgets/msg_scroll.o \
	nuvie/gui/widgets/msg_scroll_new_ui.o \
	nuvie/gui/widgets/converse_gump.o \
	nuvie/gui/widgets/converse_gump_wou.o \
	nuvie/gui/widgets/fps_counter.o \
	nuvie/gui/widgets/map_window.o \
	nuvie/keybinding/keys.o \
	nuvie/keybinding/key_actions.o \
	nuvie/keybinding/key_help_dialog.o \
	nuvie/menus/asset_viewer_dialog.o \
	nuvie/menus/audio_dialog.o \
	nuvie/menus/cheats_dialog.o \
	nuvie/menus/gameplay_dialog.o \
	nuvie/menus/game_menu_dialog.o \
	nuvie/menus/input_dialog.o \
	nuvie/menus/video_dialog.o \
	nuvie/misc/sdl_compat.o \
	nuvie/misc/iavl_tree.o \
	nuvie/misc/u6_line_walker.o \
	nuvie/misc/u6_list.o \
	nuvie/misc/u6_misc.o \
	nuvie/pathfinder/actor_path_finder.o \
	nuvie/pathfinder/astar_path.o \
	nuvie/pathfinder/combat_path_finder.o \
	nuvie/pathfinder/dir_finder.o \
	nuvie/pathfinder/party_path_finder.o \
	nuvie/pathfinder/path.o \
	nuvie/pathfinder/path_finder.o \
	nuvie/pathfinder/sched_path_finder.o \
	nuvie/pathfinder/seek_path.o \
	nuvie/pathfinder/u6_astar_path.o \
	nuvie/portraits/portrait.o \
	nuvie/portraits/portrait_md.o \
	nuvie/portraits/portrait_se.o \
	nuvie/portraits/portrait_u6.o \
	nuvie/save/save_game.o \
	nuvie/screen/dither.o \
	nuvie/screen/game_palette.o \
	nuvie/screen/screen.o \
	nuvie/screen/surface.o \
	nuvie/script/script.o \
	nuvie/script/script_actor.o \
	nuvie/script/script_cutscene.o \
	nuvie/sound/adlib_sfx_manager.o \
	nuvie/sound/custom_sfx_manager.o \
	nuvie/sound/mididrv_m_adlib.o \
	nuvie/sound/mididrv_m_mt32.o \
	nuvie/sound/midiparser_m.o \
	nuvie/sound/origin_fx_adib_driver.o \
	nuvie/sound/pc_speaker_sfx_manager.o \
	nuvie/sound/song.o \
	nuvie/sound/song_adplug.o \
	nuvie/sound/song_filename.o \
	nuvie/sound/sound_manager.o \
	nuvie/sound/towns_sfx_manager.o \
	nuvie/sound/adplug/adplug_player.o \
	nuvie/sound/adplug/emu_opl.o \
	nuvie/sound/adplug/fm_opl.o \
	nuvie/sound/adplug/mid.o \
	nuvie/sound/adplug/opl_class.o \
	nuvie/sound/adplug/u6m.o \
	nuvie/sound/decoder/adlib_sfx_stream.o \
	nuvie/sound/decoder/fm_towns_decoder_stream.o \
	nuvie/sound/decoder/pc_speaker.o \
	nuvie/sound/decoder/pc_speaker_stream.o \
	nuvie/sound/decoder/random_collection_audio_stream.o \
	nuvie/sound/decoder/u6_adplug_decoder_stream.o \
	nuvie/usecode/u6_usecode.o \
	nuvie/usecode/usecode.o \
	nuvie/views/actor_view.o \
	nuvie/views/container_view_gump.o \
	nuvie/views/container_widget.o \
	nuvie/views/container_widget_gump.o \
	nuvie/views/doll_view_gump.o \
	nuvie/views/doll_widget.o \
	nuvie/views/draggable_view.o \
	nuvie/views/inventory_view.o \
	nuvie/views/inventory_widget.o \
	nuvie/views/map_editor_view.o \
	nuvie/views/md_sky_strip_widget.o \
	nuvie/views/party_view.o \
	nuvie/views/portrait_view.o \
	nuvie/views/portrait_view_gump.o \
	nuvie/views/scroll_view_gump.o \
	nuvie/views/scroll_widget_gump.o \
	nuvie/views/sign_view_gump.o \
	nuvie/views/spell_view.o \
	nuvie/views/spell_view_gump.o \
	nuvie/views/sun_moon_ribbon.o \
	nuvie/views/sun_moon_strip_widget.o \
	nuvie/views/view.o \
	nuvie/views/view_manager.o
endif

ifdef ENABLE_ULTIMA8
MODULE_OBJS += \
	ultima8/metaengine.o \
	ultima8/ultima8.o \
	ultima8/audio/audio_channel.o \
	ultima8/audio/audio_mixer.o \
	ultima8/audio/audio_process.o \
	ultima8/audio/audio_sample.o \
	ultima8/audio/midi_player.o \
	ultima8/audio/music_flex.o \
	ultima8/audio/music_process.o \
	ultima8/audio/cru_music_process.o \
	ultima8/audio/raw_audio_sample.o \
	ultima8/audio/sonarc_audio_sample.o \
	ultima8/audio/sound_flex.o \
	ultima8/audio/speech_flex.o \
	ultima8/audio/u8_music_process.o \
	ultima8/conf/config_file_manager.o \
	ultima8/convert/convert_shape.o \
	ultima8/convert/u8/convert_shape_u8.o \
	ultima8/convert/crusader/convert_shape_crusader.o \
	ultima8/filesys/archive.o \
	ultima8/filesys/flex_file.o \
	ultima8/filesys/raw_archive.o \
	ultima8/filesys/savegame.o \
	ultima8/filesys/u8_save_file.o \
	ultima8/games/cru_game.o \
	ultima8/games/game.o \
	ultima8/games/game_data.o \
	ultima8/games/game_info.o \
	ultima8/games/start_crusader_process.o \
	ultima8/games/start_u8_process.o \
	ultima8/games/treasure_loader.o \
	ultima8/games/u8_game.o \
	ultima8/gfx/anim_dat.o \
	ultima8/gfx/avi_player.o \
	ultima8/gfx/cycle_process.o \
	ultima8/gfx/frame_id.o \
	ultima8/gfx/fade_to_modal_process.o \
	ultima8/gfx/gump_shape_archive.o \
	ultima8/gfx/inverter_process.o \
	ultima8/gfx/main_shape_archive.o \
	ultima8/gfx/palette.o \
	ultima8/gfx/palette_fader_process.o \
	ultima8/gfx/palette_manager.o \
	ultima8/gfx/raw_shape_frame.o \
	ultima8/gfx/render_surface.o \
	ultima8/gfx/shape.o \
	ultima8/gfx/shape_archive.o \
	ultima8/gfx/shape_frame.o \
	ultima8/gfx/shape_info.o \
	ultima8/gfx/skf_player.o \
	ultima8/gfx/type_flags.o \
	ultima8/gfx/wpn_ovlay_dat.o \
	ultima8/gfx/xform_blend.o \
	ultima8/gfx/fonts/font.o \
	ultima8/gfx/fonts/font_manager.o \
	ultima8/gfx/fonts/font_shape_archive.o \
	ultima8/gfx/fonts/jp_font.o \
	ultima8/gfx/fonts/jp_rendered_text.o \
	ultima8/gfx/fonts/rendered_text.o \
	ultima8/gfx/fonts/shape_font.o \
	ultima8/gfx/fonts/shape_rendered_text.o \
	ultima8/gfx/fonts/ttf_rendered_text.o \
	ultima8/gfx/fonts/tt_font.o \
	ultima8/gumps/ask_gump.o \
	ultima8/gumps/bark_gump.o \
	ultima8/gumps/book_gump.o \
	ultima8/gumps/computer_gump.o \
	ultima8/gumps/container_gump.o \
	ultima8/gumps/credits_gump.o \
	ultima8/gumps/cru_ammo_gump.o \
	ultima8/gumps/cru_credits_gump.o \
	ultima8/gumps/cru_demo_gump.o \
	ultima8/gumps/cru_energy_gump.o \
	ultima8/gumps/cru_health_gump.o \
	ultima8/gumps/cru_inventory_gump.o \
	ultima8/gumps/cru_menu_gump.o \
	ultima8/gumps/cru_pickup_area_gump.o \
	ultima8/gumps/cru_pickup_gump.o \
	ultima8/gumps/cru_stat_gump.o \
	ultima8/gumps/cru_status_gump.o \
	ultima8/gumps/cru_weapon_gump.o \
	ultima8/gumps/desktop_gump.o \
	ultima8/gumps/difficulty_gump.o \
	ultima8/gumps/fast_area_vis_gump.o \
	ultima8/gumps/game_map_gump.o \
	ultima8/gumps/gump.o \
	ultima8/gumps/gump_notify_process.o \
	ultima8/gumps/inverter_gump.o \
	ultima8/gumps/item_relative_gump.o \
	ultima8/gumps/keypad_gump.o \
	ultima8/gumps/main_menu_process.o \
	ultima8/gumps/menu_gump.o \
	ultima8/gumps/message_box_gump.o \
	ultima8/gumps/minimap_gump.o \
	ultima8/gumps/mini_stats_gump.o \
	ultima8/gumps/modal_gump.o \
	ultima8/gumps/movie_gump.o \
	ultima8/gumps/paged_gump.o \
	ultima8/gumps/paperdoll_gump.o \
	ultima8/gumps/quit_gump.o \
	ultima8/gumps/readable_gump.o \
	ultima8/gumps/resizable_gump.o \
	ultima8/gumps/scroll_gump.o \
	ultima8/gumps/shape_viewer_gump.o \
	ultima8/gumps/slider_gump.o \
	ultima8/gumps/target_gump.o \
	ultima8/gumps/translucent_gump.o \
	ultima8/gumps/u8_save_gump.o \
	ultima8/gumps/weasel_dat.o \
	ultima8/gumps/weasel_gump.o \
	ultima8/gumps/widgets/button_widget.o \
	ultima8/gumps/widgets/edit_widget.o \
	ultima8/gumps/widgets/sliding_widget.o \
	ultima8/gumps/widgets/text_widget.o \
	ultima8/kernel/delay_process.o \
	ultima8/kernel/kernel.o \
	ultima8/kernel/mouse.o \
	ultima8/kernel/object.o \
	ultima8/kernel/object_manager.o \
	ultima8/kernel/process.o \
	ultima8/misc/debugger.o \
	ultima8/misc/encoding.o \
	ultima8/misc/id_man.o \
	ultima8/misc/util.o \
	ultima8/usecode/bit_set.o \
	ultima8/usecode/byte_set.o \
	ultima8/usecode/uc_list.o \
	ultima8/usecode/uc_machine.o \
	ultima8/usecode/uc_process.o \
	ultima8/usecode/uc_stack.o \
	ultima8/usecode/usecode.o \
	ultima8/usecode/usecode_flex.o \
	ultima8/world/bobo_boomer_process.o \
	ultima8/world/camera_process.o \
	ultima8/world/container.o \
	ultima8/world/create_item_process.o \
	ultima8/world/crosshair_process.o \
	ultima8/world/current_map.o \
	ultima8/world/damage_info.o \
	ultima8/world/destroy_item_process.o \
	ultima8/world/egg.o \
	ultima8/world/egg_hatcher_process.o \
	ultima8/world/fireball_process.o \
	ultima8/world/fire_type.o \
	ultima8/world/fire_type_table.o \
	ultima8/world/get_object.o \
	ultima8/world/glob_egg.o \
	ultima8/world/gravity_process.o \
	ultima8/world/item.o \
	ultima8/world/item_factory.o \
	ultima8/world/item_selection_process.o \
	ultima8/world/item_sorter.o \
	ultima8/world/map.o \
	ultima8/world/map_glob.o \
	ultima8/world/minimap.o \
	ultima8/world/missile_tracker.o \
	ultima8/world/monster_egg.o \
	ultima8/world/snap_process.o \
	ultima8/world/sort_item.o \
	ultima8/world/split_item_process.o \
	ultima8/world/sprite_process.o \
	ultima8/world/super_sprite_process.o \
	ultima8/world/target_reticle_process.o \
	ultima8/world/teleport_egg.o \
	ultima8/world/world.o \
	ultima8/world/actors/actor.o \
	ultima8/world/actors/actor_anim_process.o \
	ultima8/world/actors/actor_bark_notify_process.o \
	ultima8/world/actors/ambush_process.o \
	ultima8/world/actors/animation.o \
	ultima8/world/actors/animation_tracker.o \
	ultima8/world/actors/anim_action.o \
	ultima8/world/actors/attack_process.o \
	ultima8/world/actors/auto_firer_process.o \
	ultima8/world/actors/avatar_death_process.o \
	ultima8/world/actors/avatar_gravity_process.o \
	ultima8/world/actors/avatar_mover_process.o \
	ultima8/world/actors/battery_charger_process.o \
	ultima8/world/actors/clear_feign_death_process.o \
	ultima8/world/actors/cru_avatar_mover_process.o \
	ultima8/world/actors/cru_healer_process.o \
	ultima8/world/actors/cru_pathfinder_process.o \
	ultima8/world/actors/combat_dat.o \
	ultima8/world/actors/combat_process.o \
	ultima8/world/actors/grant_peace_process.o \
	ultima8/world/actors/guard_process.o \
	ultima8/world/actors/heal_process.o \
	ultima8/world/actors/loiter_process.o \
	ultima8/world/actors/main_actor.o \
	ultima8/world/actors/npc_dat.o \
	ultima8/world/actors/pace_process.o \
	ultima8/world/actors/pathfinder.o \
	ultima8/world/actors/pathfinder_process.o \
	ultima8/world/actors/quick_avatar_mover_process.o \
	ultima8/world/actors/resurrection_process.o \
	ultima8/world/actors/rolling_thunder_process.o \
	ultima8/world/actors/scheduler_process.o \
	ultima8/world/actors/surrender_process.o \
	ultima8/world/actors/targeted_anim_process.o \
	ultima8/world/actors/teleport_to_egg_process.o \
	ultima8/world/actors/u8_avatar_mover_process.o

ifdef USE_IMGUI
MODULE_OBJS += \
	ultima8/debugtools.o
endif

endif

# This module can be built as a plugin
ifeq ($(ENABLE_ULTIMA), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
