/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AP_VideoTX.h"

#if AP_VIDEOTX_ENABLED

#include <AP_RCTelemetry/AP_CRSF_Telem.h>
#include <GCS_MAVLink/GCS.h>

#include <AP_HAL/AP_HAL.h>

extern const AP_HAL::HAL& hal;

AP_VideoTX *AP_VideoTX::singleton;

const AP_Param::GroupInfo AP_VTXParamBand::var_info[] = {
    AP_GROUPINFO("C1", 1, AP_VTXParamBand, channel[0], 0),
    AP_GROUPINFO("C2", 2, AP_VTXParamBand, channel[1], 0),
    AP_GROUPINFO("C3", 3, AP_VTXParamBand, channel[2], 0),
    AP_GROUPINFO("C4", 4, AP_VTXParamBand, channel[3], 0),
    AP_GROUPINFO("C5", 5, AP_VTXParamBand, channel[4], 0),
    AP_GROUPINFO("C6", 6, AP_VTXParamBand, channel[5], 0),
    AP_GROUPINFO("C7", 7, AP_VTXParamBand, channel[6], 0),
    AP_GROUPINFO("C8", 8, AP_VTXParamBand, channel[7], 0),
    AP_GROUPEND
};

const AP_Param::GroupInfo AP_VideoTX::var_info[] = {

    // @Param: ENABLE
    // @DisplayName: Is the Video Transmitter enabled or not
    // @Description: Toggles the Video Transmitter on and off
    // @Values: 0:Disable,1:Enable
    AP_GROUPINFO_FLAGS("ENABLE", 1, AP_VideoTX, _enabled, 0, AP_PARAM_FLAG_ENABLE),

    // @Param: POWER
    // @DisplayName: Video Transmitter Power Level
    // @Description: Video Transmitter Power Level. Different VTXs support different power levels, the power level chosen will be rounded down to the nearest supported power level
    // @Range: 1 1000
    AP_GROUPINFO("POWER",    2, AP_VideoTX, _power_mw, 0),

    // @Param: CHANNEL
    // @DisplayName: Video Transmitter Channel
    // @Description: Video Transmitter Channel
    // @User: Standard
    // @Range: 0 7
    AP_GROUPINFO("CHANNEL",  3, AP_VideoTX, _channel, 0),

    // @Param: BAND
    // @DisplayName: Video Transmitter Band
    // @Description: Video Transmitter Band
    // @User: Standard
    // @Values: 0:Band A,1:Band B,2:Band E,3:Airwave,4:RaceBand,5:Low RaceBand,6:1G3 Band A,7:1G3 Band B,8:Band X,9:3G3 Band A,10:3G3 Band B
    AP_GROUPINFO("BAND",  4, AP_VideoTX, _band, 0),

    // @Param: FREQ
    // @DisplayName: Video Transmitter Frequency
    // @Description: Video Transmitter Frequency. The frequency is derived from the setting of BAND and CHANNEL
    // @User: Standard
    // @ReadOnly: True
    // @Range: 100 32767
    AP_GROUPINFO("FREQ",  5, AP_VideoTX, _frequency_mhz, 0),

    // @Param: OPTIONS
    // @DisplayName: Video Transmitter Options
    // @Description: Video Transmitter Options. Pitmode puts the VTX in a low power state. Unlocked enables certain restricted frequencies and power levels. Do not enable the Unlocked option unless you have appropriate permissions in your jurisdiction to transmit at high power levels. One stop-bit may be required for VTXs that erroneously mimic iNav behaviour.
    // @User: Advanced
    // @Bitmask: 0:Pitmode,1:Pitmode until armed,2:Pitmode when disarmed,3:Unlocked,4:Add leading zero byte to requests,5:Use 1 stop-bit in SmartAudio,6:Ignore CRC in SmartAudio,7:Ignore status updates in CRSF and blindly set VTX options
    AP_GROUPINFO("OPTIONS",  6, AP_VideoTX, _options, 0),

    // @Param: MAX_POWER
    // @DisplayName: Video Transmitter Max Power Level
    // @Description: Video Transmitter Maximum Power Level. Different VTXs support different power levels, this prevents the power aux switch from requesting too high a power level. The switch supports 6 power levels and the selected power will be a subdivision between 0 and this setting.
    // @Range: 25 1000
    AP_GROUPINFO("MAX_POWER", 7, AP_VideoTX, _max_power_mw, 800),

    // Optional packed frequency overrides: bit23 enabled, bits22..19 band,
    // bits18..16 channel, bits15..0 frequency MHz. 0 disables a slot.
    // Values are restricted to 24 bits for lossless MAVLink float parameters.
    AP_GROUPINFO("TBL1", 8, AP_VideoTX, _table_override[0], 0),
    AP_GROUPINFO("TBL2", 9, AP_VideoTX, _table_override[1], 0),
    AP_GROUPINFO("TBL3", 10, AP_VideoTX, _table_override[2], 0),
    AP_GROUPINFO("TBL4", 11, AP_VideoTX, _table_override[3], 0),
    AP_GROUPINFO("TBL5", 12, AP_VideoTX, _table_override[4], 0),
    AP_GROUPINFO("TBL6", 13, AP_VideoTX, _table_override[5], 0),
    AP_GROUPINFO("TBL7", 14, AP_VideoTX, _table_override[6], 0),
    AP_GROUPINFO("TBL8", 15, AP_VideoTX, _table_override[7], 0),
    AP_GROUPINFO("TBL9", 16, AP_VideoTX, _table_override[8], 0),
    AP_GROUPINFO("TBL10", 17, AP_VideoTX, _table_override[9], 0),
    AP_GROUPINFO("TBL11", 18, AP_VideoTX, _table_override[10], 0),
    AP_GROUPINFO("TBL12", 19, AP_VideoTX, _table_override[11], 0),
    AP_GROUPINFO("TBL13", 20, AP_VideoTX, _table_override[12], 0),
    AP_GROUPINFO("TBL14", 21, AP_VideoTX, _table_override[13], 0),
    AP_GROUPINFO("TBL15", 22, AP_VideoTX, _table_override[14], 0),
    AP_GROUPINFO("TBL16", 23, AP_VideoTX, _table_override[15], 0),

    // Complete persistent frequency table, standard AP_Param storage.
    AP_GROUPINFO("TCNT", 24, AP_VideoTX, _persistent_band_count, 0),
    AP_GROUPINFO("TCRC", 25, AP_VideoTX, _persistent_crc, 0),
    AP_SUBGROUPINFO(_persistent_band[0], "TB0_", 26, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[1], "TB1_", 27, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[2], "TB2_", 28, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[3], "TB3_", 29, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[4], "TB4_", 30, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[5], "TB5_", 31, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[6], "TB6_", 32, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[7], "TB7_", 33, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[8], "TB8_", 34, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[9], "TB9_", 35, AP_VideoTX, AP_VTXParamBand),
    AP_SUBGROUPINFO(_persistent_band[10], "TB10_", 36, AP_VideoTX, AP_VTXParamBand),

    // Independent custom VTX power table.
    AP_GROUPINFO("PCNT", 37, AP_VideoTX, _power_table_count, 0),
    AP_GROUPINFO("PTYPE", 38, AP_VideoTX, _power_table_type, 0),
    AP_GROUPINFO("PIDX", 39, AP_VideoTX, _power_table_index, 0),
    AP_GROUPINFO("PCRC", 40, AP_VideoTX, _power_table_crc, 0),
    AP_GROUPINFO("PC1", 41, AP_VideoTX, _power_table_codes[0], 0),
    AP_GROUPINFO("PC2", 42, AP_VideoTX, _power_table_codes[1], 0),
    AP_GROUPINFO("PC3", 43, AP_VideoTX, _power_table_codes[2], 0),
    AP_GROUPINFO("PC4", 44, AP_VideoTX, _power_table_codes[3], 0),
    AP_GROUPINFO("PC5", 45, AP_VideoTX, _power_table_codes[4], 0),
    AP_GROUPINFO("PC6", 46, AP_VideoTX, _power_table_codes[5], 0),
    AP_GROUPINFO("PC7", 47, AP_VideoTX, _power_table_codes[6], 0),
    AP_GROUPINFO("PC8", 48, AP_VideoTX, _power_table_codes[7], 0),
    AP_GROUPINFO("PC9", 49, AP_VideoTX, _power_table_codes[8], 0),
    AP_GROUPINFO("PC10", 50, AP_VideoTX, _power_table_codes[9], 0),
    AP_GROUPINFO("PL1", 51, AP_VideoTX, _power_table_labels[0], 0),
    AP_GROUPINFO("PL2", 52, AP_VideoTX, _power_table_labels[1], 0),
    AP_GROUPINFO("PL3", 53, AP_VideoTX, _power_table_labels[2], 0),
    AP_GROUPINFO("PL4", 54, AP_VideoTX, _power_table_labels[3], 0),
    AP_GROUPINFO("PL5", 55, AP_VideoTX, _power_table_labels[4], 0),
    AP_GROUPINFO("PL6", 56, AP_VideoTX, _power_table_labels[5], 0),
    AP_GROUPINFO("PL7", 57, AP_VideoTX, _power_table_labels[6], 0),
    AP_GROUPINFO("PL8", 58, AP_VideoTX, _power_table_labels[7], 0),
    AP_GROUPINFO("PL9", 59, AP_VideoTX, _power_table_labels[8], 0),
    AP_GROUPINFO("PL10", 60, AP_VideoTX, _power_table_labels[9], 0),
    AP_GROUPEND
};

