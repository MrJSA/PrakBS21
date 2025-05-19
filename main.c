#include <stdio.h>
#include "sub.h"
#include "keyValStore.h"

int main() {
    init_store();           // Shared Memory & Semaphor initialisieren
    run_server(5678);       // Server starten
    return 0;
}
