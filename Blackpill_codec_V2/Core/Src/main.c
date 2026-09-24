/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sine_table.h"
#include "waveforms.h"
#include "oled.h"
#include "keypad.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {

	LEFT,
	RIGHT

}canal_e;

typedef enum {

	OFF,
	ON

}estado_e;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FS           48828.125         // fs REAL de la V2 (I2SCLK 100MHz / 2048)

#define FRAMES_HALF  256               // frames estéreo por media vuelta del DMA
#define FRAMES_TOTAL (2 * FRAMES_HALF) // frames en TODO el buffer (512)
#define BUF_HW       (FRAMES_TOTAL * 4)// halfwords totales: 2 canales x 2 hw c/u (2048)
#define I2S_SIZE     (FRAMES_TOTAL * 2)// "muestras de 32 bits" que le pasamos a la HAL (1024)

#define AMP_MAX      2048              // amplitud plena (la fracción es amp/2048)
#define AMP_SHIFT    11                // 2^11 = 2048  (para dividir con un shift)
#define AMP_STEP     256               // cuánto sube/baja la amplitud por tecla (8 pasos)

#define PCM_ADDR     (0x46 << 1)       // dirección I2C del codec: 7 bits corridos a 8
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_tx;

/* USER CODE BEGIN PV */
uint32_t          acc[2] = {0, 0};              // acumuladores de fase (uno por canal)
volatile uint32_t ftw[2] = {0, 0};              // FTW de cada canal (lo cambia el parser)
volatile uint8_t  salida_estado[2] = {ON, ON};
volatile int32_t  amp[2] = {AMP_MAX, AMP_MAX};  // amplitud de cada canal
volatile uint8_t  canal_seleccionado = LEFT;                    // canal "activo" (el que edita el teclado)
uint16_t          audio_buf[BUF_HW];            // el buffer circular del DMA
volatile uint8_t  modo_diferencial = 1;         // 1 = salida diferencial, 0 = simple (single-ended)

// Parametros que llegan desde el USB o el teclado (uno por canal)
extern float frecuencia[2];
extern uint32_t amplitud[2];
extern volatile uint8_t salida_activa[2];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S2_Init(void);
/* USER CODE BEGIN PFP */
static void fill(uint16_t *dst);
static void pcm_write(uint8_t reg, uint8_t val);
void PCM3060_SetAmplitude(uint8_t canal, uint32_t amplitud);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S2_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  // Frecuencia inicial de cada canal
  ftw[0] = (uint32_t)(frecuencia[0] * 4294967296.0 / FS);
  ftw[1] = (uint32_t)(frecuencia[1] * 4294967296.0 / FS);

  // Precargo las DOS mitades ANTES de largar el DMA
  fill(&audio_buf[0]);
  fill(&audio_buf[BUF_HW / 2]);

  // Largo el I²S por DMA en modo circular (arranca y no para nunca)
  HAL_I2S_Transmit_DMA(&hi2s2, audio_buf, I2S_SIZE);

  // Le doy tiempo al codec a estabilizar sus relojes internos
  HAL_Delay(50);

  // Despierto el codec y subo volúmenes
  pcm_write(64, 0xE0);   // reg 64: saca el DAC de power-save (ON, salida DIFERENCIAL)
  pcm_write(65, 0xFF);   // reg 65: volumen DAC L = 0 dB
  pcm_write(66, 0xFF);   // reg 66: volumen DAC R = 0 dB

  // --- OLED + chirp por defecto ---
  oled_init();
  chirp_config(0, 200.0f, 2000.0f, 1.0f);   // chirp por defecto: 200 Hz -> 2 kHz en 1 s
  chirp_config(1, 200.0f, 2000.0f, 1.0f);
  keypad_init();                            // dejo las filas del teclado en alto
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

   // --- Actualizacion de frecuencia y amplitud (vienen del USB o del teclado) ---
   ftw[0] = (uint32_t)(frecuencia[0] * 4294967296.0 / FS);
   ftw[1] = (uint32_t)(frecuencia[1] * 4294967296.0 / FS);
   PCM3060_SetAmplitude(0, amplitud[0]);   // volumen del canal L (registro del codec)
   PCM3060_SetAmplitude(1, amplitud[1]);   // volumen del canal R (registro del codec)

   // --- modo de salida (diferencial / simple): se aplica solo cuando cambia ---
   static uint8_t last_modo = 0xFF;
   if (modo_diferencial != last_modo) {
       last_modo = modo_diferencial;
       pcm_write(64, modo_diferencial ? 0xE0 : 0xE1);
   }

   // --- teclado: escaneo cada 20 ms ---
   static uint32_t last_key = 0;
   if (HAL_GetTick() - last_key >= 20) {
       last_key = HAL_GetTick();
       keypad_process();
   }

   // --- refresco del OLED cada 200 ms ---
   static uint32_t last_oled = 0;
   if (HAL_GetTick() - last_oled >= 200) {
       last_oled = HAL_GetTick();
       oled_show(canal_seleccionado, waveform[canal_seleccionado],
                 (uint32_t)frecuencia[canal_seleccionado], (uint8_t)amplitud[canal_seleccionado]);
   }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_24B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  /* USER CODE END I2S2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1, GPIO_PIN_SET);

  /*Configure GPIO pins : PA0 PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// Llena UNA mitad del buffer: FRAMES_HALF frames (muestras estéreo)
static void fill(uint16_t *dst)
{
    for (int i = 0; i < FRAMES_HALF; i++)
    {
        int32_t s[2];                 // una muestra de 24 bits por canal (L y R)

        for (int c = 0; c < 2; c++)
        {
            int32_t v = wave_next(c);                  // avanza la fase y da la muestra segun la forma de onda
            if (!salida_activa[c]) v = 0;              // si el canal esta apagado -> silencio
            s[c] = v;                                  // si el canal está mudo → 0
        }

        // canal L: parto la muestra de 24 bits en 2 halfwords (MSB primero)
        *dst++ = (uint16_t)(s[0] >> 8);   // los 16 bits de arriba
        *dst++ = (uint16_t)(s[0] << 8);   // los 8 de abajo (+ relleno)
        // canal R
        *dst++ = (uint16_t)(s[1] >> 8);
        *dst++ = (uint16_t)(s[1] << 8);
    }
}

// El DMA terminó la 1ª mitad → la relleno
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    fill(&audio_buf[0]);
}

// El DMA terminó la 2ª mitad → la relleno
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    fill(&audio_buf[BUF_HW / 2]);
}

// Escribe un byte en un registro del codec por I²C
static void pcm_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    HAL_I2C_Master_Transmit(&hi2c1, PCM_ADDR, buf, 2, HAL_MAX_DELAY);
}

void PCM3060_SetAmplitude(uint8_t canal, uint32_t amplitud) // Toma la variable amplitud traida por usb, la convierte a db y escr
{
    uint8_t reg = (canal == 0) ? 0x41 : 0x42;   // 0x41 = DAC L, 0x42 = DAC R

    if (amplitud == 0)
    {
        pcm_write(reg, 0x00);   // mute de ese canal
        return;
    }

    if (amplitud > 100)
        amplitud = 100;

    float A = amplitud / 100.0f;

    float db = 20.0f * log10f(A);

    int pasos = (int)roundf(-db / 0.5f);

    uint8_t valor = 255 - pasos;

    pcm_write(reg, valor);   // volumen de ese canal
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