//#define VTX_DEBUG
#ifdef VTX_DEBUG
# define debug(fmt, args...)	hal.console->printf("VTX: " fmt "\n", ##args)
#else
# define debug(fmt, args...)	do {} while(0)
#endif

extern const AP_HAL::HAL& hal;

const char * AP_VideoTX::band_names[] = {"A","B","E","F","R","L","1G3_A","1G3_B","X","3G3_A","3G3_B"};

const uint16_t AP_VideoTX::VIDEO_CHANNELS[AP_VideoTX::MAX_BANDS][VTX_MAX_CHANNELS] =
{
    { 5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725}, /* Band A */
    { 5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866}, /* Band B */
    { 5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945}, /* Band E */
    { 5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880}, /* Airwave */
    { 5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917}, /* Race */
    { 5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621}, /* LO Race */
    { 1080, 1120, 1160, 1200, 1240, 1280, 1320, 1360}, /* Band 1G3_A */
    { 1080, 1120, 1160, 1200, 1258, 1280, 1320, 1360}, /* Band 1G3_B */
    { 4990, 5020, 5050, 5080, 5110, 5140, 5170, 5200}, /* Band X */
    { 3330, 3350, 3370, 3390, 3410, 3430, 3450, 3470}, /* Band 3G3_A */
    { 3170, 3190, 3210, 3230, 3250, 3270, 3290, 3310}  /* Band 3G3_B */
};

