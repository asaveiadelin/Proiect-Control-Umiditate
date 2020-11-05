/*
				XMC 2GO
--------------------------------------------------
Rx 						--- P2.6 (UART)
Tx 						--- P2.0 (UART)
Senzor umiditate sol	--- P2.7 (ADC_MEASUREMENT)
Senzor temperatura aer	---
Senzor picaturi			---
Pompa					---

ADC     --- 12 biti (2^12=4096)
VCC ADC --- 3.3 V
formula conversie in volti: rezultat = 3.3 * valoare_analog / 4096
*/

#include <DAVE.h>

int  analog;                 // rezultat conversie AD

void adch()                   // rutina tratare intrerupere AF
{
	analog = ADC_MEASUREMENT_GetResult(&adc); //lectura data
}

float calcul_procent(float valoare)
{
	float procent;
	procent=100-(100*(valoare-1.7)/(3.29-1.7));
	if(procent<=0)
	{
		procent=0;
	}
	else if(procent>=100)
	{
		procent=100;
	}
	return procent;
}

void delay(int j)
{
	for (int i=0;i<0xffff*j;i++); // temporizare
}

int main(void)
{
	uint8_t text_start[] = "\nSmart Garden\r\n\n\n";
	uint8_t umiditate_sol[] = " % umiditate sol";
	uint8_t send_selectare_mod[] = "\n\n\n\nUmiditatea este scazuta!\nSelecteaza mod AUTOMAT sau MANUAL.\n";
	uint8_t send_timp[] = "\n\nA fost selectat modul AUTOMAT!\nVerificati dupa 10 secunde.\n";
	uint8_t send_manual[] = "\n\nA fost selectat modul MANUAL!\nFolositi butoanele START, STOP, CLOSE";
	uint8_t send_pornit_pompa[] = "\nAm PORNIT pompa.\n";
	uint8_t send_oprit_pompa[] = "\nAm OPRIT pompa.\n";
	uint8_t send_close_pompa[] = "\nIesire mod MANUAL\n";
	uint8_t read_data   = 0x0;

	char ad_float[4];           // bufer date convertor A/D
	float x;	// valoarea tensiunii de la intrare in volti

	DAVE_Init();		          // initializare DAVE
	UART_Transmit(&UART_0, text_start, sizeof(text_start)); //mesaj "Smart Garden"
	ADC_MEASUREMENT_Init(&adc); // este inclusa si in DAVE_Init()
	while(1U)
	{
	  ADC_MEASUREMENT_StartConversion(&adc);  // genereaza intr. ls sfarsit
	  delay(1);
	  x=(3.3)*analog/(float)4096;
	  sprintf(ad_float, "%1.3f", calcul_procent(x));     // foloseste lungime buffer

	  UART_Transmit(&UART_0, ad_float, sizeof(ad_float));// afisare procent
	  UART_Transmit(&UART_0, umiditate_sol, sizeof(umiditate_sol)); //mesaj "umiditate sol"
	  if(!UART_IsTxBusy(&UART_0))        // liber transmisie
      UART_TransmitWord(&UART_0,'\r');   // inceput linie
      if(!UART_IsTxBusy(&UART_0))
      UART_TransmitWord(&UART_0,'\n');   // linie noua
      int w=0;

      if(calcul_procent(x)<60)
      {
    	  UART_Transmit(&UART_0, send_selectare_mod, sizeof(send_selectare_mod));
    	  UART_Receive(&UART_0, &read_data,1);
    	  if(read_data == 'a')	//mod automat
    	  {
             DIGITAL_IO_SetOutputLow (&led); 	//porneste pompa
             UART_Transmit(&UART_0, send_timp, sizeof(send_timp));
             delay(200);	// perioada in care sa se ude planta
    	  }
    	  else if(read_data == 'm')		//mod manual
    	  {
        	 UART_Transmit(&UART_0, send_manual, sizeof(send_manual));
        	 do{
        		 UART_Receive(&UART_0, &read_data,1);
        	 	 if(read_data == 'z')	//buton START manual
        	 	 {
        	 		 DIGITAL_IO_SetOutputLow (&led); 	//porneste pompa
        	 		 UART_Transmit(&UART_0, send_pornit_pompa, sizeof(send_pornit_pompa));
        	 	 }
        	 	 else if(read_data == 'x')	//buton STOP manual
        	 	 {
        	 		 DIGITAL_IO_SetOutputHigh (&led); 	//opreste pompa
        	 		 UART_Transmit(&UART_0, send_oprit_pompa, sizeof(send_oprit_pompa));
        	 	 }
        	 	 else if(read_data == 'c')	//iesire mod manual
        	 	 {
        	 		 w=1;
        	 	     UART_Transmit(&UART_0, send_close_pompa, sizeof(send_close_pompa));
        	 	 }
        	 }while(w==0);
    	  }
      }
      else
      {
         DIGITAL_IO_SetOutputHigh (&led);  //opreste pompa
      }
      delay(4);
  }
  return 0;
}
