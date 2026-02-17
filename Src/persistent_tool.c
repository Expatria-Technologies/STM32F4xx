/*

  my_plugin.c - user defined plugin for keeping current number tool over reboot

  Set $485=1 to enable, $485=0 to disable.

  Part of grblHAL

  Public domain.

*/

#include "grbl/hal.h"
#include "grbl/report.h"
#include "grbl/nvs_buffer.h"
#include "grbl/nuts_bolts.h"
#include "grbl/protocol.h"

#if GRBL_BUILD < 20230822
#error Persistent tool plugin requires build 20230822 or later!
#endif

typedef struct {
    bool keep_tool;
    tool_id_t tool_id;
    int32_t tlo_reference[N_AXIS];
    coord_data_t tool_length_offset;
    axes_signals_t tlo_reference_set;
    coord_system_id_t coord_system_id;
    tool_offset_mode_t tool_offset_mode;
} plugin_settings_t;

static nvs_address_t nvs_address;
static plugin_settings_t my_settings;
static on_report_options_ptr on_report_options;
static on_tool_changed_ptr on_tool_changed;
static on_parser_init_ptr on_parser_init;
static home_machine_ptr home_machine;
static on_homing_completed_ptr on_homing_completed;
static on_probe_completed_ptr on_probe_completed;
;

// Write settings to non volatile storage (NVS).
static void plugin_settings_save (void)
{
    my_settings.keep_tool = settings.flags.tool_persistent;
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&my_settings, sizeof(plugin_settings_t), true);
}

// Restore default settings and write to non volatile storage (NVS).
static void plugin_settings_restore (void)
{
    my_settings.keep_tool = settings.flags.tool_persistent;
    my_settings.tool_id = 0;

    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&my_settings, sizeof(plugin_settings_t), true);
}

// Load settings from non volatile storage (NVS).
// If load fails restore to default values.
static void plugin_settings_load (void)
{
    if(hal.nvs.memcpy_from_nvs((uint8_t *)&my_settings, nvs_address, sizeof(plugin_settings_t), true) != NVS_TransferResult_OK)
        plugin_settings_restore();
}

// Settings descriptor used by the core when interacting with this plugin.
static setting_details_t setting_details = {
    .save = plugin_settings_save,
    .load = plugin_settings_load,
    .restore = plugin_settings_restore
};

// Add info about our plugin to the $I report.
static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt)
        hal.stream.write("[PLUGIN:Persistent tool v0.02]" ASCII_EOL);
}

static void onToolChanged (tool_data_t *tool)
{
    int_fast16_t i;
    
    if(on_tool_changed)
        on_tool_changed(tool);

    if(settings.flags.tool_persistent) {
        my_settings.tool_id = tool->tool_id;

        my_settings.tlo_reference_set.value = sys.tlo_reference_set.value;

        my_settings.coord_system_id = gc_state.modal.g5x_offset.id;
        my_settings.tool_offset_mode = gc_state.modal.tool_offset_mode;        
        
        my_settings.tool_length_offset = tool->offset;
        for(i=0; i<N_AXIS; i++){
            my_settings.tlo_reference[i] = sys.tlo_reference[i];            
        }

        plugin_settings_save();
    }
}

static void onParserInit (parser_state_t *gc_state)
{
    int_fast16_t i;
    
    if(on_parser_init)
        on_parser_init(gc_state);

    if(sys.cold_start && my_settings.keep_tool) {
      #if N_TOOLS
        if(my_settings.tool_id <= N_TOOLS)
            gc_state->tool = &grbl.tool_table.tool[my_settings.tool_id];
      #else
        gc_state->tool->tool_id = my_settings.tool_id;
      #endif

        gc_state->modal.g5x_offset.id = my_settings.coord_system_id;
        gc_state->modal.tool_offset_mode = my_settings.tool_offset_mode;    
        
        gc_state->tool->offset = my_settings.tool_length_offset;
        for(i=0; i<N_AXIS; i++){
            sys.tlo_reference[i] = my_settings.tlo_reference[i];            
            
            //gc_state->tool->offset[i] = my_settings.tool_length_offset[i];
            //gc_state->tool->offset[i] = my_settings.tool_length_offset[i];
        }

        sys.tlo_reference_set.value = my_settings.tlo_reference_set.value;

        report_add_realtime(Report_TLOReference);
    }

}

static void onProbeCompleted (void){

    int_fast16_t i;
    for(i=0; i<N_AXIS; i++){
        my_settings.tlo_reference[i] = sys.tlo_reference[i];            
    }

    my_settings.tlo_reference_set.value = sys.tlo_reference_set.value;

    if(on_probe_completed)
        on_probe_completed();
}

// A call my_plugin_init will be issued automatically at startup.
// There is no need to change any source code elsewhere.
void persistent_tool_init (void)
{
    // Try to allocate space for our settings in non volatile storage (NVS).
    if((nvs_address = nvs_alloc(sizeof(plugin_settings_t)))) {

        // Add info about our plugin to the $I report.
        on_report_options = grbl.on_report_options;
        grbl.on_report_options = onReportOptions;

        // Subscribe to parser init event, sets current tool number to stored value on a cold start.
        on_parser_init = grbl.on_parser_init;
        grbl.on_parser_init = onParserInit;

        // Subscribe to tool changed event, write tool number (tool_id) to NVS when triggered.
        on_tool_changed =  grbl.on_tool_changed;
        grbl.on_tool_changed = onToolChanged;

        on_probe_completed =  grbl.on_probe_completed;
        grbl.on_probe_completed = onProbeCompleted;

        // Register our settings with the core.
        settings_register(&setting_details);
    }
}