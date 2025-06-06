#define DT_DRV_COMPAT maxim_max77976

// #if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
// #warning "MAX77976 driver enabled without any devices"
// #endif

#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>

/* --------------------------------------------------------------------------
 * Register map
 */

#define CHIP_ID 0x00
#define CHIP_REVISION 0x01
#define OTP_REVISION 0x02
#define TOP_INT 0x03
#define TOP_INT_MASK 0x04
#define TOP_CTRL 0x05
#define SW_RESET 0x50
#define SM_CTRL 0x51
#define I2C_CNFG 0x40
#define CHG_INT 0x10
#define CHG_INT_MASK 0x11
#define CHG_INT_OK 0x12
#define CHG_DETAILS_00 0x13
#define CHG_DETAILS_01 0x14
#define CHG_DETAILS_02 0x15
#define CHG_CNFG_00 0x16
#define CHG_CNFG_01 0x17
#define CHG_CNFG_02 0x18
#define CHG_CNFG_03 0x19
#define CHG_CNFG_04 0x1A
#define CHG_CNFG_05 0x1B
#define CHG_CNFG_06 0x1C
#define CHG_CNFG_07 0x1D
#define CHG_CNFG_08 0x1E
#define CHG_CNFG_09 0x1F
#define CHG_CNFG_10 0x20
#define CHG_CNFG_11 0x21
#define CHG_CNFG_12 0x22
#define CHG_CNFG_13 0x23
#define STAT_CNFG 0x24

/* CHG_DETAILS_01.CHG_DTLS values */
enum max77976_charging_state {
	MAX77976_CHARGING_PREQUALIFICATION = 0x0,
	MAX77976_CHARGING_FAST_CONST_CURRENT,
	MAX77976_CHARGING_FAST_CONST_VOLTAGE,
	MAX77976_CHARGING_TOP_OFF,
	MAX77976_CHARGING_DONE,
	MAX77976_CHARGING_RESERVED_05,
	MAX77976_CHARGING_TIMER_FAULT,
	MAX77976_CHARGING_SUSPENDED_QBATT_OFF,
	MAX77976_CHARGING_OFF,
	MAX77976_CHARGING_RESERVED_09,
	MAX77976_CHARGING_THERMAL_SHUTDOWN,
	MAX77976_CHARGING_WATCHDOG_EXPIRED,
	MAX77976_CHARGING_SUSPENDED_JEITA,
	MAX77976_CHARGING_SUSPENDED_THM_REMOVAL,
	MAX77976_CHARGING_SUSPENDED_PIN,
	MAX77976_CHARGING_RESERVED_0F,
};

/* CHG_DETAILS_01.BAT_DTLS values */
enum max77976_battery_state {
	MAX77976_BATTERY_BATTERY_REMOVAL = 0x0,
	MAX77976_BATTERY_PREQUALIFICATION,
	MAX77976_BATTERY_TIMER_FAULT,
	MAX77976_BATTERY_REGULAR_VOLTAGE,
	MAX77976_BATTERY_LOW_VOLTAGE,
	MAX77976_BATTERY_OVERVOLTAGE,
	MAX77976_BATTERY_RESERVED,
	MAX77976_BATTERY_BATTERY_ONLY, // No valid adapter is present
};

/* CHG_CNFG_00.MODE values */
enum max77976_mode {
	MAX77976_MODE_CHARGER_BUCK		= 0x5,
	MAX77976_MODE_BOOST			= 0x9,
};

/* CHG_CNFG_02.CHG_CC: charge current limit, 100..5500 mA, 50 mA steps */
#define MAX77976_CHG_CC_STEP			  50000U
#define MAX77976_CHG_CC_MIN			 100000U
#define MAX77976_CHG_CC_MAX			5500000U

/* CHG_CNFG_09.CHGIN_ILIM: input current limit, 100..3200 mA, 100 mA steps */
#define MAX77976_CHGIN_ILIM_STEP		 100000U
#define MAX77976_CHGIN_ILIM_MIN			 100000U
#define MAX77976_CHGIN_ILIM_MAX			3200000U

 /* --------------------------------------------------------------------------
  * Data structures
  */
struct max77976_config {
    struct i2c_dt_spec i2c;
};

struct max77976_data {
    uint8_t a;
};

