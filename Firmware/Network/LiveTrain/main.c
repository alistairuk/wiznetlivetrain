/**
 ******************************************************************************
 * @file    LiveTrain/main.c
 * @author  A
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * Based on WIZnet example code
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "wizchip_conf.h"
#include "dhcp.h"
#include "dns.h"
#include <stdio.h>
#include <string.h>

/** @addtogroup W7500x_StdPeriph_Examples
 * @{
 */

/** @addtogroup WZTOE_WebClient
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DATA_BUF_SIZE 4096

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay;
uint8_t test_buf[DATA_BUF_SIZE];
wiz_NetInfo gWIZNETINFO;
char dns_domain_name[] = "www.agm.me.uk\0";
uint8_t dns_domain_ip[4];
uint8_t web_page_flag = 0;
char http_request_path[] = "/wiznet/livetrain/test.php\0";
char output_prefix[] = "###";

/* Private function prototypes -----------------------------------------------*/
static void UART_Config(void);
static void DUALTIMER_Config(void);
static void Network_Config(void);
void dhcp_assign(void);
void dhcp_update(void);
void dhcp_conflict(void);
int32_t WebClient(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport, uint8_t* requestheader);
void delay(__IO uint32_t milliseconds);
void TimingDelay_Decrement(void);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    uint32_t ret;
    uint8_t dhcp_retry = 0;
    uint8_t dns_retry = 0;

    SystemInit();

    /* Configure the UART for output to the display */
    UART_InitTypeDef UART_InitStructure;
    UART_StructInit(&UART_InitStructure);
    UART_Init(UART0, &UART_InitStructure);
    UART_Cmd(UART0, ENABLE);

    /* SysTick_Config */
    SysTick_Config((GetSystemClock() / 1000));

    /* Set WZ_100US Register */
    setTIC100US((GetSystemClock() / 10000));

    UART_Config();
    DUALTIMER_Config();

    printf("W7500x Standard Peripheral Library version : %d.%d.%d\r\n", __W7500X_STDPERIPH_VERSION_MAIN, __W7500X_STDPERIPH_VERSION_SUB1, __W7500X_STDPERIPH_VERSION_SUB2);

    printf("SourceClock : %d\r\n", (int) GetSourceClock());
    printf("SystemClock : %d\r\n", (int) GetSystemClock());

    /* Initialize PHY */
#ifdef W7500
    printf("PHY Init : %s\r\n", PHY_Init(GPIOB, GPIO_Pin_15, GPIO_Pin_14) == SET ? "Success" : "Fail");
#elif defined (W7500P)
    printf("PHY Init : %s\r\n", PHY_Init(GPIOB, GPIO_Pin_14, GPIO_Pin_15) == SET ? "Success" : "Fail");
#endif

    /* Check Link */
    printf("Link : %s\r\n", PHY_GetLinkStatus() == PHY_LINK_ON ? "On" : "Off");

    /* Network information setting before DHCP operation. Set only MAC. */
    Network_Config();

    /* DHCP Process */
    DHCP_init(0, test_buf);
    reg_dhcp_cbfunc(dhcp_assign, dhcp_update, dhcp_conflict);
    if (gWIZNETINFO.dhcp == NETINFO_DHCP) {       // DHCP
        printf("Start DHCP\r\n");
        while (1) {
            ret = DHCP_run();

            if (ret == DHCP_IP_LEASED) {
                printf("DHCP Success\r\n");
                break;
            }
            else if (ret == DHCP_FAILED) {
                dhcp_retry++;
            }

            if (dhcp_retry > 3) {
                printf("DHCP Fail\r\n");
                break;
            }
        }
    }

    /* Network information setting after DHCP operation.
     * Displays the network information allocated by DHCP. */
    Network_Config();

    /* DNS Process */
    DNS_init(1, test_buf);
    printf("Start DNS\r\n");
    while (1) {
        ret = DNS_run(gWIZNETINFO.dns, (uint8_t *) dns_domain_name, dns_domain_ip);

        if (ret == 1) {
            printf("[%s] of ip : %d.%d.%d.%d\r\n", dns_domain_name, dns_domain_ip[0], dns_domain_ip[1], dns_domain_ip[2], dns_domain_ip[3]);
            break;
        }
        else {
            dns_retry++;
        }

        if (dns_retry > 3) {
            printf("DNS Fail\r\n");
            break;
        }
    }
#

    /* Create the request header to send */
    char request_header[128];
    strcpy(request_header, "GET ");
    strcat(request_header, http_request_path);
    strcat(request_header, " HTTP/1.1\r\n");
    strcat(request_header, "Host: ");
    strcat(request_header, dns_domain_name);
    strcat(request_header, "\r\n");
    strcat(request_header, "Connection: close\r\n");
    strcat(request_header, "\r\n");

    /* The main loop */
    while (1) {
        /* Make theweb request */
        int data_len = WebClient(2, test_buf, dns_domain_ip, 80, (uint8_t*) request_header);
        /* Check we got some valid data back */
        if (data_len>0) {
            /* Search for the start of our data */
            char* data_pointer = strpbrk((char*) test_buf, output_prefix) + strlen(output_prefix);
            if (data_pointer!=0) {
                printf("Data: %s\r\n", data_pointer);
                /* Loop though our data and send it to the serial port */
                uint8_t textpos = 0;
                while ((data_pointer[textpos]!=0) & (data_pointer[textpos]!='\r') & (data_pointer[textpos]!='\n')) {
                    UART_SendData(UART0, data_pointer[textpos++]);
                }
                /* Terminate the output data */
                UART_SendData(UART0, '\r');
                UART_SendData(UART0, '\n');
            }
        }

        /* Wait a few seconds befor requesting again */
        delay(4000);

    }
	
	return 0;
}

