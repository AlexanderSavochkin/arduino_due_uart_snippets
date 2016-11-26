#include "due_sam3x.h"
#include "uart_routines.h"

#define RX_BUFF_SIZE 8192
#define TX_BUFF_SIZE 8192

volatile char rx_circular_buffer[RX_BUFF_SIZE];
volatile char tx_circular_buffer[TX_BUFF_SIZE];

volatile unsigned int tx_circular_buffer_head;

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

  // Acknowledge errors
  if ((status & UART_SR_OVRE) == UART_SR_OVRE || (status & UART_SR_FRAME) == UART_SR_FRAME)
  {
    // TODO: error reporting outside ISR
    UART->UART_CR |= UART_CR_RSTSTA;
  }
}

int uart_write_string_discard_overflow(int n, char *str)
{
  int i;

  volatile uint32_t junk;
  UART->UART_PTCR = UART_PTCR_TXTDIS;
  junk = UART->UART_PTSR;			// before computing remaining, wait a couple of cycles 

  uint32_t current_tx_address = UART->UART_TPR;
  uint32_t *current_buffer_counter = &(UART->UART_TCR);

  if (UART->UART_TNCR != 0 || UART->UART_TCR + UART->UART_TPR >= tx_circular_buffer + TX_BUFF_SIZE - 1)
  {
    current_buffer_counter = &(UART->UART_TNCR);
  }

  for (i = 0; i < n && str[i] != '\0'; ++i)
  {
    if (tx_circular_buffer + tx_circular_buffer_head == current_tx_address)
    {
      break;
    }

    tx_circular_buffer[tx_circular_buffer_head++] = str[i];
    ++(*current_buffer_counter);    

    if (tx_circular_buffer_head >= TX_BUFF_SIZE)
    {
      tx_circular_buffer_head = 0;
      uint32_t *current_buffer_counter = &(UART->UART_TNCR);
      UART->UART_TNPR = tx_circular_buffer;
    }
  }


  UART->UART_PTCR = UART_PTCR_TXTEN;
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

  // Disable PDC for reading
  UART->UART_PTCR = UART_PTCR_RXTDIS;

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
