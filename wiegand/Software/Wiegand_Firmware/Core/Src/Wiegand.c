#include "wiegand.h"

/* ================== delay_us bằng DWT (Cortex-M3) ================== */

static uint8_t dwt_initialized = 0;

static void DWT_Delay_Init(void)
{
    if (dwt_initialized) return;

    /* Enable TRC */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Reset cycle counter */
    DWT->CYCCNT = 0;
    /* Enable cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    dwt_initialized = 1;
}

void delay_us(uint32_t us)
{
    if (!dwt_initialized)
    {
        DWT_Delay_Init();
    }

    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (SystemCoreClock / 1000000U) * us;

    while ((DWT->CYCCNT - start) < ticks)
    {
        /* wait */
    }
}

/* ================== Hàm phụ tính parity ================== */

// Parity cho 12 bit (dùng cho Wiegand-26)
static uint8_t parity_even_12(uint16_t val)
{
    val &= 0x0FFFU;  // chỉ giữ 12 bit
    uint8_t p = 0;
    for (uint8_t i = 0; i < 12; i++)
    {
        p ^= (val >> i) & 0x1U;
    }
    return p & 0x1U;   // 1 nếu số bit '1' lẻ
}

// Parity cho 16 bit (dùng cho Wiegand-34)
static uint8_t parity_even_16(uint16_t val)
{
    uint8_t p = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        p ^= (val >> i) & 0x1U;
    }
    return p & 0x1U;   // 1 nếu số bit '1' lẻ
}

/* ================== API Wiegand chung ================== */

void Wiegand_Init(void)
{
    // Đảm bảo D0/D1 ở mức HIGH (idle)
    HAL_GPIO_WritePin(WIEGAND_D0_GPIO_Port, WIEGAND_D0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(WIEGAND_D1_GPIO_Port, WIEGAND_D1_Pin, GPIO_PIN_SET);
}

/**
 * @brief Gửi 1 bit Wiegand:
 *        - bit 0: kéo D0 xuống thấp
 *        - bit 1: kéo D1 xuống thấp
 */
void Wiegand_SendBit(uint8_t bit)
{
    if (bit == 0)
    {
        HAL_GPIO_WritePin(WIEGAND_D0_GPIO_Port, WIEGAND_D0_Pin, GPIO_PIN_RESET);
        delay_us(WIEGAND_PULSE_US);
        HAL_GPIO_WritePin(WIEGAND_D0_GPIO_Port, WIEGAND_D0_Pin, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(WIEGAND_D1_GPIO_Port, WIEGAND_D1_Pin, GPIO_PIN_RESET);
        delay_us(WIEGAND_PULSE_US);
        HAL_GPIO_WritePin(WIEGAND_D1_GPIO_Port, WIEGAND_D1_Pin, GPIO_PIN_SET);
    }

    delay_us(WIEGAND_INTERVAL_US);
}

/* ================== Wiegand-26 ================== */
/* Format H10301: [P_even][8-bit facility][16-bit card][P_odd] */

/**
 * @brief Gửi 26-bit frame chuẩn:
 *        [P_even][24 bit data][P_odd]
 *        Trong đó:
 *        - P_even: even parity cho 12 bit đầu của data
 *        - P_odd : odd  parity cho 12 bit cuối của data
 */
void Wiegand_Send26_Raw(uint32_t data24)
{
    data24 &= 0xFFFFFFUL;  // chỉ giữ 24 bit

    uint16_t first12 = (uint16_t)((data24 >> 12) & 0x0FFFU);
    uint16_t last12  = (uint16_t)(data24 & 0x0FFFU);

    // parity_even_12 trả 1 nếu số bit '1' lẻ
    uint8_t p_even = parity_even_12(first12);          // even parity
    uint8_t p_odd  = parity_even_12(last12) ? 0 : 1;   // odd parity

    // Gửi parity đầu
    Wiegand_SendBit(p_even);

    // Gửi 24 bit data (MSB trước)
    for (int8_t i = 23; i >= 0; i--)
    {
        uint8_t b = (data24 >> i) & 0x1U;
        Wiegand_SendBit(b);
    }

    // Gửi parity cuối
    Wiegand_SendBit(p_odd);
}

/**
 * @brief Gửi chuẩn Wiegand-26 kiểu H10301:
 *        [1 bit even] [8-bit facility] [16-bit card] [1 bit odd]
 */
void Wiegand_Send26(uint8_t facility, uint16_t card)
{
    uint32_t data24 = ((uint32_t)facility << 16) | (uint32_t)card;
    Wiegand_Send26_Raw(data24);
}

/**
 * @brief Map trực tiếp UID[4] (Mifare 4 byte) sang Wiegand-26
 *        8 bit facility  = uid[1]
 *        16 bit card     = (uid[2] << 8) | uid[3]
 */
void Wiegand_Send26_FromUID(uint8_t uid[4])
{
    uint8_t facility  = uid[1];
    uint16_t card     = ((uint16_t)uid[2] << 8) | (uint16_t)uid[3];

    Wiegand_Send26(facility, card);
}

/* ================== Wiegand-34 ================== */
/* Format phổ biến: [P_even][32-bit data][P_odd]
 *  - P_even: even parity cho 16 bit cao
 *  - P_odd : odd  parity cho 16 bit thấp
 */

/**
 * @brief Gửi frame Wiegand-34:
 *        [P_even][32-bit data][P_odd]
 */
void Wiegand_Send34_Raw(uint32_t data32)
{
    uint16_t first16 = (uint16_t)((data32 >> 16) & 0xFFFFU);
    uint16_t last16  = (uint16_t)(data32 & 0xFFFFU);

    uint8_t p_even = parity_even_16(first16);          // even parity
    uint8_t p_odd  = parity_even_16(last16) ? 0 : 1;   // odd parity

    // Gửi parity đầu
    Wiegand_SendBit(p_even);

    // Gửi 32 bit data (MSB trước)
    for (int8_t i = 31; i >= 0; i--)
    {
        uint8_t b = (data32 >> i) & 0x1U;
        Wiegand_SendBit(b);
    }

    // Gửi parity cuối
    Wiegand_SendBit(p_odd);
}

/**
 * @brief Dùng full UID[4] (4 byte) -> Wiegand-34
 *        data32 = (uid[0]<<24) | (uid[1]<<16) | (uid[2]<<8) | uid[3]
 */
void Wiegand_Send34_FromUID(uint8_t uid[4])
{
    uint32_t data32 =  ((uint32_t)uid[0] << 24) |
                       ((uint32_t)uid[1] << 16) |
                       ((uint32_t)uid[2] << 8)  |
                       ((uint32_t)uid[3]);

    Wiegand_Send34_Raw(data32);
}
