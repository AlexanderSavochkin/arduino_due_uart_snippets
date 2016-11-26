#ifndef _UART_ROUTINES_
#define _UART_ROUTINES_

typedef enum {BlockOnPendingOperation, GiveUpOnPendingOperation} OverflowMode;

int uart_write_char( const uint8_t uc_data, OverflowMode overflowMode);

int uart_write_string(int n, char *str, OverflowMode overflowMode);

int uart_read_char(OverflowMode overflowMode);

void setupUART();


#endif