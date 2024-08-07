// Definisci USE_DEBUG per abilitare la stampa di debug
#define USE_DEBUG

class Debug {
public:
  // Costruttore
  Debug() {}

  // Funzione per stampare un messaggio senza una nuova riga
  void print(const String& message) {
#ifdef USE_DEBUG
    printMillis();
    Serial.print(message);
#endif
  }

  // Funzione per stampare un messaggio senza una nuova riga e senza millis
  void printp(const String& message) {
#ifdef USE_DEBUG
    Serial.print(message);
#endif
  }

  // Funzione per stampare un messaggio formattato
  void printf(const char* format, ...) {
#ifdef USE_DEBUG
    printMillis();
    va_list args;
    va_start(args, format);
    Serial.vprintf(format, args);
    va_end(args);
#endif
  }

  // Funzione per stampare un messaggio con una nuova riga
  void println(const String& message) {
#ifdef USE_DEBUG
    printMillis();
    Serial.println(message);
#endif
  }

  // Funzione per stampare una nuova riga
  void println() {
#ifdef USE_DEBUG
    printMillis();
    Serial.println();
#endif
  }

  // Funzione di debug senza testo, solo una nuova riga
  void printspc(void) {
#ifdef USE_DEBUG
    Serial.println(F(""));
#endif
  }

private:
  // Funzione privata per stampare il timestamp
  void printMillis() {
    Serial.print("[");
    Serial.print(millis());
    Serial.print("] ");
  }
};
