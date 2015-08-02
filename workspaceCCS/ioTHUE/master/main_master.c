#include <msp430.h>
#include "../msprf24/msprf24.h"
#include <stdint.h>

#define false   0
#define true    1

// Mascara PORT_1
#define RX_PIN  BIT1
#define TX_PIN  BIT2

// Mascaras Buffer
// rfpayload[x]
#define ADDR0   0
#define ADDR1   1
#define ADDR2   2
#define RELE    3
#define DIMMER  4
#define PL_SIZE 5 // payload size
#define START_CHAR  '|'
#define END_CHAR    '!'


void Serial_config(void);
void Serial_escreve_dado(char);
void Serial_escreve_texto(char *);
void Serial_receive(void);

const uint8_t masterAddr[]  = { 'R', 'P', 'I' };
const uint8_t nodeAddr[]    = { 'M', 'S', 'P' };

uint8_t rfpayload[PL_SIZE] = { };
uint8_t pl_index = 0; // payload index

char serial_data_rcv;
uint8_t handle_serial = false;
uint8_t send_pkg = false;

int main(){
    uint8_t txretry_count=0, txretry_latch=0;

    WDTCTL = (WDTPW + WDTHOLD);
    DCOCTL = CALDCO_16MHZ;
    BCSCTL1 = CALBC1_16MHZ;
    BCSCTL2 = DIVS_1;  // SMCLK = 8MHz

    // Inicializacao do transceiver
    rf_crc = RF24_EN_CRC;
    rf_addr_width = 5;
    rf_speed_power = RF24_SPEED_MIN | RF24_POWER_MAX;
    rf_channel = 8;
    msprf24_init();
    msprf24_open_pipe(0, 1);  // This is only for receiving auto-ACK packets.
    msprf24_set_pipe_packetsize(0, 0);  // Dynamic payload support
        // Note: Pipe#0 is hardcoded in the transceiver hardware as the designated "pipe" for a TX node to receive
        // auto-ACKs.  This does not have to match the pipe# used on the RX side.

    Serial_config();

    // Main loop
    while (1) {
        // Handle any nRF24 IRQs first
        if (rf_irq & RF24_IRQ_FLAGGED) {
            msprf24_get_irq_reason();

            if (rf_irq & RF24_IRQ_TX) {
                msprf24_irq_clear(RF24_IRQ_TX);
                flush_tx();
                w_rx_addr(0, (uint8_t*)masterAddr);
                msprf24_powerdown();
            }

            if (rf_irq & RF24_IRQ_TXFAILED) {
                msprf24_irq_clear(RF24_IRQ_TXFAILED);
                if (txretry_count) {
                    txretry_count--;
                    if (!txretry_latch) {
                        txretry_latch = 1;
                        tx_reuse_lastpayload();
                    }
                    pulse_ce();
                } else {
                    // Ran out of retries, give up
                    flush_tx();
                    w_rx_addr(0, (uint8_t*)masterAddr);
                    txretry_latch = 0;
                    msprf24_powerdown();
                }
            }
            // No need to handle RX packets since we never enter RX mode
        }

        if (handle_serial){
            if(serial_data_rcv == START_CHAR){
                pl_index == 0;
            }
            else if(serial_data_rcv == END_CHAR){
                send_pkg = true;
                pl_index == 0;
            }
            else if(pl_index < PL_SIZE){
                rfpayload[pl_index++] = serial_data_rcv;
            }
            else{
                pl_index = 0;
            }

            handle_serial = false;
        }

        if (msprf24_queue_state() & RF24_QUEUE_TXEMPTY && send_pkg) {

            Serial_escreve_texto("\nEnviando pacote: ");
            Serial_escreve_texto(rfpayload);

            msprf24_standby();
            w_tx_addr((uint8_t*)nodeAddr);
            w_rx_addr(0, (uint8_t*)nodeAddr);  // To catch the auto-ACK reply
            w_tx_payload(2, (uint8_t*)rfpayload);
            msprf24_activate_tx();
            txretry_count = 20;  // Try damned hard to send this, retrying up to 20 * ARC times (ARC=15 by default)

            send_pkg = false;
        }

    }
}

void Serial_config(void){
    // Configura pinos serial
    P1SEL  |= (RX_PIN + TX_PIN);
    P1SEL2 |= (RX_PIN + TX_PIN);

    // USCI reset: desabilitado para operacao
    // UCSWRST (BIT1) = 1
    UCA0CTL1 |= UCSWRST; // |= 0x01

    // Modo UART:
        // UCMODE1 (BIT2) = 0
        // UCMODE0 (BIT1) = 0
    // Modo assincrono:
        // UCSYNC (BIT0) = 0
    UCA0CTL0 &= ~(UCMODE1 + UCMODE0 + UCSYNC); // &= ~0x07

    // USCI clock: modo 2 (SMCLK):
    // UCSSEL (BIT7 e BIT6):
            // (BIT7) = 1
            // (BIT6) = 0
    UCA0CTL1 |= UCSSEL_2; // |= 0x80

    // Oversampling desabilitado
        //  UCOS16 (BIT1) = 0
    UCA0MCTL &= ~UCOS16;

    // 9600 bps: 8M / 9600 = 833,333
    // int(8M / 9600) = 833 = 0x[03][41]
    // round((833,333 - 833)*8) = [3]
    UCA0BR1 = 0x03;
    UCA0BR0 = 0x41;
    UCA0MCTL |= 0x06; // |= (0x[3]) << 1;

    // USCI reset: liberado para operacao
    UCA0CTL1 &= ~UCSWRST; // &= ~0x01

    // Habilita int. de recepcao
    IE2 |= UCA0RXIE;
}

void Serial_escreve_dado(char dado){
    while(!(IFG2 & UCA0TXIFG)); // aguarda buffer vazio
    UCA0TXBUF = dado;
}

void Serial_escreve_texto(char *caracter){
    while(*caracter){
        Serial_escreve_dado(*caracter);
        caracter++;
    }
}


#pragma vector = USCIAB0RX_VECTOR
__interrupt void Serial_receive(void){

    serial_data_rcv = UCA0RXBUF;
    handle_serial = true;

    IFG2 &= ~UCA0RXIFG;
}