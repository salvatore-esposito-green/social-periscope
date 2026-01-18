# 📟 SONAR BADGE: Codemotion Submarine Edition
_Protocollo di Aggancio Sociale tramite ESP-NOW_

Il **Sonar Badge** è un dispositivo di proximity-networking progettato per trasformare i corridoi di Codemotion in un oceano digitale. Dimentica il classico networking: qui siamo in modalità "**Battaglia Navale Collaborativa**".

## 🚀 La Tecnologia

A differenza del comune Bluetooth, il badge utilizza il protocollo **ESP-NOW** a 2.4GHz.

- **Zero Latenza**: Nessun accoppiamento richiesto. I sottomarini (badge) comunicano istantaneamente.

- **Rilevamento Passivo**: Il badge emette un "Ping" radio ogni 300ms.

- **Analisi RSSI**: Il sistema analizza la potenza del segnale ricevuto per calcolare la distanza dei sottomarini amici.

## 📡 Modalità PERISCOPIO (Sonar On)

Quando il tuo badge rileva un segnale radio amico, si attiva l'interfaccia acustica **Sonar**:

1. **Fase di Avvicinamento (Range 5m - 2m)**: Il buzzer emette un beep lento e profondo. È il primo contatto radar.

2. **Fase di Intercettazione (Range 2m - 1m)**: Il ritmo accelera progressivamente e il tono diventa più acuto. La tensione sale.

3. **Punto di Collisione (Sotto 1 metro)**: Il Sonar "impazzisce". Il ritmo raggiunge l'apice (50ms) e il suono diventa frenetico. **È il segnale: il sottomarino amico è di fronte a te!**

## 💬 Missione: "Quattro Chiacchiere"

Il Sonar Badge non serve a distruggere, ma a connettere. Una volta raggiunto l'apice del suono, il protocollo di bordo impone di:

- Emergere in superficie.

- Disattivare il sonar.

- Iniziare una conversazione reale con il "comandante" dell'altro badge.