// mapping of power level to milliwatt to dbm
// valid power levels from SmartAudio spec, the adjacent levels might be the actual values
// so these are marked as level + 0x10 and will be switched if a dbm message proves it
AP_VideoTX::PowerLevel AP_VideoTX::_power_levels[VTX_MAX_POWER_LEVELS] = {
    // level, mw, dbm, dac
    { 0xFF,  0,    0, 0    }, // only in SA 2.1
    { 0,    25,   14, 7    },
    { 0x11, 100,  20, 0xFF }, // only in SA 2.1
    { 1,    200,  23, 16   },
    { 0x12, 400,  26, 0xFF }, // only in SA 2.1
    { 2,    500,  27, 25   },
    { 0x12, 600,  28, 0xFF }, // Tramp lies above power levels and always returns 25/100/200/400/600
    { 3,    800,  29, 40   },
    { 0x13, 1000, 30, 0xFF }, // only in SA 2.1
    { 0xFF, 0,    0,  0XFF, PowerActive::Inactive }  // slot reserved for a custom power level
};

AP_VideoTX::AP_VideoTX()
{
    if (singleton) {
        AP_HAL::panic("Too many VTXs");
        return;
    }
    singleton = this;

    AP_Param::setup_object_defaults(this, var_info);
}

AP_VideoTX::~AP_VideoTX(void)
{
    singleton = nullptr;
}

bool AP_VideoTX::load_custom_power_table()
{
    _custom_power_boot_requested = _power_table_count.get() != 0;
    _custom_power_boot_crc = 0;
    _custom_power_valid = false;
    _custom_power_confirmation.store(0, std::memory_order_relaxed);
    _custom_power_table = {};
    const int32_t count = _power_table_count.get();
    const int32_t type = _power_table_type.get();
    const int32_t crc = _power_table_crc.get();
    const int32_t index = _power_table_index.get();
    if (count < 1 || count > JTPower::MAX_LEVELS ||
        type < 1 || type > 4 || index < 0 || index >= count ||
        crc <= 0 || crc > 0xFFFFFF) {
        return false;
    }
    JTPower::Table candidate{};
    candidate.protocol = JTPower::Protocol(type);
    candidate.count = uint8_t(count);
    for (uint8_t i = 0; i < JTPower::MAX_LEVELS; i++) {
        const int32_t code = _power_table_codes[i].get();
        const int32_t label = _power_table_labels[i].get();
        if (code < 0 || code > 65535 || label < 0 || label > 0xFFFFFF ||
            (i >= count && (code != 0 || label != 0))) {
            return false;
        }
        candidate.levels[i] = {uint16_t(code), uint32_t(label)};
    }
    if (!JTPower::validate(candidate) || JTPower::checksum(candidate) != uint32_t(crc)) {
        return false;
    }
    _custom_power_table = candidate;
    _custom_power_boot_crc = uint32_t(crc);
    _custom_power_valid = true;
    return true;
}

