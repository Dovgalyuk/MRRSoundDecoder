#ifndef DRV8235_H
#define DRV8235_H

/* ========================================================================== */
/*                             REGISTER MAP OFFSETS                           */
/* ========================================================================== */

#define DRV8235_REG_FAULT            0x00  /*!< Status: Latched and real-time faults (RO) */
#define DRV8235_REG_RC_STATUS1       0x01  /*!< Status: Estimated motor ripple speed (RO) */
#define DRV8235_REG_RC_STATUS2       0x02  /*!< RSVD */
#define DRV8235_REG_RC_STATUS3       0x03  /*!< RSVD */
#define DRV8235_REG_REG_STATUS1      0x04  /*!< Status: Measured motor terminal voltage (RO) */
#define DRV8235_REG_REG_STATUS2      0x05  /*!< Status: Measured motor current (RO) */
#define DRV8235_REG_REG_STATUS3      0x06  /*!< Status: Internal regulation PWM duty cycle (RO) */
// #define DRV8235_REG_REG_STATUS4      0x07  /*!< RSVD */
// #define DRV8235_REG_REG_STATUS5      0x08  /*!< RSVD */

#define DRV8235_REG_CONFIG0          0x09  /*!< Config: Output enable, OVP, Stall, Clear, Duty Ctrl (RW) */
#define DRV8235_REG_CONFIG1          0x0A  /*!< Config: Inrush time lower byte [7:0] (RW) */
#define DRV8235_REG_CONFIG2          0x0B  /*!< Config: Inrush time upper byte [15:8] (RW) */
#define DRV8235_REG_CONFIG3          0x0C  /*!< Config: IMODE, SMODE, INT_VREF, TBLANK, TDEG, OCP/TSD mode (RW)* */
#define DRV8235_REG_CONFIG4          0x0D  /*!< Config: RC/Stall/CBC reports, PMODE, I2C Bridge Control (RW) */

#define DRV8235_REG_REG_CTRL0        0x0E  /*!< Control: Soft-Start, Regulation Scheme, PWM Freq, W_SCALE (RW) */
#define DRV8235_REG_REG_CTRL1        0x0F  /*!< Control: Target speed / voltage setpoint (RW) */
#define DRV8235_REG_REG_CTRL2        0x10  /*!< Control: Output voltage filter cut-off, EXT_DUTY (RW) */

// #define DRV8235_REG_RC_CTRL0         0x11  /*!< RSVD */
// #define DRV8235_REG_RC_CTRL1         0x12  /*!< RSVD */
#define DRV8235_REG_RC_CTRL2         0x13  /*!< Ripple Count: Scales (INV_R, KMC, THR) & Threshold upper bits [9:8] (RW) */
#define DRV8235_REG_RC_CTRL3         0x14  /*!< Ripple Count: Motor inverse coil resistance (INV_R) (RW) */
#define DRV8235_REG_RC_CTRL4         0x15  /*!< Ripple Count: Motor BEMF constant parameter (KMC) (RW) */
// #define DRV8235_REG_RC_CTRL5         0x16  /*!< RSVD */
// #define DRV8235_REG_RC_CTRL6         0x17  /*!< RSVD */
#define DRV8235_REG_RC_CTRL7         0x18  /*!< Speed Loop: PI Proportional Constant (KP / KP_DIV) (RW) */
#define DRV8235_REG_RC_CTRL8         0x19  /*!< Speed Loop: PI Integral Constant (KI / KI_DIV) (RW) */

#define DRV8235_REG_COUNT            0x20

/* ========================================================================== */
/*                             REGISTER BIT DEFINITIONS                       */
/* ========================================================================== */

/* FAULT Register (0x00) Bits */
#define DRV8235_FAULT_MASK_FAULT         (1U << 7)  /*!< 1 = Fault active, nFAULT pin pulled low */
#define DRV8235_FAULT_MASK_STALL         (1U << 5)  /*!< 1 = Motor stall detected */
#define DRV8235_FAULT_MASK_OCP           (1U << 4)  /*!< 1 = Overcurrent protection triggered */
#define DRV8235_FAULT_MASK_OVP           (1U << 3)  /*!< 1 = Overvoltage protection event */
#define DRV8235_FAULT_MASK_TSD           (1U << 2)  /*!< 1 = Thermal shutdown occurred */
#define DRV8235_FAULT_MASK_NPOR          (1U << 1)  /*!< 0 = Power-on-reset occurred; latched 1 after CLR_FLT */

