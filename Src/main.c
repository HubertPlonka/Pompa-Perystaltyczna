#include "main.h"
#include <stdio.h>



ADC_HandleTypeDef hadc;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;


void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC_Init(void);
static void MX_USART2_UART_Init(void);



uint8_t Jasnosc = 7;  //Jasnosc wyswietlacza
uint32_t Poten;  //Odczyt potencjometru do ustawienia ilosci wody do przepompowania
uint32_t PotenLCD;  //Odczyt potencjometru, odpowiednio zeskalowany, by wyswietlac liczbe mililitrow
int Wykonanie; //Zmienna kontrolujaca pozostala liczbe krokow do wykonania
int Status = 1;  //Status: 1 - ustawianie, silnik stoi; 2 - praca silnika
int Cykl = 1;  //Sekwencja ktora wykonuje silnik(kolejne kroki)
char buffer[12];  //Bufor do transmisji UART
uint8_t Bpoten[14] = "Potencjometr: ";  //Wartosc odczytana z potencjometru
uint8_t Bstatus[13] = "     Status: ";  //Wyswietlanie zmiennej status przez UART
uint8_t Bcykl[11] = "     Cykl: ";  //Jak powyzej
int period = 0;  //Podczas odczytu wartosc potencjometru bardzo skacze, aby zwiekszyc czytelnosc nie odczytujemy caly czas, tylko w okresach


void ScreenControl()  //Funkcja sluzaca wyswietlaniu zmiennych na ekranie komputera
{
	HAL_UART_Transmit(&huart2, Bpoten, 14, 10);
	HAL_UART_Transmit(&huart2, (uint8_t*)buffer, sprintf(buffer, "%d", Poten), 2);
	HAL_UART_Transmit(&huart2, Bstatus, 13, 10);
	HAL_UART_Transmit(&huart2, (uint8_t*)buffer, sprintf(buffer, "%d", Status), 2);
	HAL_UART_Transmit(&huart2, Bcykl, 11, 10);
	HAL_UART_Transmit(&huart2, (uint8_t*)buffer, sprintf(buffer, "%d", Cykl), 2);
	char newline[2] = "\r\n";
	HAL_UART_Transmit(&huart2, (uint8_t *) newline, 2, 2);
}


void PrzyciskEnter()  //Funkcja zmieniajaca Status na 2
{
	if(HAL_GPIO_ReadPin(GPIOB, BUT_ENTER_Pin) == 0) 
	{
			Status = 2;
		  Wykonanie = 0;
	}
}

void PrzyciskEscape()  //Funkcja zmieniajaca Status na 1, mozna przerwac prace silnika w trakcie pompowania i powrocic do ustawiania
{
	if(HAL_GPIO_ReadPin(GPIOB, BUT_ESC_Pin) == 0) 
	{
			Status = 1;
	}
}

void MOTOR() {  //Funkcja przelaczajaca pomiedzy kolejnymi krokami silnika i odliczajaca ile z wartosci odczytanej z potencjometru pozostalo by przepompowac zadana ilosc plynu
  if (Cykl == 5) {
    Cykl = 1;
	}
		Wykonanie++;
		if (Wykonanie == 3)    
		{
			Poten--;
			Wykonanie = 0;
  }
		if (Poten == 0)
		{
			Status = 1;
		}
	
  switch (Cykl) {     //Przelaczanie pomiedzy cyklami
    case 1:  
			HAL_GPIO_WritePin(GPIOA, ENA_ENB_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN1_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN2_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN3_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN4_Pin, GPIO_PIN_RESET);
      
      Cykl = Cykl + 1;
      break;
    case 2:     
			HAL_GPIO_WritePin(GPIOA, ENA_ENB_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN1_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN2_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN3_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN4_Pin, GPIO_PIN_SET);
 
      Cykl = Cykl + 1;
      break;
    case 3:   
			HAL_GPIO_WritePin(GPIOA, ENA_ENB_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN1_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN2_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN3_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN4_Pin, GPIO_PIN_SET);
      
      Cykl = Cykl + 1;
      break;
    case 4:   
			HAL_GPIO_WritePin(GPIOA, ENA_ENB_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN1_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN2_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN3_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, IN4_Pin, GPIO_PIN_RESET);
     
      Cykl = Cykl + 1;
      break;
  };
}     