/* --------------------------------------------------------------------------
 * power_supply properties
*/
static int max77976_get_status(const struct device *dev, int *val)
 {
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_DETAILS_01;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);
    read_buff[0] = read_buff[0] & 0x0F;

    if(err < 0) 
    {
        return err;
    }
    switch (read_buff[0]) 
    {
        case MAX77976_CHARGING_PREQUALIFICATION:
        case MAX77976_CHARGING_FAST_CONST_CURRENT:
        case MAX77976_CHARGING_FAST_CONST_VOLTAGE:
        case MAX77976_CHARGING_TOP_OFF:
            *val = CHARGER_STATUS_CHARGING;
            break;
        case MAX77976_CHARGING_DONE:
            *val = CHARGER_STATUS_FULL;
            break;
        case MAX77976_CHARGING_TIMER_FAULT:
        case MAX77976_CHARGING_SUSPENDED_QBATT_OFF:
        case MAX77976_CHARGING_SUSPENDED_JEITA:
        case MAX77976_CHARGING_SUSPENDED_THM_REMOVAL:
        case MAX77976_CHARGING_SUSPENDED_PIN:
            *val = CHARGER_STATUS_NOT_CHARGING;
            break;
        case MAX77976_CHARGING_OFF:
        case MAX77976_CHARGING_THERMAL_SHUTDOWN:
        case MAX77976_CHARGING_WATCHDOG_EXPIRED:
            *val = CHARGER_STATUS_DISCHARGING;
            break;
        default:
            *val = CHARGER_STATUS_UNKNOWN;
    }
    return 0;


 }

static int max77976_get_charge_type(const struct device *dev, int *val)
{
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_DETAILS_01;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);
    read_buff[0] = read_buff[0] & 0x0F;

    if(err < 0) 
    {
        return err;
    }

    switch (read_buff[0]) 
    {
        case MAX77976_CHARGING_PREQUALIFICATION:
            *val = CHARGER_CHARGE_TYPE_TRICKLE;
            break;
        case MAX77976_CHARGING_FAST_CONST_CURRENT:
        case MAX77976_CHARGING_FAST_CONST_VOLTAGE:
            *val = CHARGER_CHARGE_TYPE_FAST;
            break;
        case MAX77976_CHARGING_TOP_OFF:
            *val = CHARGER_CHARGE_TYPE_STANDARD;
            break;
        case MAX77976_CHARGING_DONE:
        case MAX77976_CHARGING_TIMER_FAULT:
        case MAX77976_CHARGING_SUSPENDED_QBATT_OFF:
        case MAX77976_CHARGING_OFF:
        case MAX77976_CHARGING_THERMAL_SHUTDOWN:
        case MAX77976_CHARGING_WATCHDOG_EXPIRED:
        case MAX77976_CHARGING_SUSPENDED_JEITA:
        case MAX77976_CHARGING_SUSPENDED_THM_REMOVAL:
        case MAX77976_CHARGING_SUSPENDED_PIN:
            *val = CHARGER_CHARGE_TYPE_NONE;
            break;
        default:
            *val = CHARGER_CHARGE_TYPE_UNKNOWN;
    }
    
    return 0;

}

static int max77976_get_health(const struct device *dev, int *val)
{
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_DETAILS_01;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);
    if(err < 0) 
    {
        return err;
    }

    switch (read_buff[0]) 
    {
        case MAX77976_BATTERY_BATTERY_REMOVAL:
            *val = CHARGER_HEALTH_NO_BATTERY;
            break;
        case MAX77976_BATTERY_LOW_VOLTAGE:
        case MAX77976_BATTERY_REGULAR_VOLTAGE:
            *val = CHARGER_HEALTH_GOOD;
            break;
        case MAX77976_BATTERY_TIMER_FAULT:
            *val = CHARGER_HEALTH_SAFETY_TIMER_EXPIRE;
            break;
        case MAX77976_BATTERY_OVERVOLTAGE:
            *val = CHARGER_HEALTH_OVERVOLTAGE;
            break;
        case MAX77976_BATTERY_PREQUALIFICATION:
        case MAX77976_BATTERY_BATTERY_ONLY:
            *val = CHARGER_HEALTH_UNKNOWN;
            break;
        default:
            *val = CHARGER_HEALTH_UNKNOWN;
    }
    
        return 0;
}

static int max77976_get_online(const struct device *dev, int *val)
{
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_DETAILS_01;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);

    read_buff[0] = (read_buff[0] >> 6) & 0x01;

    if(err < 0) 
    {
        return err;
    }
    *val = (read_buff[0] ? 1 : 0);

}

// .................................................................................
// Reads reg value
// (Modified this so it just returns the actual register value)
// .................................................................................
static int max77976_get_input_reg_current(const struct device *dev , int *val)
{
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_CNFG_09;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);
    read_buff[0] = read_buff[0] & 0x3F;

//    if(read_buff[0] > 2)
//    {
//        *val = 100000;
//    }
//    else
//    {
//        *val = read_buff[0] * 50000;
//    }
    *val = read_buff[0];
    return err;
}

