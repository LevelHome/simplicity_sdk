#include "sl_hal_gpio.h"
#include "sl_hci_uart.h"
#include "sl_hci_common_transport.h"
#include "sl_btctrl_hci_packet.h"
#include "sl_btctrl_hci_transport.h"
#include "sl_iostream_init_eusart_instances.h"
#include "sl_iostream_eusart_vcom_config.h"

/**
 * @brief Reads data from the IOStream UART interface.
 *
 * This function reads data from the UART interface using the specified length.
 *
 * @param data Pointer to the buffer where the read data will be stored.
 * @param len The length of the data to be read.
 * @return The number of bytes read from the UART interface.
 */
int sl_hci_uart_read(uint8_t *data, uint16_t len)
{
  size_t bytes_read;
  sl_status_t status;
  status = sl_iostream_read(sl_iostream_vcom_handle, data, len, &bytes_read);

  EFM_ASSERT(status != SL_STATUS_INVALID_CONFIGURATION);

  if (status == SL_STATUS_EMPTY) {
    bytes_read = 0;
  }
  return bytes_read;
}

/**
 * @brief Writes data to the UART interface.
 *
 * This function writes data to the UART interface using the specified length.
 *
 * @param data Pointer to the buffer containing the data to be written.
 * @param len The length of the data to be written.
 * @param callback Pointer to the callback function to be called after the write operation is completed.
 * @return The status of the write operation.
 */
Ecode_t sl_hci_uart_write(uint8_t *data, uint16_t len, void (*callback)(uint32_t))
{
  sl_status_t status;
  status = sl_iostream_write(sl_iostream_vcom_handle, data, len);
  if (status == SL_STATUS_OK) {
    callback(status);
  }
  return status;
}

/**
 * @brief Gets the port and pin for the UART interface.
 *
 * This function retrieves the port and pin information for the UART interface.
 *
 * @param port Pointer to the variable where the port information will be stored.
 * @param pin Pointer to the variable where the pin information will be stored.
 */
void sl_btctrl_hci_transport_get_port_pin(uint8_t *port, uint8_t *pin)
{
  *port = SL_IOSTREAM_EUSART_VCOM_RX_PORT;
  *pin = SL_IOSTREAM_EUSART_VCOM_RX_PIN;
}

/**
 * @brief Disables sleep mode.
 *
 * This function disables sleep mode. This function will be overridden in sl_hci_uart.c if usartdrv_uart is used.
 *
 * @param disable Flag indicating whether to disable sleep mode or not.
 */
SL_WEAK void sl_hci_disable_sleep(bool disable)
{
  (void) disable;
  return;
}
