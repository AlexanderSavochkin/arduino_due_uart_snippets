#include "due_sam3x.init.h"

void setup()
{
  // The general init (clock, libc, watchdog disable)
  init_controller();


  PIO_Configure(PIOA, PIO_PERIPH_A,PIO_PA8A_URXD|PIO_PA9A_UTXD, PIO_DEFAULT);

  // Enable the pull up on the Rx and Tx pin
  PIOA->PIO_PUER = PIO_PA8A_URXD | PIO_PA9A_UTXD;

 
  /* Board pin 13 == PB27 */
  PIO_Configure(PIOB, PIO_OUTPUT_1, PIO_PB27, PIO_DEFAULT);

  pmc_enable_periph_clk(ID_UART);

  // Disable PDC channel
  UART->UART_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;

  // Reset and disable receiver and transmitter
  UART->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS;

  // Configure mode
  uint32_t dwMode = US_MR_CHRL_8_BIT | US_MR_NBSTOP_1_BIT | UART_MR_PAR_NO;
  uint32_t modeReg = dwMode & 0x00000E00;

  UART->UART_MR = modeReg;

  // Configure baudrate (asynchronous, no oversampling)
  uint32_t dwBaudRate = 9600;
  UART->UART_BRGR = (SystemCoreClock / dwBaudRate) >> 4;

  // Configure interrupts
  UART->UART_IDR = 0xFFFFFFFF;

  // Enable receiver and transmitter
  UART->UART_CR = UART_CR_RXEN | UART_CR_TXEN;

}

void loop() 
{
  unsigned char delta = 0;
  while(1)
  {
    unsigned char uc_data;
    for (uc_data = 'a'; uc_data < 'z'; ++uc_data)
    {
      // Is the hardware currently busy?
      //while ((UART->UART_SR & UART_SR_TXRDY) != UART_SR_TXRDY && (UART->UART_SR & UART_SR_RXRDY) !=  UART_SR_RXRDY );
      // Bypass buffering and send character directly
      //UART->UART_THR = uc_data;
      while (1)
      {
        if ((UART->UART_SR & UART_SR_RXRDY) ==  UART_SR_RXRDY )
        {
          uint32_t received = UART->UART_RHR;
          if (received == 'u')
            delta = 'A' - 'a';
          else if (received == 'l')
            delta = 0;
        }
        if ((UART->UART_SR & UART_SR_TXRDY) == UART_SR_TXRDY)
        {
          UART->UART_THR = uc_data + delta;
          break;
        }
      }
    }
  }
}

int main(void)
{
  setup();
  loop();
  return 0;
}