// .................................................................................
// Our code already divides the CC (mA) value by 50, so here, val is 0..255
// corresponding to the coded values (e.g. val = 0x14 = 1000 mA)
// .................................................................................
static int max77976_set_input_reg_current(const struct device *dev, int *val)
{
    int err;
    uint8_t buf[2];

    const struct max77976_config *cfg = dev->config;

    buf[0] = CHG_CNFG_09;
    buf[1] = *val;

    err = i2c_write_dt(&cfg->i2c, buf, 2);
    return err;
}
// .................................................................................

// .................................................................................
// Reads reg value
// (Modified this so it just returns the actual register value)
// .................................................................................
static int max77976_get_charge_control_limit(const struct device *dev, int *val)
{
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_CNFG_02;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);
    read_buff[0] = read_buff[0] & 0x7F;

//    if(read_buff[0] < 2)
//    {
//        *val = 100000;
//    }
//    else
//    {
//        *val = read_buff[0] * 50000;
//    }
    *val = read_buff[0];
    return err;
}

// .................................................................................
// Our code already divides the CC (mA) value by 50, so here, val is 0..255
// corresponding to the coded values (e.g. val = 0x14 = 1000 mA)
// .................................................................................
static int max77976_set_charge_control_limit(const struct device *dev, int *val)
{
    int err;
    uint8_t buf[2];

    const struct max77976_config *cfg = dev->config;

    buf[0] = CHG_CNFG_02;
    buf[1] = *val;

    err = i2c_write_dt(&cfg->i2c, buf, 2);
    return err;

}

// .................................................................................
// Determine if charger is receiving power in, by reading the 
// CHGIN_OK bit (6) of register CHG_INT_OK
// Example results: CHARGER_ONLINE_OFFLINE, CHARGER_ONLINE_FIXED
// .................................................................................
static int max77976_get_charge_in(const struct device *dev, int *val)
{
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_INT_OK;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);

    read_buff[0] = (read_buff[0] >> 6) & 0x01;

    if(err < 0) 
    {
        return err;
    }
    *val = (read_buff[0] ? 1 : 0);

}

// .................................................................................
// Lock / Unlock Charger Protection
// 0 = Unlock (write 0x0C to register bits)
// 1 = Lock (write 0x00 to register bits)
// .................................................................................
static int max77976_set_charger_protection(const struct device *dev, int *val)
{
    int err;
    uint8_t buf[2];
    int reg;
    const struct max77976_config *cfg = dev->config;

    if ((*val != 0) && (*val != 1))
    {
        // Only accept 0 (unlock) and 1 (lock)
        return -1;
    }
    buf[0] = CHG_CNFG_06;
    buf[1] = (*val == 0) ? 0x0C : 0x00;

    err = i2c_write_dt(&cfg->i2c, buf, 2);
    return err;
}
// .................................................................................
// Lock / Unlock Charger Protection
// 0 = Unlock, 1 = Lock
// .................................................................................
static int max77976_set_fast_charge_current(const struct device *dev, int *val)
{
    int err;
    uint8_t buf[2];
    int reg;
    const struct max77976_config *cfg = dev->config;

    if ((*val != 0) && (*val != 1))
    {
        // Only accept 0 (unlock) and 1 (lock)
        return -1;
    }
    buf[0] = CHG_CNFG_06;
    buf[1] = (*val == 0) ? 0x00 : 0x0C;

    err = i2c_write_dt(&cfg->i2c, buf, 2);
    return err;
}
// .................................................................................
// Reads reg value
// .................................................................................
static int max77976_get_CHG_CNFG_04(const struct device *dev , int *val)
{
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_CNFG_04;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);
    read_buff[0] = read_buff[0] & 0x3F;

    *val = read_buff[0];
    return err;
}

// .................................................................................
// Set the Temrination Voltage
// .................................................................................
static int max77976_set_termination_voltage(const struct device *dev, int *val)
{
    int err;
    uint8_t buf[2];
    const struct max77976_config *cfg = dev->config;

    buf[0] = CHG_CNFG_04;
    buf[1] = 0x1C | 0x20;     // this will set the limit to 4.20V

    err = i2c_write_dt(&cfg->i2c, buf, 2);
    return err;
}