/**
 * @brief  Configures the UART Peripheral.
 * @note
 * @param  None
 * @retval None
 */
static void UART_Config(void)
{
    UART_InitTypeDef UART_InitStructure;

    UART_StructInit(&UART_InitStructure);

#if defined (USE_WIZWIKI_W7500_EVAL)
    UART_Init(UART1, &UART_InitStructure);
    UART_Cmd(UART1, ENABLE);
#else
    S_UART_Init(115200);
    S_UART_Cmd(ENABLE);
#endif
}

/**
 * @brief  Configures the DUALTIMER Peripheral.
 * @note
 * @param  None
 * @retval None
 */
static void DUALTIMER_Config(void)
{
    DUALTIMER_InitTypDef DUALTIMER_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    DUALTIMER_InitStructure.Timer_Load = GetSystemClock() / 1; //1s
    DUALTIMER_InitStructure.Timer_Prescaler = DUALTIMER_Prescaler_1;
    DUALTIMER_InitStructure.Timer_Wrapping = DUALTIMER_Periodic;
    DUALTIMER_InitStructure.Timer_Repetition = DUALTIMER_Wrapping;
    DUALTIMER_InitStructure.Timer_Size = DUALTIMER_Size_32;
    DUALTIMER_Init(DUALTIMER0_0, &DUALTIMER_InitStructure);

    DUALTIMER_ITConfig(DUALTIMER0_0, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = DUALTIMER0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DUALTIMER_Cmd(DUALTIMER0_0, ENABLE);
}

/**
 * @brief  Configures the Network Information.
 * @note
 * @param  None
 * @retval None
 */
static void Network_Config(void)
{
    uint8_t mac_addr[6] = { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 };

    memcpy(gWIZNETINFO.mac, mac_addr, 6);
    gWIZNETINFO.dhcp = NETINFO_DHCP;

    ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);

    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
    printf("IP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    printf("GW: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
    printf("SN: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
    printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
}

/**
 * @brief  The call back function of ip assign.
 * @note
 * @param  None
 * @retval None
 */
void dhcp_assign(void)
{
    getIPfromDHCP(gWIZNETINFO.ip);
    getGWfromDHCP(gWIZNETINFO.gw);
    getSNfromDHCP(gWIZNETINFO.sn);
    getDNSfromDHCP(gWIZNETINFO.dns);

    ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
}

/**
 * @brief  The call back function of ip update.
 * @note
 * @param  None
 * @retval None
 */
void dhcp_update(void)
{
    ;
}

/**
 * @brief  The call back function of ip conflict.
 * @note
 * @param  None
 * @retval None
 */
void dhcp_conflict(void)
{
    ;
}

/**
 * @brief  WebClient example function.
 * @note
 * @param  sn: Socket number to use.
 * @param  buf: The buffer the socket will use.
 * @param  destip:IP of destination to connect.
 * @param  destport: Port of destination to connect.
 * @param  requestheader: Request header to send to server.
 * @retval None
 */
int32_t WebClient(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport, uint8_t* requestheader)
{
    int32_t ret;
    uint16_t size = 0;
    uint16_t any_port = 50000;

    close(sn);
    if ((ret = socket(sn, Sn_MR_TCP, any_port++, 0x00)) != sn) return ret;

    printf("%d:Try to connect to the %d.%d.%d.%d : %d\r\n", sn, destip[0], destip[1], destip[2], destip[3], destport);
    if ((ret = connect(sn, destip, destport)) != SOCK_OK) return ret;

    if (getSn_SR(sn) == SOCK_ESTABLISHED) {
        if (getSn_IR(sn) & Sn_IR_CON) {
            printf("%d:Connected to - %d.%d.%d.%d : %d\r\n", sn, destip[0], destip[1], destip[2], destip[3], destport);

            setSn_IR(sn, Sn_IR_CON);

            ret = send(sn, requestheader, strlen((char*)requestheader));
            if (ret < 0) {
                close(sn);
                return ret;
            }
        }
        delay(2000);
        if ((size = getSn_RX_RSR(sn)) > 0) {
            if (size >= DATA_BUF_SIZE) size = DATA_BUF_SIZE-1;
            ret = recv(sn, buf, size);
            if (ret <= 0) return ret;
            printf("%s", buf);
            buf[size] = 0;
            return size;
        }
    }

    return  0;
}


/**
 * @brief  Inserts a delay time.
 * @param  nTime: specifies the delay time length, in milliseconds.
 * @retval None
 */
void delay(__IO uint32_t milliseconds)
{
    TimingDelay = milliseconds;

    while (TimingDelay != 0)
        ;
}

/**
 * @brief  Decrements the TimingDelay variable.
 * @param  None
 * @retval None
 */
void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00) {
        TimingDelay--;
    }
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

/**
 * @}
 */

/**
 * @}
 */

/******************** (C) COPYRIGHT WIZnet *****END OF FILE********************/