bool AP_VideoTX::init(void)
{
    if (_initialized) {
        return false;
    }

    // PARAMETER_CONVERSION - Added: Sept-2022
    _options.convert_parameter_width(AP_PARAM_INT16);

    // find the index into the power table
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (_power_mw <= _power_levels[i].mw) {
            if (_power_mw != _power_levels[i].mw) {
                if (i > 0) {
                    _current_power = i - 1;
                }
                _power_mw.set_and_save(get_power_mw());
            } else {
                _current_power = i;
            }
            break;
        }
    }
    _current_band = _band;
    _current_channel = _channel;
    _current_frequency = _frequency_mhz;
    _current_options = _options;
    _current_enabled = _enabled;
    _initialized = true;
    // Only a fully committed and checksum-verified parameter table is used.
    load_jtv1_params();
    load_custom_power_table();

    return true;
}

uint16_t AP_VideoTX::get_frequency_mhz(uint8_t band, uint8_t channel) const
{
    if (band >= MAX_BANDS || channel >= VTX_MAX_CHANNELS) {
        return 0;
    }
    if (_jtv1_active) {
        return band < _jtv1_table.bands ? _jtv1_table.frequency[band][channel] : 0;
    }
    // An override is an explicit mapping; absent overrides retain the factory table.
    for (uint8_t i = 0; i < VTX_TABLE_OVERRIDE_COUNT; i++) {
        const uint32_t entry = uint32_t(_table_override[i].get());
        if ((entry & 0x800000U) != 0 &&
            ((entry >> 19) & 0x0FU) == band &&
            ((entry >> 16) & 0x07U) == channel) {
            const uint16_t freq = entry & 0xFFFFU;
            return (freq >= 100 && freq <= 32767) ? freq : 0;
        }
    }
    return VIDEO_CHANNELS[band][channel];
}

bool AP_VideoTX::get_band_and_channel(uint16_t freq, VideoBand& band, uint8_t& channel) const
{
    if (freq == 0) {
        return false;
    }
    for (uint8_t i = 0; i < AP_VideoTX::MAX_BANDS; i++) {
        for (uint8_t j = 0; j < VTX_MAX_CHANNELS; j++) {
            if (get_frequency_mhz(i, j) == freq) {
                band = VideoBand(i);
                channel = j;
                return true;
            }
        }
    }
    return false;
}

bool AP_VideoTX::stage_jtv1(const uint8_t *data, size_t length)
{
    _jtv1_staged_valid = false;
    _jtv1_staged_valid = JTV1::decode(data, length, _jtv1_staged);
    return _jtv1_staged_valid;
}

bool AP_VideoTX::activate_staged_jtv1()
{
    // Never switch the frequency map in flight. The transport/persistence
    // layer must also ensure this is only called after durable commit.
    if (!_jtv1_staged_valid || hal.util->get_soft_armed()) {
        return false;
    }
    // Do not activate a table that lacks the currently selected channel.
    if (_band.get() < 0 || _band.get() >= _jtv1_staged.bands ||
        _channel.get() < 0 || _channel.get() >= VTX_MAX_CHANNELS ||
        _jtv1_staged.frequency[_band.get()][_channel.get()] == 0) {
        return false;
    }
    _jtv1_table = _jtv1_staged;
    _jtv1_active = true;
    // Keep the configured frequency aligned with the newly activated table.
    // Do not save here: persistent activation requires a separate durable commit.
    _frequency_mhz.set(_jtv1_table.frequency[_band.get()][_channel.get()]);
    _jtv1_staged_valid = false;
    return true;
}

// The 24-bit checksum is exactly representable in float-based parameter tools.
// It is a consistency marker, not a cryptographic integrity guarantee.
static uint32_t vtx_param_checksum(const JTV1::Table &table)
{
    uint32_t hash = 2166136261U;
    hash = (hash ^ table.bands) * 16777619U;
    for (uint8_t b = 0; b < JTV1::MAX_BANDS; b++) {
        for (uint8_t c = 0; c < JTV1::MAX_CHANNELS; c++) {
            const uint16_t freq = table.frequency[b][c];
            hash = (hash ^ (freq & 0xFFU)) * 16777619U;
            hash = (hash ^ (freq >> 8)) * 16777619U;
        }
    }
    return (hash & 0xFFFFFFU) | 1U;
}

bool AP_VideoTX::save_staged_jtv1_params()
{
    if (!_jtv1_staged_valid || hal.util->get_soft_armed() ||
        _jtv1_staged.powers != 0 ||
        _band.get() < 0 || _band.get() >= _jtv1_staged.bands ||
        _channel.get() < 0 || _channel.get() >= JTV1::MAX_CHANNELS ||
        _jtv1_staged.frequency[_band.get()][_channel.get()] == 0) {
        return false;
    }
    // Invalidate the previous table before writing any channel.
    _persistent_crc.set_and_save(0);
    for (uint8_t b = 0; b < JTV1::MAX_BANDS; b++) {
        for (uint8_t c = 0; c < JTV1::MAX_CHANNELS; c++) {
            _persistent_band[b].channel[c].set_and_save_ifchanged(
                _jtv1_staged.frequency[b][c]);
        }
    }
    _persistent_band_count.set_and_save(_jtv1_staged.bands);
    _persistent_crc.set_and_save(vtx_param_checksum(_jtv1_staged));
    return true;
}

