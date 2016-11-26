#include "due_sam3x.h"
#include "uart_routines.h"

#define RX_BUFF_SIZE 8192
#define TX_BUFF_SIZE 8192

volatile char rx_circular_buffer[RX_BUFF_SIZE];
volatile char tx_circular_buffer[TX_BUFF_SIZE];

volatile unsigned int tx_circular_buffer_head;
volatile unsigned int tx_circular_buffer_tail;
volatile unsigned int rx_circular_buffer_head;
volatile unsigned int rx_circular_buffer_tail;

// IT handlers
void UART_Handler(void)
{

  uint32_t status = UART->UART_SR;

  // Did we receive data?
  if ((status & UART_SR_RXRDY) == UART_SR_RXRDY)
  {
    unsigned int i = (unsigned int)(rx_circular_buffer_head + 1) % RX_BUFF_SIZE;
    uint8_t c = UART->UART_RHR;

    if ( i != rx_circular_buffer_tail )
    {
      rx_circular_buffer[rx_circular_buffer_head] = c;
      rx_circular_buffer_head = i;
    }
  }

  // Do we need to keep sending data?
  if ((status & UART_SR_TXRDY) == UART_SR_TXRDY) 
  {

    if (tx_circular_buffer_tail != tx_circular_buffer_head) {
      UART->UART_THR = tx_circular_buffer[tx_circular_buffer_tail];
      tx_circular_buffer_tail = (unsigned int)(tx_circular_buffer_tail + 1) % TX_BUFF_SIZE;
    }
    else
    {
      // Mask off transmit interrupt so we don't get it anymore
      UART->UART_IDR = UART_IDR_TXRDY;
    }
  }

  // Acknowledge errors
  if ((status & UART_SR_OVRE) == UART_SR_OVRE || (status & UART_SR_FRAME) == UART_SR_FRAME)
  {
    // TODO: error reporting outside ISR
    UART->UART_CR |= UART_CR_RSTSTA;
  }
}

int uart_write_char( const uint8_t uc_data, OverflowMode overflowMode)
{
  // Is the hardware currently busy?
  if (((UART->UART_SR & UART_SR_TXRDY) != UART_SR_TXRDY) |
      (tx_circular_buffer_tail != tx_circular_buffer_head))
  {
    // If busy we buffer
    int nextWrite = (tx_circular_buffer_head + 1) % TX_BUFF_SIZE;

    if (tx_circular_buffer_tail == nextWrite && overflowMode == GiveUpOnPendingOperation)
    {
      return 0;
    }

    while (tx_circular_buffer_tail == nextWrite)
      ; // Spin locks if we're about to overwrite the buffer. This continues once the data is sent

    tx_circular_buffer[tx_circular_buffer_head] = uc_data;
    tx_circular_buffer_head = nextWrite;
    // Make sure TX interrupt is enabled
    UART->UART_IER = UART_IER_TXRDY;
    return 1;
  }
  else 
  {
     // Bypass buffering and send character directly
     UART->UART_THR = uc_data;
     return 1;
  }
}


int uart_write_string(int n, char *str, OverflowMode overflowMode)
{
  int i;
  for (i = 0; i < n && str[i] != 0; ++i)
  {
    int write_result;
    write_result = uart_write_char(str[i],overflowMode);
    if (!write_result)
    {
      break;
    }
  }
  return i;
}

int uart_read_char(OverflowMode overflowMode)
{
  // if the head isn't ahead of the tail, we don't have any characters
  if ( rx_circular_buffer_head == rx_circular_buffer_tail )
  {
    if (overflowMode == GiveUpOnPendingOperation)
    {
      return -1;
    }
    else
    {
      while(rx_circular_buffer_head == rx_circular_buffer_tail);
    }
  }

  uint8_t uc = rx_circular_buffer[rx_circular_buffer_tail];
  rx_circular_buffer_tail = (unsigned int)(rx_circular_buffer_tail + 1) % RX_BUFF_SIZE;
  return uc;
}


void setupUART()
{

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
  UART->UART_IER = UART_IER_RXRDY | UART_IER_OVRE | UART_IER_FRAME;

  NVIC_EnableIRQ(UART_IRQn);

  // Enable receiver and transmitter
  UART->UART_CR = UART_CR_RXEN | UART_CR_TXEN;
}