// .................................................................................
// Set the max charging current
// val = max charging current (mA)
// .................................................................................
static int max77976_set_CC(const struct device *dev, int *val)
{
    int err, buf[2];
    int lock, max_cc, temp;
    const struct max77976_config *cfg = dev->config;

    if (*val < 100) {
        val = 100;
    }
    if (*val > 3500) {
        val = 3500;
    }

    // 1. Charger Protection - Unlock: (CHG_CNFG_06) reg 0x1C, bits 2,3
    lock = 0;
    max77976_set_charger_protection(dev, &lock);

    // 2. Fast Charging current: (CHG_CNFG_02) reg 0x18
    max_cc = *val/50;
    max77976_set_charge_control_limit(dev, &max_cc);

    // 2b. Verify
//    max77976_get_charge_control_limit(dev, &temp);        // !dbg!

    // 3. Input Limit: (CHG_CNFG_09) reg 0x1F
    if (max_cc < 0x4D) {
        max_cc += 20;       // Add 1 amp
    }
    max77976_set_input_reg_current(dev, &max_cc);

    // 3b. Verify
//    max77976_get_input_reg_current(dev, &temp);        // !dbg!

    // 4. Termination Voltage: (CHG_CNFG_04) reg 0x1C, bit 6
    max77976_set_termination_voltage(dev, &max_cc);

    // 4b. Verify
//    max77976_get_CHG_CNFG_04(dev, &temp);        // !dbg!

    // 5. Charger Protection - Lock: (CHG_CNFG_06) reg 0x1C, bits 2,3)
    lock = 1;
    max77976_set_charger_protection(dev, &lock); //
}
// .................................................................................
// Read register CHG_CNFG_00
// CHGIN_OK bit (6) of register CHG_INT_OK
// Example results: CHARGER_ONLINE_OFFLINE, CHARGER_ONLINE_FIXED
// .................................................................................
static int max77976_get_CONFIG_00(const struct device *dev, int *val)
{
    int err;

    uint8_t write_buff[1];
    uint8_t read_buff[1];

    write_buff[0] = CHG_CNFG_00;

    const struct max77976_config *cfg = dev->config;

    err = i2c_write_read_dt(&cfg->i2c, write_buff, 1, read_buff, 1);

    if(err < 0) 
    {
        return err;
    }
    *val = read_buff[0];

}
// .................................................................................
// Set the mode
// 0x04 (default)
// 0x05 (charged from wall)
// 0x09 (chaged from battery)
// First, read the reg, keep the upper 4 bits, and OR in the mode to the lower 4 bits
// .................................................................................
static int max77976_set_mode(const struct device *dev, int val)
{
    int err, old;
    uint8_t buf[2];
    
    if (val > 0x0A)
    {
        return -1;
    }

    const struct max77976_config *cfg = dev->config;

    err = max77976_get_CONFIG_00(dev, &old);

    old = old & 0xF0;

    buf[0] = CHG_CNFG_00;
    buf[1] = old | (val & 0x0F);

    err = i2c_write_dt(&cfg->i2c, buf, 2);
    return err;


}
// .................................................................................

static int max77976_get_property(const struct device *dev, const charger_prop_t prop, const union charger_propval *val)
{
    int err;
    switch (prop) {
        case CHARGER_PROP_STATUS:
            err = max77976_get_status(dev, &val->status);
            break;
        case CHARGER_PROP_CHARGE_TYPE:
            err = max77976_get_charge_type(dev, &val->charge_type);
            break;
        case CHARGER_PROP_HEALTH:
            err = max77976_get_health(dev, &val->health);
            break;
        case CHARGER_PROP_ONLINE:
//            err = max77976_get_online(dev, &val->online);
            // Calling this instead, because want to determine if charger is receiving power in,
            // by reading the CHGIN_OK bit (6) of register CHG_INT_OK
            err = max77976_get_charge_in(dev, &val->online);
            break;
        case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA :
            err = max77976_get_charge_control_limit(dev, &val->const_charge_current_ua);
            break;
        case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA :
            err = max77976_get_input_reg_current(dev, &val->input_current_regulation_current_ua);
            break;
        default:
            err = -EINVAL;
        }
    
        return err;
}

static int max77976_set_property(const struct device *dev, const charger_prop_t prop, const union charger_propval *val)
{
    int err;

    switch (prop) {
    case CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA:
        err = max77976_set_charge_control_limit(dev, &val->const_charge_current_ua);
        break;
    case CHARGER_PROP_INPUT_REGULATION_CURRENT_UA :
        err = max77976_set_input_reg_current(dev, &val->input_current_regulation_current_ua);
        break;
    case CHARGER_PROP_CUSTOM_BEGIN :
        err = max77976_set_CC(dev, &val->const_charge_current_ua);
        break;
    case CHARGER_PROP_CHARGE_TYPE:
        err = max77976_set_mode(dev, val->charge_type);
    default:
        err = -EINVAL;
}

return err;
};

static int max77976_init(const struct device *dev)
{
    return 0;
}

static const struct charger_driver_api max77976_driver_api = {
	.get_property = max77976_get_property,
	.set_property = max77976_set_property,
};

#define MAX77976_INIT(inst)                                                                         \
                                                                                                   \
	static const struct max77976_config max77976_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                     \
                                                                                           \
	static struct max77976_data max77976_data_##inst;                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max77976_init, NULL, &max77976_data_##inst,                      \
			      &max77976_config_##inst, POST_KERNEL, CONFIG_CHARGER_INIT_PRIORITY,   \
			      &max77976_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX77976_INIT)