bool AP_VideoTX::load_jtv1_params()
{
    const int8_t bands = _persistent_band_count.get();
    if (bands < 1 || bands > JTV1::MAX_BANDS || _persistent_crc.get() == 0) {
        return false;
    }
    JTV1::Table candidate{};
    candidate.bands = uint8_t(bands);
    candidate.channels = JTV1::MAX_CHANNELS;
    for (uint8_t b = 0; b < JTV1::MAX_BANDS; b++) {
        for (uint8_t c = 0; c < JTV1::MAX_CHANNELS; c++) {
            const int16_t freq = _persistent_band[b].channel[c].get();
            if (freq < 0 || (freq != 0 && freq < 100) ||
                (b >= candidate.bands && freq != 0)) {
                return false;
            }
            candidate.frequency[b][c] = uint16_t(freq);
        }
    }
    if (uint32_t(_persistent_crc.get()) != vtx_param_checksum(candidate)) {
        return false;
    }
    if (_band.get() < 0 || _band.get() >= candidate.bands ||
        _channel.get() < 0 || _channel.get() >= JTV1::MAX_CHANNELS ||
        candidate.frequency[_band.get()][_channel.get()] == 0) {
        return false;
    }
    _jtv1_table = candidate;
    _jtv1_active = true;
    _frequency_mhz.set(candidate.frequency[_band.get()][_channel.get()]);
    // The cached current frequency was captured before the custom table loaded.
    // It may now identify a different band/channel. Treat it as unknown until
    // a VTX backend reports its actual state, so set_defaults() cannot remap
    // the selected band/channel using the stale pre-table frequency.
    _current_frequency = 0;
    return true;
}

// set the current power
void AP_VideoTX::set_configured_power_mw(uint16_t power)
{
    _power_mw.set_and_save_ifchanged(power);
}

uint8_t AP_VideoTX::find_current_power() const
{
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (_power_mw == _power_levels[i].mw) {
            return i;
        }
    }
    return 0;
}

// set the power in dbm, rounding appropriately
void AP_VideoTX::set_power_dbm(uint8_t power, PowerActive active)
{
    if (power == _power_levels[_current_power].dbm
        && _power_levels[_current_power].active == active) {
        return;
    }

    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (power == _power_levels[i].dbm) {
            _current_power = i;
            _power_levels[i].active = active;
            debug("learned power %ddbm", power);
            // now unlearn the "other" power level since we have no other way of guessing
            // the supported levels
            if ((_power_levels[i].level & 0xF0) == 0x10) {
                _power_levels[i].level = _power_levels[i].level & 0xF;
            }
            if (i > 0 && _power_levels[i-1].level == _power_levels[i].level) {
                debug("invalidated power %dwm, level %d is now %dmw", _power_levels[i-1].mw, _power_levels[i].level, _power_levels[i].mw);
                _power_levels[i-1].level = 0xFF;
                _power_levels[i-1].active = PowerActive::Inactive;
            } else if (i < VTX_MAX_POWER_LEVELS-1 && _power_levels[i+1].level == _power_levels[i].level) {
                debug("invalidated power %dwm, level %d is now %dmw", _power_levels[i+1].mw, _power_levels[i].level, _power_levels[i].mw);
                _power_levels[i+1].level = 0xFF;
                _power_levels[i+1].active = PowerActive::Inactive;
            }
            return;
        }
    }
    // learn the non-standard power
    _current_power = update_power_dbm(power, active);
}

// add an active power setting in dbm
uint8_t AP_VideoTX::update_power_dbm(uint8_t power, PowerActive active)
{
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (power == _power_levels[i].dbm) {
            if (_power_levels[i].active != active) {
                _power_levels[i].active = active;
                debug("%s power %ddbm", active == PowerActive::Active ? "learned" : "invalidated", power);
            }
            return i;
        }
    }
    // handed a non-standard value, use the last slot
    _power_levels[VTX_MAX_POWER_LEVELS-1].dbm = power;
    _power_levels[VTX_MAX_POWER_LEVELS-1].level = 255;
    _power_levels[VTX_MAX_POWER_LEVELS-1].dac = 255;
    _power_levels[VTX_MAX_POWER_LEVELS-1].mw = uint16_t(roundf(powf(10, power * 0.1f)));
    _power_levels[VTX_MAX_POWER_LEVELS-1].active = active;
    debug("non-standard power %ddbm -> %dmw", power, _power_levels[VTX_MAX_POWER_LEVELS-1].mw);
    return VTX_MAX_POWER_LEVELS-1;
}

