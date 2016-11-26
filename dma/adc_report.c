#include "due_sam3x.init.h"
#include "uart_routines.h"


void setupADC()
{
  /*Configure ADC */
  pmc_enable_periph_clk(ID_ADC); // To use peripheral, we must enable clock distributon to it
  adc_init(ADC, SystemCoreClock, ADC_FREQ_MIN, ADC_STARTUP_NORM); // initialize, set maximum posibble speed
  adc_disable_interrupt(ADC, 0xFFFFFFFF); 
  adc_set_resolution(ADC, ADC_12_BITS);
  adc_configure_power_save(ADC, 0, 0); // Disable sleep
  adc_configure_timing(ADC, 0, ADC_SETTLING_TIME_3, 1); // Set timings - standard values
  adc_set_bias_current(ADC, 1); // Bias current - maximum performance over current consumption
  adc_stop_sequencer(ADC); // not using it
  adc_disable_tag(ADC); // it has to do with sequencer, not using it
  adc_disable_ts(ADC); // deisable temperature sensor
  adc_disable_channel_differential_input(ADC, ADC_CHANNEL_7);
  adc_configure_trigger(ADC, ADC_TRIG_SW, 1); // triggering from software, freerunning mode
  adc_disable_all_channel(ADC);
  adc_enable_channel(ADC, ADC_CHANNEL_0);
  //adc_enable_channel(ADC, ADC_CHANNEL_1);
  adc_start(ADC);
}

void loop() 
{
  while(1)
  {
    char buff[50];

    uint32_t val;
    while ((adc_get_status(ADC) & ADC_ISR_DRDY) != ADC_ISR_DRDY)
      {}; //Wait for end of conversion
    val = adc_get_latest_value(ADC); // Read ADC

    int nprinted = snprintf(buff, 50, "%d\r\n", val);
    uart_write_string_discard_overflow(nprinted, buff);

    Sleep(100);
  }
}

int main(void)
{
  init_controller();   // The general init (clock, libc, watchdog disable)

  setupUART();
  setupADC();

  /* Board pin 13 == PB27 */
  PIO_Configure(PIOB, PIO_OUTPUT_1, PIO_PB27, PIO_DEFAULT);
  PIOB->PIO_SODR = PIO_PB27;
  
  uart_write_string(7, "Hello!\r\n");

  loop();
  return 0;
}
