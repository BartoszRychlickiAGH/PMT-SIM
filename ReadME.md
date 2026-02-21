### PMT-SIM:
* **Hardware Core**: Obsługa układu zgodnie ze specyfikacją: [AD5361 Datasheet](https://www.alldatasheet.com/datasheet-pdf/view/521541/AD/AD5361BSTZ-REEL1.html).
* **Voltage Control**: Firmware umożliwia precyzyjne ustawienie napięcia w zakresie od **-10V do 10V**.
* **Output Capabilities**: 
    * **Firmware**: Oprogramowanie niskopoziomowe wspiera pełną kontrolę nad **6 niezależnymi kanałami wyjściowymi**.
    * **GUI**: Obecna implementacja po stronie interfejsu graficznego (GUI) umożliwia sterowanie **2 wybranymi kanałami**.6)


## How to Run

Uruchomienie projektu wymaga poprawnego przygotowania warstwy sprzętowej oraz wgrania firmware'u za pomocą zintegrowanych narzędzi płytki Nucleo.

### 1. Hardware Setup
* **Integrated Programmer:** Płytka **Nucleo-F103RB** posiada wbudowany programator **ST-LINK V2-1**. Nie są wymagane zewnętrzne urządzenia – wystarczy połączenie z komputerem za pomocą kabla USB.
* **Connectivity:** * Upewnij się, że zworki na złączu **CN2** (ST-LINK) są założone, co umożliwia programowanie mikrokontrolera na płytce.
    * Połącz piny zgodnie z sekcją [Pinout](#pinout), aby zapewnić komunikację z modułem PMT-SIM.



### 2. Flashing the Firmware
Proces kompilacji i wgrywania oprogramowania w środowisku **STM32CubeIDE**:

1. **Import:** Otwórz środowisko i zaimportuj projekt **PMT_Tests**.
2. **Build:** Kliknij ikonę **młotka** (Build) na górnym pasku. Proces zakończy się sukcesem, gdy w konsoli pojawi się informacja o wygenerowaniu pliku `.elf`.
3. **Run:** Kliknij zieloną ikonę **Run** (strzałka). IDE automatycznie:
    * Wykryje wbudowany programator ST-LINK.
    * Zaprogramuje pamięć Flash mikrokontrolera.
    * Zresetuje układ, uruchamiając nową wersję oprogramowania.

	  
## Authors:

| No. | Author | Role / Responsibility |
| :--- | :--- | :--- |
| 1 | **Bartosz Rychlicki** | **Embedded** – Firmware development |
| 2 | **Michał Gądek** | **GUI** – User interface design |
| 3 | **Michał Jurek** | **Embedded** – Firmware development |

## Functonalities:
### 1. DAC & Signal Management
* **DAC Configuration**: Inicjalizacja i ustawianie przetwornika DAC w odpowiednim trybie pracy.
* **Triggering HITs**: Generowanie i wysyłanie sygnałów HIT1/HIT2 do wyjść PCB.

### 2. Signal Routing & Voltage Control
* **Output Selection**: Wybór wyjścia PMT-SIM (kanały 1-6).
* **Voltage Regulation**: Precyzyjne odbieranie zadanej wartości napięcia i ustawianie go na wybranym wyjściu fizycznym.

### 3. Communication & Feedback
* **UART Interface**: Odbieranie komend sterujących oraz wysyłanie potwierdzeń wykonania zadań (firmware response) do GUI.
* **Error Handling**: Weryfikacja poprawności komend i raportowanie statusu (OK/ER).


## Pinout

| STM32 Pin | Function | Description |
| :--- | :--- | :--- |
| **PA2** | UART TX | Debug/Communication Output |
| **PA3** | UART RX | Debug/Communication Input |
| **PA5** | SPI SCK | Serial Clock |
| **PA6** | SPI MISO | Master In Slave Out (SDO) |
| **PA7** | SPI MOSI | Master Out Slave In (SDI) |
| **PB3** | GPIO | HIT1 Signal |
| **PB4** | GPIO | HIT2 Signal |
| **PB5** | GPIO | nSYNC Signal (Active Low) |

	
## Communication Protocol (UART)

Komunikacja odbywa się za pomocą komend tekstowych z separatorem średnikowym (`;`). Każda komenda musi być zakończona znakami `\r\n`.

### I. Received Commands (RX)
*Komendy wysyłane do STM32 w celu sterowania wyjściami PCB.*

| Command Format | Action | Description |
| :--- | :--- | :--- |
| `h;<1/2>\r\n` | **Trigger HIT** | Aktywuje wyjście HIT1 (dla `1`) lub HIT2 (dla `2`). |
| `<1/6>;<-10/10>\r\n` | **Set Voltage** | Ustawia napięcie (zakres -10V do 10V) na kanale (1-6). |



### II. Transmitted Confirmations (TX)
*Informacje zwrotne wysyłane do GUI po przetworzeniu komendy.*

| Response Format | Status | Description |
| :--- | :--- | :--- |
| `h;<1/2>;<OK/ER>` | **HIT Status** | Potwierdzenie (`OK`) lub błąd (`ER`) wykonania wyzwalacza. |
| `<1/6>;<-10/10>;<OK/ER>` | **Voltage Status** | Potwierdzenie (`OK`) lub błąd (`ER`) ustawienia napięcia. |

> **Note:** Status `ER` jest zwracany w przypadku odebrania wartości spoza zdefiniowanych zakresów (np. kanał `7` lub napięcie `15V`).
			
## Changelog

| Date & Time | Event / Milestone | Details |
| :--- | :--- | :--- |
| **29.01.2026** <br> 04:43 AM | **Firmware Update** | Files updated based on recent developments; functional tests conducted. |
| **22.01.2026** <br> 10:45 AM | **Functional Tests** | Tests successful. Identified and resolved a minor issue with 3rd mode frequency measurement. |
| **19.01.2026** <br> 12:44 AM | **Implementation Push** | Codebase pushed and finalized for functional testing phase. |
| **27.10.2025** <br> 05:47 PM | **Project Kick-off** | GitHub repository was officially created. |