// add all active power setting in dbm
void AP_VideoTX::update_all_power_dbm(uint8_t nlevels, const uint8_t power[])
{
    for (uint8_t i = 0; i < nlevels; i++) {
        update_power_dbm(power[i], PowerActive::Active);
    }
    // invalidate the remaining ones
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (_power_levels[i].active == PowerActive::Unknown) {
            _power_levels[i].active = PowerActive::Inactive;
        }
    }
}

// set the power in mw
void AP_VideoTX::set_power_mw(uint16_t power)
{
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (power == _power_levels[i].mw) {
            _current_power = i;
            break;
        }
    }
}

// set the power "level"
void AP_VideoTX::set_power_level(uint8_t level, PowerActive active)
{
    if (level == _power_levels[_current_power].level
        && _power_levels[_current_power].active == active) {
        return;
    }

    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (level == _power_levels[i].level) {
            _current_power = i;
            _power_levels[i].active = active;
            debug("learned power level %d: %dmw", level, get_power_mw());
            break;
        }
    }
}

// set the power dac
void AP_VideoTX::set_power_dac(uint16_t power, PowerActive active)
{
    if (power == _power_levels[_current_power].dac
        && _power_levels[_current_power].active == active) {
        return;
    }

    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (power == _power_levels[i].dac) {
            _current_power = i;
            _power_levels[i].active = active;
            debug("learned power %dmw", get_power_mw());
        }
    }
}

// set the current channel
void AP_VideoTX::set_enabled(bool enabled)
{
    _current_enabled = enabled;
    if (!_enabled.configured()) {
        _enabled.set_and_save(enabled);
    }
}

void AP_VideoTX::set_power_is_current()
{
    set_power_dbm(get_configured_power_dbm());
}

void AP_VideoTX::set_freq_is_current()
{
    _current_frequency = _frequency_mhz;
    _current_band = _band;
    _current_channel = _channel;
}

// periodic update
void AP_VideoTX::update(void)
{
    if (!_enabled) {
        return;
    }

    // manipulate pitmode if pitmode-on-disarm or power-on-arm is set
    if (has_option(VideoOptions::VTX_PITMODE_ON_DISARM) || has_option(VideoOptions::VTX_PITMODE_UNTIL_ARM)) {
        if (hal.util->get_soft_armed() && has_option(VideoOptions::VTX_PITMODE)) {
            _options.set(_options & ~uint8_t(VideoOptions::VTX_PITMODE));
        } else if (!hal.util->get_soft_armed() && !has_option(VideoOptions::VTX_PITMODE)
            && has_option(VideoOptions::VTX_PITMODE_ON_DISARM)) {
            _options.set(_options | uint8_t(VideoOptions::VTX_PITMODE));
        }
    }
    // check that the requested power is actually allowed
    // reset if not
    if (_power_mw != get_power_mw()) {
        if (_power_levels[find_current_power()].active == PowerActive::Inactive) {
            // reset to something we know works
            debug("power reset to %dmw from %dmw", get_power_mw(), _power_mw.get());
            _power_mw.set_and_save(get_power_mw());
        }
    }
}

bool AP_VideoTX::update_options() const
{
    if (!_defaults_set) {
        return false;
    }
    // check pitmode
    if ((_options & uint8_t(VideoOptions::VTX_PITMODE))
        != (_current_options & uint8_t(VideoOptions::VTX_PITMODE))) {
        return true;
    }

#if HAL_CRSF_TELEM_ENABLED
    // using CRSF so unlock is not an option
    if (AP::crsf_telem() != nullptr) {
        return false;
    }
#endif
    // check unlock only
    if ((_options & uint8_t(VideoOptions::VTX_UNLOCKED)) != 0
        && (_current_options & uint8_t(VideoOptions::VTX_UNLOCKED)) == 0) {
        return true;
    }

    // ignore everything else
    return false;
}

bool AP_VideoTX::custom_power_valid() const
{
    if (!_custom_power_valid || _power_table_count.get() != _custom_power_table.count || _power_table_type.get() != int32_t(_custom_power_table.protocol) || uint32_t(_power_table_crc.get()) != _custom_power_boot_crc) {
        return false;
    }
    for (uint8_t i = 0; i < JTPower::MAX_LEVELS; i++) {
        if (_power_table_codes[i].get() != int32_t(_custom_power_table.levels[i].code) || _power_table_labels[i].get() != int32_t(_custom_power_table.levels[i].label)) {
            return false;
        }
    }
    return true;
}