void Sterowanie()  //Funkcja przelaczajaca pomiedzy ustawianiem i praca
{
	
	switch (Status) {
    case 1:                         // STOP
      HAL_GPIO_WritePin(GPIOB, LED_STOP_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOB, LED_RUN_Pin, GPIO_PIN_RESET);
      
      
      HAL_GPIO_WritePin(GPIOA, ENA_ENB_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN1_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN2_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN3_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, IN4_Pin, GPIO_PIN_RESET);

      HAL_ADC_Start(&hadc);
		  if( HAL_ADC_PollForConversion(&hadc, 1) == HAL_OK)
		  {
				
			Poten = HAL_ADC_GetValue(&hadc);  //Odczyt potencjometru
				Poten = 2* Poten;
	    }
      
      if (Poten <= 10) {
        Poten = 10;
      }
      if (Poten >= 8000) {
        Poten = 8000;
      }
      break;

    case 2:                     // START pompy
      HAL_GPIO_WritePin(GPIOB, LED_STOP_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOB, LED_RUN_Pin, GPIO_PIN_SET);

      MOTOR();
     
     HAL_Delay(2.2);  //Doswiadczalnie wyznaczone minimalne opoznienie pomiedzy kolejnymi krokami silnika by ten pracowal z maksymalna predkoscia
      break;
  }   
}  

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_USART2_UART_Init();
	
	
	TM1637_clearDisplay();  //Wyczyszczenie wyswietlacza i ustawienie jasnosci
	TM1637_brightness(Jasnosc);

	
  while (1)
  {
PrzyciskEnter();  //Odczyt przyciskow i przelaczenie pomiedzy praca i ustawianiem
PrzyciskEscape();
Sterowanie();


if (period == 0){  //Wyswietlanie odczytanej wartosci potencjometru co jakis czas, by zwiekszyc czytelnosc
	TM1637_display_all(PotenLCD);
	period++;
}
else{
	period++;
	period = period % 100;
}
PotenLCD = Poten / 85;  //Wartosc z potencjometru podzielona przez 85 odpowiada przepompowaniu 1 mililitra
	//ScreenControl();  //Komunikacja z PC przez UART zakomentowana, poniewaz spowalnia prace pompy
											//Wykorzystywana jedynie w celach diagnostycznych
  }
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI14|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}
static void MX_ADC_Init(void)
{

  ADC_ChannelConfTypeDef sConfig = {0};

  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }
  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}


static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}


static void MX_DMA_Init(void) 
{

  __HAL_RCC_DMA1_CLK_ENABLE();

  HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);

}


static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|ENA_ENB_Pin|LCD_DIO_Pin|LCD_CLK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, IN1_Pin|LED_RUN_Pin|LED_STOP_Pin|IN4_Pin 
                          |IN2_Pin|IN3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin ENA_ENB_Pin LCD_DIO_Pin LCD_CLK_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|ENA_ENB_Pin|LCD_DIO_Pin|LCD_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : Potencjometr_Pin */
  GPIO_InitStruct.Pin = Potencjometr_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Potencjometr_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BUT_ENTER_Pin BUT_ESC_Pin */
  GPIO_InitStruct.Pin = BUT_ENTER_Pin|BUT_ESC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : IN1_Pin LED_RUN_Pin LED_STOP_Pin IN4_Pin 
                           IN2_Pin IN3_Pin */
  GPIO_InitStruct.Pin = IN1_Pin|LED_RUN_Pin|LED_STOP_Pin|IN4_Pin 
                          |IN2_Pin|IN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

void Error_Handler(void)
{

}

#ifdef  USE_FULL_ASSERT

void assert_failed(char *file, uint32_t line)
{ 

}
#endif /* USE_FULL_ASSERT */

