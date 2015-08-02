#include <msp430.h>
#include "../msprf24/msprf24.h"
#include <stdint.h>
#include <string.h>

#define false   0
#define true    1

// Mascara PORT_1
#define RX_PIN  BIT1
#define TX_PIN  BIT2
#define RELE_PIN    BIT0
#define DIMMER_PIN  BIT4

// Mascaras Buffer
// rfbuf[x]
#define ADDR0   0
#define ADDR1   1
#define ADDR2   2
#define RELE    3
#define DIMMER  4

void Serial_config(void);
void Serial_escreve_dado(char);
void Serial_escreve_texto(char *);

// Enderecos dos dispositivos
const uint8_t thisNode[] = { 'M', 'S', 'P' };
// const uint8_t masterAddr[] = { 'R', 'P', 'I' };

int main(){
    uint8_t pktlen, pipeid;
    uint8_t rfbuf[32];

    // Desabilita Watchdog Timer
    WDTCTL = WDTPW + WDTHOLD;

    // Configura clock @16MHz
    DCOCTL = CALDCO_16MHZ;
    BCSCTL1 = CALBC1_16MHZ;
    // SMCLK: /2 -> spi max 10MHz
    BCSCTL2 = DIVS_1;

    // GPIO
    P1DIR |=  (RELE_PIN + DIMMER_PIN);
    P1OUT &= ~(RELE_PIN + DIMMER_PIN);

    // Inicializacao do nRF24L01+ RF transceiver
    rf_crc = RF24_EN_CRC;               // verificacao de redundancia ciclica
    rf_addr_width = 5;                  // tamanho do endereco
    rf_speed_power = RF24_SPEED_MIN | RF24_POWER_MAX;
    rf_channel = 8;
    msprf24_init();                     // dasabilitado por padrao
    msprf24_open_pipe(1, 1);            // Receive pipe#1 (could use #0 too, as we don't do any TX...)
    msprf24_set_pipe_packetsize(1, 0);  // 0: payload dinamico
    w_rx_addr(1, (uint8_t*)thisNode);    // atribui o endereco
    msprf24_activate_rx();              // habilita recepcao

    // Configura porta serial @9600 bps
    Serial_config();
    Serial_escreve_texto("\nconnn");

    // Main loop
    while (1) {
        // Handle incoming nRF24 IRQs
    //     Serial_escreve_texto("\nloop");
    //     if (rf_irq & RF24_IRQ_FLAGGED) {
    //         msprf24_get_irq_reason();
    //         Serial_escreve_texto("\nmsprf24_get_irq_reason()");

    //         if (rf_irq & RF24_IRQ_RX || msprf24_rx_pending()) {
    //             pktlen = r_rx_peek_payload_size();
    //             Serial_escreve_texto("\npktlen = r_rx_peek_payload_size();");
    //             if (!pktlen || pktlen > 32) { // necessario mesmo com CRC habilitado
    //                 Serial_escreve_texto("\nif (!pktlen || pktlen > 32)");
    //                 flush_rx();
    //                 msprf24_irq_clear(RF24_IRQ_RX);
    //             } else {
    //                 Serial_escreve_texto("\nelse");
    //                 pipeid = r_rx_payload(pktlen, rfbuf);
    //                 msprf24_irq_clear(RF24_IRQ_RX);
    //                 if (pipeid == 1) {  // Only paying attention to pipe#1 (this should always eval true)

    //                     Serial_escreve_texto("\n\n@");
    //                     Serial_escreve_dado((char) rfbuf[ADDR0]);
    //                     Serial_escreve_dado((char) rfbuf[ADDR1]);
    //                     Serial_escreve_dado((char) rfbuf[ADDR2]);
                        
    //                     if (rfbuf[RELE] == '1'){
    //                         P1OUT |= RELE_PIN; // ativa
    //                         Serial_escreve_texto("\nRele: on");
    //                     }else{
    //                         P1OUT &= ~RELE_PIN; // desativa
    //                         Serial_escreve_texto("\nRele: off");
    //                     }

    //                     Serial_escreve_texto("\nDimmer: ");
    //                     Serial_escreve_dado((char) rfbuf[DIMMER]);

    //                 }
    //             }
    //         }
    //     }
    //     // __bis_SR_register(LPM4_bits);
    //     Serial_escreve_texto("\nlpm41");
    //     LPM4;
    //     Serial_escreve_texto("\nlpm42");
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
