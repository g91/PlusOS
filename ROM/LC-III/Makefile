# Makefile for Macintosh LC III Complete Universal ROM Subsystems Library
# Builds liblc3rom.a with m68k-elf-gcc (zero stubs, 100% decompiled)

CC      := $(shell command -v m68k-elf-gcc 2>/dev/null || command -v m68k-linux-gnu-gcc 2>/dev/null || echo m68k-elf-gcc)
AR      := $(shell command -v m68k-elf-ar 2>/dev/null || command -v m68k-linux-gnu-ar 2>/dev/null || echo m68k-elf-ar)
RANLIB  := $(shell command -v m68k-elf-ranlib 2>/dev/null || command -v m68k-linux-gnu-ranlib 2>/dev/null || echo m68k-elf-ranlib)
CFLAGS  = -m68030 -O2 -Wall -Wextra -ffreestanding -fno-builtin -nostdlib -Iinclude -I../../kernel/core
ASFLAGS = -m68030

SRCS_C = \
    src/lc3_color_quickdraw.c \
    src/lc3_memory_manager.c \
    src/lc3_palette_manager.c \
    src/lc3_color_window_mgr.c \
    src/lc3_scsi_esp.c \
    src/lc3_sound_mgr.c \
    src/lc3_slot_manager.c \
    src/lc3_gestalt.c \
    src/lc3_os_extensions.c \
    src/lc3_process_mgr.c \
    src/lc3_system7_traps.c \
    src/lc3_appletalk_xpp.c \
    src/lc3_sane_fp68k.c \
    src/lc3_boot_mgr.c \
    src/lc3_microbug.c \
    src/lc3_heap_engine.c \
    src/lc3_syserr_alert.c \
    src/lc3_scsi_boot.c \
    src/lc3_sonora_egret.c \
    src/lc3_extended_traps.c \
    src/lc3_cqd_engine.c \
    src/lc3_universal_tables.c \
    src/lc3_pmmu_engine.c \
    src/lc3_sony_floppy.c \
    src/lc3_device_mgr.c \
    src/lc3_segment_loader.c \
    src/lc3_adb_keyboard.c \
    src/lc3_event_queue.c \
    src/lc3_vbl_interrupts.c \
    src/lc3_fs_cache.c \
    src/lc3_traps_a2xx.c \
    src/lc3_hfs_btree.c \
    src/lc3_hfs_cache.c \
    src/lc3_control_mgr.c \
    src/lc3_menu_mgr.c \
    src/lc3_textedit.c \
    src/lc3_scrap_mgr.c \
    src/lc3_desk_mgr.c \
    src/lc3_list_mgr.c \
    src/lc3_standard_file.c \
    src/lc3_disk_init.c \
    src/lc3_intl_util.c \
    src/lc3_color_picker.c \
    src/lc3_gworld.c \
    src/lc3_font_scaler.c \
    src/lc3_help_mgr.c \
    src/lc3_ppc_toolbox.c \
    src/lc3_edition_mgr.c \
    src/lc3_dialog_mgr.c \
    src/lc3_defprocs.c \
    src/lc3_adb_mgr2.c \
    src/lc3_alias_mgr.c \
    src/lc3_component_mgr.c \
    src/lc3_pic_utils.c \
    src/lc3_comm_toolbox.c \
    src/lc3_apple_events.c \
    src/lc3_rsrc_extensions.c \
    src/lc3_text_services.c \
    src/lc3_display_mgr.c \
    src/lc3_dam.c \
    src/lc3_pack7.c \
    src/lc3_atp_engine.c \
    src/lc3_test_mgr.c \
    src/lc3_notification_mgr.c \
    src/lc3_serial_scc.c \
    src/lc3_extended_time.c \
    src/lc3_power_mgr.c \
    src/lc3_adsp_engine.c \
    src/lc3_appletalk_zip.c \
    src/lc3_script_mgr.c \
    src/lc3_color_cursor.c \
    src/lc3_translation_mgr.c \
    src/lc3_fpu_coprocessor.c \
    src/lc3_sound_input.c \
    src/lc3_folder_mgr.c \
    src/lc3_poly_raster.c \
    src/lc3_multifinder.c \
    src/lc3_sound_mixer.c \
    src/lc3_aux_packages.c \
    src/lc3_thread_mgr.c \
    src/lc3_mace.c \
    src/lc3_appletalk_nbp.c \
    src/lc3_colorsync.c \
    src/lc3_drag_drop.c \
    src/lc3_quicktime.c \
    src/lc3_comm_conn.c \
    src/lc3_comm_term_ft.c \
    src/lc3_apple_guide.c \
    src/lc3_icm_engine.c \
    src/lc3_ae_core_suite.c \
    src/lc3_speech_mgr.c \
    src/lc3_scsi_mgr43.c \
    src/lc3_mixed_mode.c \
    src/lc3_control_strip.c \
    src/lc3_thread_sync.c \
    src/lc3_display_timing.c \
    src/lc3_ara_core.c \
    src/lc3_desktop_db.c \
    src/lc3_region_engine.c \
    src/lc3_truetype_engine.c \
    src/lc3_worldscript.c \
    src/lc3_cqd_dither.c \
    src/lc3_appletalk_asp_pap.c \
    src/lc3_ag_db_engine.c \
    src/lc3_pic_spool.c \
    src/lc3_hfs_iterator.c \
    src/lc3_adsp_stream.c \
    src/lc3_stdfile_custom.c \
    src/lc3_component_registry.c \
    src/lc3_mem_strip_zone.c \
    src/lc3_egret_power_sleep.c \
    src/lc3_tt_bytecode.c \
    src/lc3_asp_server.c \
    src/lc3_dev_fs_async.c \
    src/lc3_sonora_accel.c \
    src/lc3_tt_font_prog.c \
    src/lc3_atp_buffers.c \
    src/lc3_cqd_deep_mask.c \
    src/lc3_scsi_phase.c \
    src/lc3_sane_transcend.c \
    src/lc3_appletalk_rtmp.c \
    src/lc3_slot_mgr2.c \
    src/lc3_power_sense.c \
    src/lc3_font_dispatch.c \
    src/lc3_appletalk_lap.c \
    src/lc3_scsi_pdma.c \
    src/lc3_gestalt_ext.c \
    src/lc3_edition_io.c \
    src/lc3_sound_in_driver.c \
    src/lc3_wdef_color.c \
    src/lc3_mdef_color.c \
    src/lc3_cdef_color.c \
    src/lc3_scc_driver.c \
    src/lc3_sound_resample.c \
    src/lc3_stdfile_nav.c \
    src/lc3_color_picker.c \
    src/lc3_localtalk_link.c \
    src/lc3_desk_mgr.c \
    src/lc3_scsi_blind.c \
    src/lc3_textedit_styles.c \
    src/lc3_afp_client.c \
    src/lc3_sane_elementary.c \
    src/lc3_notification_mgr.c \
    src/lc3_alias_engine.c \
    src/lc3_dam_core.c \
    src/lc3_atp_client.c \
    src/lc3_gworld_lock.c \
    src/lc3_aoce_core.c \
    src/lc3_scsi_sim.c \
    src/lc3_tt_composite.c \
    src/lc3_poly_rgn.c \
    src/lc3_clut_anim.c \
    src/lc3_ddp_engine.c \
    src/lc3_mace_decode.c \
    src/lc3_microbug_disasm.c \
    src/lc3_palette_tolerance.c \
    src/lc3_nbp_wildcard.c \
    src/lc3_adsp_window.c \
    src/lc3_scsi_err_map.c \
    src/lc3_comp_dispatch.c \
    src/lc3_aep_engine.c \
    src/lc3_snd_preview.c \
    src/lc3_cqd_matrix_dither.c \
    src/lc3_te_inline.c \
    src/lc3_zip_query.c \
    src/lc3_scsi_sg.c \
    src/lc3_cqd_pen_modes.c \
    src/lc3_color_cursor.c \
    src/lc3_atp_validate.c \
    src/lc3_scsi_target.c \
    src/lc3_tt_cmap.c \
    src/lc3_fp_transcendental.c \
    src/lc3_snd_midi.c \
    src/lc3_asp_session.c \
    src/lc3_mixed_mode.c \
    src/lc3_gx_geometry.c \
    src/lc3_pap_engine.c \
    src/lc3_snd_double_buf.c \
    src/lc3_ctb_connection.c \
    src/lc3_ctb_terminal.c \
    src/lc3_ctb_ft.c \
    src/lc3_pixpat_engine.c \
    src/lc3_rtmp_engine.c \
    src/lc3_tt_raster.c \
    src/lc3_scsi_async.c \
    src/lc3_ctb_crm.c \
    src/lc3_mace_encode.c

SRCS_S = \
    asm/lc3_traps.S

OBJS = $(SRCS_C:.c=.o) $(SRCS_S:.S=.o)
LIB  = liblc3rom.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
