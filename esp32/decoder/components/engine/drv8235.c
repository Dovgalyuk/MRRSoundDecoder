#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "drv8235.h"
#include "logger.h"
#include "pins.h"
#include "cv.h"

static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t i2c_motor_handle;
static uint8_t reg_cache[DRV8235_REG_COUNT];

static void drv8235_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t data_wr[2] = {reg, val};
    int err = i2c_master_transmit(i2c_motor_handle, data_wr, 2, 10);
    if (err) {
        logger_printf("drv8235 write error %d reg=0x%x val=0x%x", err, reg, val);
    } else {
        reg_cache[reg] = val;
    }
}

uint8_t drv8235_read_reg(uint8_t reg)
{
    uint8_t val = 0;
    int err = i2c_master_transmit_receive(i2c_motor_handle, &reg, 1, &val, 1, 10);
    if (err) {
        /* Sometimes bus error occur for speed read operation */
        val = reg_cache[reg];
    } else {
        reg_cache[reg] = val;
    }
    return val;
}

static void drv8235_modify_reg(uint8_t reg, uint8_t clear_mask, uint8_t set_mask)
{
    uint8_t val = drv8235_read_reg(reg);
    uint8_t new_val = (val & ~clear_mask) | set_mask;

    if (new_val != val) {
        drv8235_write_reg(reg, new_val);
    }
}

static void drv8235_enable_outputs(bool enable)
{
    drv8235_modify_reg(DRV8235_REG_CONFIG0, DRV8235_CONFIG0_EN_OUT,
                       enable ? DRV8235_CONFIG0_EN_OUT : 0);
}

void drv8235_init(void)
{
    /* Initialize I2C for driver */
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = MOTOR_SCL,
        .sda_io_num = MOTOR_SDA,
        .glitch_ignore_cnt = 7,
    };

    LOGGER_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x30,
        .scl_speed_hz = 50000, /* Max is 100kHz for DRV8235 */
    };
    LOGGER_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &i2c_motor_handle));

    /* Fault pin */
    gpio_config_t io_conf_outputs_motor = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MOTOR_INPUT_FAULT),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    LOGGER_ERROR_CHECK(gpio_config(&io_conf_outputs_motor));

    /* Ensure outputs are disabled (EN_OUT = 0) so protected registers are writable */
    // Default is ok
    // drv8235_write_reg(DRV8235_REG_CONFIG0, 0);
    //        DRV8234_CONFIG0_EN_OVP | DRV8234_CONFIG0_EN_STALL);

    /* Configure Inrush Time (CONFIG1 & CONFIG2) */
    // default is 5ms
    // time = delta_speed * value * 102.4us
    drv8235_write_reg(DRV8235_REG_CONFIG2, 0x10);

    /* Configure Operating & Protection Modes (CONFIG3) */
    // default is ok
    // drv8235_write_reg(DRV8235_REG_CONFIG3,
    //     /* imode */
    //     // no regulation
    //     (DRV8235_IMODE_ALL_TIMES_REGULATION << DRV8235_CONFIG3_IMODE_POS)
    //     /* Stall detection enabled */
    //     | DRV8235_CONFIG3_SMODE
    //     /* Auto-retry in case of overcurrent */
    //     | DRV8235_CONFIG3_OCP_MODE
    //     /* Auto-retry in case of overtemperature */
    //     | DRV8235_CONFIG3_TSD_MODE);

    /* Configure Control Interface & Fault Reporting (CONFIG4) */
    drv8235_write_reg(DRV8235_REG_CONFIG4,
        DRV8235_CONFIG4_STALL_REP
        | DRV8235_CONFIG4_CBC_REP
        | DRV8235_PMODE_PH_EN
        /* Drive motor through I2C */
        | DRV8235_CONFIG4_I2C_BC);

    /* Configure Regulation & Scaling (REG_CTRL0) */
    drv8235_write_reg(DRV8235_REG_REG_CTRL0,
        /* Soft start/stop */
        DRV8235_REG_CTRL0_EN_SS
        /* Motor speed is regulated at 50kHz*/
        | (DRV8235_REG_SCHEME_SPEED_REG << DRV8235_REG_CTRL0_REG_CTRL_POS)
        | DRV8235_PWM_FREQ_50KHZ
        /* TODO */
        | DRV8235_W_SCALE_16);

    /* Speed = 0 (REG_CTRL1) */
    drv8235_write_reg(DRV8235_REG_REG_CTRL1, 0);

    /* Default REG_CTRL2 */

    /* RC_CTRL2 */
    // Default INV_R_SCALE = 64
    // Default KMC_SCALE = 24x2^13
    drv8235_write_reg(DRV8235_REG_RC_CTRL3,
        (DRV8235_INV_R_SCALE_1024 << DRV8235_RC_CTRL2_INV_R_SCALE_POS)
        | (cv_read(CV_KMC_SCALE) << DRV8235_RC_CTRL2_KMC_SCALE_POS));

    /* INV_R (RC_CTRL3) */
    /* INV_R = INV_R_SCALE / MotorResistance */
    drv8235_write_reg(DRV8235_REG_RC_CTRL3, 1024 / 25);

    /* KMC (RC_CTRL4) */
    /* KMC = Kv * KMC_SCALE / Nr */
    drv8235_write_reg(DRV8235_REG_RC_CTRL4, cv_read(CV_KMC));

    /* KP (RC_CTRL7) */
    // default 1/64
    drv8235_write_reg(DRV8235_REG_RC_CTRL7,
        (DRV8235_KP_DIV_1 << DRV8235_RC_CTRL7_KP_DIV_POS)
        | 31); /* mult */

    /* KI (RC_CTRL8) */
    // default 1/64
    drv8235_write_reg(DRV8235_REG_RC_CTRL8,
        (DRV8235_KI_DIV_1 << DRV8235_RC_CTRL8_KI_DIV_POS)
        | 31); /* mult */

    /* Read all */
    //logger_printf("%lld", pdMS_TO_TICKS(1));
    // 8589934593 = 0x200000001
}

