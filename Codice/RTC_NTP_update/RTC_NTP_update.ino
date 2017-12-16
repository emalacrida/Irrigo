// Variabili per il Timer aggiornamento RTC
const unsigned long INTERVAL = 86400000; // Letture ogni 24 ore (mS)
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;







void setup() {
  // put your setup code here, to run once:

}

void loop {
   // CONTROLLO DEL TEMPO TRASCORSO - CODICE NON BLOCCANTE
   if ((unsigned long)(currentMillis - previousMillis) >= INTERVAL) {

      // INSERIRE QUI IL CODICE DI AGGIORNAMENTO RTC

      // Aggiorna il contatempo del Timer Letture
      previousMillis = currentMillis;
      } // FINE DEL CODICE NON BLOCCANTE

      // RESTANTE CODICE DA ESEGUIRE
}