/* CONFIG0 Register (0x09) Bits */
#define DRV8235_CONFIG0_EN_OUT           (1U << 7)  /*!< 1 = Bridge outputs enabled, 0 = Hi-Z */
#define DRV8235_CONFIG0_EN_OVP           (1U << 6)  /*!< 1 = Overvoltage active braking enabled */
#define DRV8235_CONFIG0_EN_STALL         (1U << 5)  /*!< 1 = Hardware stall detection enabled */
#define DRV8235_CONFIG0_VSNS_SEL         (1U << 4)  /*!< 1 = Digital VM*Duty filter, 0 = Analog filter (recommended) */
#define DRV8235_CONFIG0_CLR_FLT          (1U << 1)  /*!< Write 1 to clear latched fault status bits */
#define DRV8235_CONFIG0_DUTY_CTRL        (1U << 0)  /*!< 1 = Use EXT_DUTY register when regulation inactive */

/* CONFIG3 Register (0x0C) Bits */
#define DRV8235_CONFIG3_IMODE_POS        6
#define DRV8235_CONFIG3_IMODE_MASK       (0x3U << DRV8235_CONFIG3_IMODE_POS)
#define DRV8235_CONFIG3_SMODE            (1U << 5)  /*!< 0 = Latch outputs Hi-Z on stall; 1 = Continue driving */
#define DRV8235_CONFIG3_INT_VREF         (1U << 4)  /*!< 1 = Internal 3.0V VREF; 0 = External VREF pin */
#define DRV8235_CONFIG3_TBLANK           (1U << 3)  /*!< 1 = tBLANK 1.0us; 0 = tBLANK 1.8us */
#define DRV8235_CONFIG3_TDEG             (1U << 2)  /*!< 1 = tDEG 1.0us; 0 = tDEG 2.0us */
#define DRV8235_CONFIG3_OCP_MODE         (1U << 1)  /*!< 1 = Auto-retry on OCP; 0 = Latch-off */
#define DRV8235_CONFIG3_TSD_MODE         (1U << 0)  /*!< 1 = Auto-retry on TSD; 0 = Latch-off */

/* CONFIG4 Register (0x0D) Bits */
#define DRV8235_CONFIG4_STALL_REP        (1U << 5)  /*!< 1 = Pull nFAULT low on stall detection */
#define DRV8235_CONFIG4_CBC_REP          (1U << 4)  /*!< 1 = Pull nFAULT low during cycle-by-cycle regulation */
#define DRV8235_CONFIG4_PMODE            (1U << 3)  /*!< 1 = PWM Mode (IN1=PWM, IN2=DIR); 0 = PH/EN Mode */
#define DRV8235_CONFIG4_I2C_BC           (1U << 2)  /*!< 1 = Bridge control via I2C bits; 0 = Via IN1/IN2 pins */
#define DRV8235_CONFIG4_I2C_EN_IN1       (1U << 1)  /*!< Internal I2C Enable / IN1 control bit */
#define DRV8235_CONFIG4_I2C_PH_IN2       (1U << 0)  /*!< Internal I2C Phase / IN2 control bit */

/* REG_CTRL0 Register (0x0E) Bits */
#define DRV8235_REG_CTRL0_EN_SS          (1U << 5)  /*!< 1 = Enable Soft-Start and Soft-Stop */
#define DRV8235_REG_CTRL0_REG_CTRL_POS   3
#define DRV8235_REG_CTRL0_REG_CTRL_MASK  (0x3U << DRV8235_REG_CTRL0_REG_CTRL_POS)
#define DRV8235_REG_CTRL0_PWM_FREQ       (1U << 2)  /*!< 1 = 25 kHz internal PWM; 0 = 50 kHz */
#define DRV8235_REG_CTRL0_W_SCALE_POS    0
#define DRV8235_REG_CTRL0_W_SCALE_MASK   (0x3U << DRV8235_REG_CTRL0_W_SCALE_POS)