void drv8235_set_speed(uint8_t speed)
{
    /* Configure Regulation & Scaling (REG_CTRL0) */
    drv8235_modify_reg(DRV8235_REG_REG_CTRL0, DRV8235_REG_CTRL0_W_SCALE_MASK,
        cv_read(CV_W_SCALE) << DRV8235_REG_CTRL0_W_SCALE_POS);

    drv8235_write_reg(DRV8235_REG_REG_CTRL1, speed);
}

uint8_t drv8235_get_speed(void)
{
    return drv8235_read_reg(DRV8235_REG_RC_STATUS1);
}

void drv8235_set_motor_state(drv8235_motor_state_t state)
{
    /* For speed control mode and I2C */
    uint8_t bits = 0;
    switch (state) {
    case DRV8235_MOTOR_FORWARD:
        bits = DRV8235_CONFIG4_I2C_EN_IN1 | DRV8235_CONFIG4_I2C_PH_IN2; /* IN1=1, IN2=1 */
        drv8235_enable_outputs(true);
        break;
    case DRV8235_MOTOR_REVERSE:
        bits = DRV8235_CONFIG4_I2C_EN_IN1; /* IN1=1, IN2=0 */
        drv8235_enable_outputs(true);
        break;
    case DRV8235_MOTOR_BRAKE:
        bits = 0; /* IN1=0, IN2=0 */
        drv8235_enable_outputs(true);
        break;
    case DRV8235_MOTOR_COAST:
    default:
        bits = 0;
        drv8235_enable_outputs(false);
        break;
    }

    return drv8235_modify_reg(DRV8235_REG_CONFIG4,
                              DRV8235_CONFIG4_I2C_EN_IN1 | DRV8235_CONFIG4_I2C_PH_IN2,
                              bits);
}