bool AP_VideoTX::custom_power_wire(JTPower::Protocol protocol, uint16_t &wire) const
{
    const int8_t index = _power_table_index.get();
    return custom_power_requested() && custom_power_valid() && _custom_power_table.protocol == protocol && index >= 0 && JTPower::encode(_custom_power_table, uint8_t(index), wire);
}

void AP_VideoTX::confirm_custom_power(JTPower::Protocol protocol, uint16_t code)
{
    if (custom_power_requested() && custom_power_valid() && _custom_power_table.protocol == protocol) {
        const int8_t index = JTPower::find_code(_custom_power_table, code);
        const uint32_t ticks = uint32_t(AP_HAL::millis64() / 100U);
        const uint32_t packed = index < 0 ? 0U : (ticks << 4U) | uint32_t(index + 1);
        _custom_power_confirmation.store(packed, std::memory_order_relaxed);
    }
}

int8_t AP_VideoTX::confirmed_custom_power(bool require_fresh) const
{
    const uint32_t packed = _custom_power_confirmation.load(std::memory_order_relaxed);
    const uint32_t id = packed & 15U;
    const uint32_t now = uint32_t(AP_HAL::millis64() / 100U);
    const uint32_t age = (now - (packed >> 4U)) & 0x0FFFFFFFU;
    if (id == 0 || id > _custom_power_table.count || (require_fresh && age >= 50U)) {
        return -1;
    }
    return int8_t(id - 1U);
}

bool AP_VideoTX::get_custom_power_label(char (&out)[5]) const
{
    out[0] = 0;
    const int8_t confirmed = confirmed_custom_power();
    if (!_enabled || !custom_power_requested() || !custom_power_valid() || confirmed < 0) {
        return false;
    }
    return JTPower::decode_label(_custom_power_table.levels[confirmed].label, out);
}

bool AP_VideoTX::custom_power_pending(JTPower::Protocol protocol) const
{
    uint16_t wire;
    return _enabled && _defaults_set && !get_pitmode() && !get_configured_pitmode() && custom_power_wire(protocol, wire) && confirmed_custom_power(false) != _power_table_index.get();
}

bool AP_VideoTX::update_power() const {
    if (custom_power_requested()) {
        return false;
    }
    if (!_defaults_set || _power_mw == get_power_mw() || get_pitmode()) {
        return false;
    }
    // check that the requested power is actually allowed
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (_power_mw == _power_levels[i].mw
            && _power_levels[i].active != PowerActive::Inactive) {
            return true;
        }
    }
    // asked for something unsupported - only SA2.1 allows this and will have already provided a list
    return false;
}

bool AP_VideoTX::have_params_changed() const
{
    return _enabled
        && (update_power()
        || update_band()
        || update_channel()
        || update_frequency()
        || update_options());
}

// update the configured frequency to match the channel and band
void AP_VideoTX::update_configured_frequency()
{
    const uint16_t freq = get_frequency_mhz(_band, _channel);
    if (freq != 0) {
        _frequency_mhz.set_and_save(freq);
    }
}

// update the configured channel and band to match the frequency
void AP_VideoTX::update_configured_channel_and_band()
{
    VideoBand band;
    uint8_t channel;
    if (get_band_and_channel(_frequency_mhz, band, channel)) {
        _band.set_and_save(band);
        _channel.set_and_save(channel);
    } else {
        update_configured_frequency();
    }
}

// set the current configured values if not currently set in storage
// this is necessary so that the current settings can be seen
bool AP_VideoTX::set_defaults()
{
    if (_defaults_set) {
        return false;
    }

    if (has_active_jtv1()) {
        const uint16_t selected = get_frequency_mhz(_band, _channel);
        if (selected == 0 || _current_frequency == 0) {
            return false;
        }
        VideoBand band;
        uint8_t channel;
        if (get_band_and_channel(_current_frequency, band, channel)) {
            _current_band = band;
            _current_channel = channel;
        }
        if (!_options.configured()) {
            _options.set_and_save(_current_options);
        }
        _frequency_mhz.set_and_save_ifchanged(selected);
        _defaults_set = true;
        announce_vtx_settings();
        return true;
    }

    // check that our current view of frequency matches band/channel
    // if not then force one to be correct
    uint16_t calced_freq = get_frequency_mhz(_current_band, _current_channel);
    if (calced_freq == 0) {
        // Do not propagate an invalid override to the VTX.
        return false;
    }
    if (_current_frequency != calced_freq) {
        if (_current_frequency > 0) {
            VideoBand band;
            uint8_t channel;
            if (get_band_and_channel(_current_frequency, band, channel)) {
                _current_band = band;
                _current_channel = channel;
            } else {
                _current_frequency = calced_freq;
            }
        } else {
            _current_frequency = calced_freq;
        }
    }

    if (!_options.configured()) {
        _options.set_and_save(_current_options);
    }
    if (!_channel.configured()) {
        _channel.set_and_save(_current_channel);
    }
    if (!_band.configured()) {
        _band.set_and_save(_current_band);
    }
    if (!_power_mw.configured()) {
        _power_mw.set_and_save(get_power_mw());
    }
    if (!_frequency_mhz.configured()) {
        _frequency_mhz.set_and_save(_current_frequency);
    }

    // Now check that the user didn't screw up by selecting incompatible options
    if (_frequency_mhz != get_frequency_mhz(_band, _channel)) {
        if (_frequency_mhz > 0) {
            update_configured_channel_and_band();
        } else {
            update_configured_frequency();
        }
    }

    _defaults_set = true;

    announce_vtx_settings();

    return true;
}