/* REG_CTRL2 Register (0x10) Bits */
#define DRV8235_REG_CTRL2_OUT_FLT_POS    6
#define DRV8235_REG_CTRL2_OUT_FLT_MASK   (0x3U << DRV8235_REG_CTRL2_OUT_FLT_POS)
#define DRV8235_REG_CTRL2_PROG_DUTY_MASK 0x3FU      /*!< External PWM Duty Cycle: 0 to 63 (0% to 100%) */

/* RC_CTRL2 Register (0x13) Bits */
#define DRV8235_RC_CTRL2_INV_R_SCALE_POS 6
#define DRV8235_RC_CTRL2_INV_R_SCALE_MASK (0x3U << DRV8235_RC_CTRL2_INV_R_SCALE_POS)
#define DRV8235_RC_CTRL2_KMC_SCALE_POS   4
#define DRV8235_RC_CTRL2_KMC_SCALE_MASK  (0x3U << DRV8235_RC_CTRL2_KMC_SCALE_POS)

#define DRV8235_RC_CTRL7_KP_DIV_POS      5

#define DRV8235_RC_CTRL8_KI_DIV_POS      5

/* ========================================================================== */
/*                             ENUMERATION TYPES                              */
/* ========================================================================== */

/**
 * @brief Current Regulation Mode (IMODE)
 */
typedef enum {
    DRV8235_IMODE_NO_REGULATION         = 0x0,
    DRV8235_IMODE_INRUSH_REGULATION     = 0x1,
    DRV8235_IMODE_ALL_TIMES_REGULATION  = 0x2,
} drv8235_imode_t;

/**
 * @brief Stall Response Mode (SMODE)
 */
typedef enum {
    DRV8235_SMODE_LATCH_HIZ = 0, /*!< Disable H-Bridge (Hi-Z) on stall until cleared */
    DRV8235_SMODE_CONTINUE  = 1  /*!< Continue driving motor during stall */
} drv8235_smode_t;

/**
 * @brief Input Interface Pin Mode (PMODE)
 */
typedef enum {
    DRV8235_PMODE_PH_EN = 0, /*!< Phase / Enable Control: IN1=Enable, IN2=Phase */
    DRV8235_PMODE_PWM   = (1 << 3)  /*!< PWM / Direction Control: IN1=PWM/Speed, IN2=Direction */
} drv8235_pmode_t;

/**
 * @brief Bridge Control Signal Source
 */
typedef enum {
    DRV8235_BC_PINS = 0, /*!< External MCU pins (IN1, IN2) control H-bridge */
    DRV8235_BC_I2C  = 1  /*!< Internal I2C register bits control H-bridge */
} drv8235_bridge_ctrl_source_t;

/**
 * @brief Closed-Loop Regulation Scheme (REG_CTRL)
 */
typedef enum {
    DRV8235_REG_SCHEME_FIXED_OFF_TIME = 0x00, /*!< Fixed off-time current regulation */
    DRV8235_REG_SCHEME_CYCLE_BY_CYCLE = 0x01, /*!< Cycle-by-cycle current regulation */
    DRV8235_REG_SCHEME_SPEED_REG      = 0x02, /*!< Closed-loop motor speed regulation (requires ripple count) */
    DRV8235_REG_SCHEME_VOLTAGE_REG    = 0x03  /*!< Closed-loop motor terminal voltage regulation */
} drv8235_reg_scheme_t;

/**
 * @brief Internal PWM Frequency Setting
 */
typedef enum {
    DRV8235_PWM_FREQ_50KHZ = 0, /*!< 50 kHz internal PWM frequency */
    DRV8235_PWM_FREQ_25KHZ = DRV8235_REG_CTRL0_PWM_FREQ  /*!< 25 kHz internal PWM frequency */
} drv8235_pwm_freq_t;

/**
 * @brief Ripple Speed Scaling Factor (W_SCALE)
 */
typedef enum {
    DRV8235_W_SCALE_16  = 0x00, /*!< 16 rad/s / LSB (Max speed: 4,080 rad/s) */
    DRV8235_W_SCALE_32  = 0x01, /*!< 32 rad/s / LSB (Max speed: 8,160 rad/s) */
    DRV8235_W_SCALE_64  = 0x02, /*!< 64 rad/s / LSB (Max speed: 16,320 rad/s) */
    DRV8235_W_SCALE_128 = 0x03  /*!< 128 rad/s / LSB (Max speed: 32,640 rad/s) */
} drv8235_w_scale_t;

/**
 * @brief Inverse Resistance Scale Factor (INV_R_SCALE)
 */
typedef enum {
    DRV8235_INV_R_SCALE_2    = 0x00, /*!< Scale 2 */
    DRV8235_INV_R_SCALE_64   = 0x01, /*!< Scale 64 */
    DRV8235_INV_R_SCALE_1024 = 0x02, /*!< Scale 1024 */
    DRV8235_INV_R_SCALE_8192 = 0x03  /*!< Scale 8192 */
} drv8235_inv_r_scale_t;

/**
 * @brief Motor BEMF Constant Scale Factor (KMC_SCALE)
 */
typedef enum {
    DRV8235_KMC_SCALE_24X2_8  = 0x00, /*!< 24 * 2^8 */
    DRV8235_KMC_SCALE_24X2_9  = 0x01, /*!< 24 * 2^9 */
    DRV8235_KMC_SCALE_24X2_12 = 0x02, /*!< 24 * 2^12 */
    DRV8235_KMC_SCALE_24X2_13 = 0x03  /*!< 24 * 2^13 */
} drv8235_kmc_scale_t;

/**
 * @brief Output Voltage Filter Cut-off Frequency (OUT_FLT)
 */
typedef enum {
    DRV8235_OUT_FLT_250HZ  = 0x00, /*!< 250 Hz cut-off */
    DRV8235_OUT_FLT_500HZ  = 0x01, /*!< 500 Hz cut-off */
    DRV8235_OUT_FLT_750HZ  = 0x02, /*!< 750 Hz cut-off */
    DRV8235_OUT_FLT_1000HZ = 0x03  /*!< 1000 Hz cut-off (Recommended for >=20kHz PWM) */
} drv8235_out_flt_t;

/**
 * @brief Motor Direction / Motion State Command
 */
typedef enum {
    DRV8235_MOTOR_COAST    = 0, /*!< Outputs Hi-Z (Coast to stop) */
    DRV8235_MOTOR_FORWARD  = 1, /*!< OUT1=High, OUT2=Low (Forward drive) */
    DRV8235_MOTOR_REVERSE  = 2, /*!< OUT1=Low, OUT2=High (Reverse drive) */
    DRV8235_MOTOR_BRAKE    = 3  /*!< OUT1=Low, OUT2=Low (Active low-side brake) */
} drv8235_motor_state_t;

/**
 * @brief Proportional constant in PI control loop
 */
typedef enum {
    DRV8235_KP_DIV_32   = 0,
    DRV8235_KP_DIV_64   = 1,
    DRV8235_KP_DIV_128  = 2,
    DRV8235_KP_DIV_256  = 3,
    DRV8235_KP_DIV_512  = 4,
    DRV8235_KP_DIV_16   = 5,
    DRV8235_KP_DIV_1    = 6,
} drv8235_kp_div;

/**
 * @brief Integral constant in PI control loop
 */
typedef enum {
    DRV8235_KI_DIV_32   = 0,
    DRV8235_KI_DIV_64   = 1,
    DRV8235_KI_DIV_128  = 2,
    DRV8235_KI_DIV_256  = 3,
    DRV8235_KI_DIV_512  = 4,
    DRV8235_KI_DIV_16   = 5,
    DRV8235_KI_DIV_1    = 6,
} drv8235_ki_div;

void drv8235_init(void);
void drv8235_set_speed(uint8_t speed);
void drv8235_set_motor_state(drv8235_motor_state_t state);
uint8_t drv8235_get_speed(void);
uint8_t drv8235_read_reg(uint8_t reg);

#endif