void AP_VideoTX::announce_vtx_settings() const
{
    // Output a friendly message so the user knows the VTX has been detected
    GCS_SEND_TEXT(MAV_SEVERITY_INFO, "VTX: %s%d %dMHz, PWR: %dmW",
        (_band.get() >= 0 && _band.get() < MAX_BANDS) ? band_names[_band.get()] : "?",
        _channel.get() + 1, _frequency_mhz.get(),
        has_option(VideoOptions::VTX_PITMODE) ? 0 : _power_mw.get());
}

// change the video power based on switch input
// 6-pos range is in the middle of the available range
bool AP_VideoTX::step_channel(int8_t direction)
{
    if (!_enabled || !_jtv1_active || (direction != 1 && direction != -1)) {
        return false;
    }
    const int16_t total = MAX_BANDS * VTX_MAX_CHANNELS;
    int16_t index = int16_t(_band.get()) * VTX_MAX_CHANNELS + _channel.get();
    if (_band.get() < 0 || _band.get() >= MAX_BANDS || _channel.get() < 0 || _channel.get() >= VTX_MAX_CHANNELS) {
        return false;
    }
    for (int16_t i = 0; i < total; i++) {
        index = (index + direction + total) % total;
        const uint8_t band = index / VTX_MAX_CHANNELS;
        const uint8_t channel = index % VTX_MAX_CHANNELS;
        const uint16_t freq = get_frequency_mhz(band, channel);
        if (freq != 0) {
            _band.set_and_save_ifchanged(band);
            _channel.set_and_save_ifchanged(channel);
            _frequency_mhz.set_and_save_ifchanged(freq);
            return true;
        }
    }
    return false;
}

bool AP_VideoTX::step_power(int8_t direction)
{
    if (!_enabled || !custom_power_valid() || (direction != 1 && direction != -1)) {
        return false;
    }
    const int16_t index = _power_table_index.get();
    const int16_t next = index + direction;
    if (index < 0 || index >= _custom_power_table.count || next < 0 || next >= _custom_power_table.count) {
        return false;
    }
    _power_table_index.set_and_save_ifchanged(next);
    return true;
}

void AP_VideoTX::change_power(int8_t position)
{
    if (!_enabled || position < 0 || position > 5) {
        return;
    }
    if (custom_power_requested()) {
        if (custom_power_valid()) {
            const uint8_t index = (uint16_t(position) * (_custom_power_table.count - 1) + 2U) / 5U;
            _power_table_index.set_and_save_ifchanged(index);
        }
        return;
    }
    // first find out how many possible levels there are
    uint8_t num_active_levels = 0;
    for (uint8_t i = 0; i < VTX_MAX_POWER_LEVELS; i++) {
        if (_power_levels[i].active != PowerActive::Inactive && _power_levels[i].mw <= _max_power_mw) {
            num_active_levels++;
        }
    }
    // iterate through to find the level
    uint16_t level = constrain_int16(roundf((num_active_levels * (position + 1)/ 6.0f) - 1), 0, num_active_levels - 1);
    debug("looking for pos %d power level %d from %d", position, level, num_active_levels);
    uint16_t power = 0;
    for (uint8_t i = 0, j = 0; i < num_active_levels; i++, j++) {
        while (j < VTX_MAX_POWER_LEVELS-1 && _power_levels[j].active == PowerActive::Inactive) {
            j++;
        }
        if (i == level) {
            power = _power_levels[j].mw;
            debug("selected power %dmw", power);
            break;
        }
    }

    if (power == 0) {
        if (!hal.util->get_soft_armed()) {    // don't allow pitmode to be entered if already armed
            set_configured_options(get_configured_options() | uint8_t(VideoOptions::VTX_PITMODE));
        }
    } else {
        if (has_option(VideoOptions::VTX_PITMODE)) {
            set_configured_options(get_configured_options() & ~uint8_t(VideoOptions::VTX_PITMODE));
        }
        set_configured_power_mw(power);
    }
}

namespace AP {
    AP_VideoTX& vtx() {
        return *AP_VideoTX::get_singleton();
    }
};

#endif
