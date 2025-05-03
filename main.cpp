#include "mbed.h"

#define CODE_LENGTH 4

char correctCode[CODE_LENGTH] = {'1', '3', '5', '9'};
char enterCode[CODE_LENGTH];
int enterDigits = 0;

AnalogIn potentiometer(A0);
DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);
PwmOut buzzer(D9);
UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

float potentiometerReading = 0.00f;   // Raw ADC input A1 value

DigitalOut keypadRow[4] = {PB_3, PB_5, PC_7, PA_15};
DigitalIn keypadCol[4] = {PB_12, PB_13, PB_15, PC_6};

char matrixKeypadIndexToCharArray[] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

void inputsInit() {
    for(int i = 0; i < 4; i++) {
        keypadCol[i].mode(PullUp);
    }
}

void outputsInit() {
    alarmLed = 0;
    incorrectCodeLed = 0;
    buzzer = 0;
}

char matrixKeypadScan() {
    for(int row = 0; row < 4; row++) {
        for(int i = 0; i < 4; i++) {
            keypadRow[i] = 1;
        }
        keypadRow[row] = 0;

        for(int col = 0; col < 4; col++) {
            if(keypadCol[col] == 0) {
                return matrixKeypadIndexToCharArray[row * 4 + col];
            }
        }
    }
    return '\0';
}

char matrixKeypadUpdate() {
    static char lastKey = '\0';
    static Timer debounceTimer;
    static bool debounceTimerStarted = false;
    char currentKey = matrixKeypadScan();

    if(currentKey != '\0' && lastKey == '\0' && !debounceTimerStarted) {
        debounceTimer.reset();
        debounceTimer.start();
        debounceTimerStarted = true;
    }

    if(debounceTimerStarted && debounceTimer.elapsed_time().count() > 20000) {
        debounceTimer.stop();
        debounceTimerStarted = false;
        lastKey = currentKey;
        if(currentKey != '\0') {
            return currentKey;
        }
    }

    if(currentKey == '\0') {
        lastKey = '\0';
    }

    return '\0';
}

int main() {
    inputsInit();
    outputsInit();

    uartUsb.write("\nSystem is on\r\n", 15);

    bool alarmActivated = false;

    while (true) {

        float potentiometerReading = potentiometer.read();

        if (potentiometer.read() >=0.51f && !alarmActivated) {
            alarmActivated = true;
            uartUsb.write("\nAlarm activated - potentiometer value above 0.50\r\n", 52);
            alarmLed = 1;
        }

        char key = matrixKeypadUpdate();

        if (key != '\0') {
            if(key >= '0' && key <= '9') {
                if(enterDigits < CODE_LENGTH) {
                enterCode[enterDigits++] = key;
                uartUsb.write(&key, 1);
                }
            } else if(key == '#') {
                if(enterDigits == CODE_LENGTH) {
                    //uartUsb.write("\r\nCode entered: ", 16);
                    //uartUsb.write(enterCode, CODE_LENGTH);
                    //uartUsb.write("\r\n", 2);
                    bool correct = true;
                    for(int i = 0; i < CODE_LENGTH; i++) {
                        if(enterCode[i] != correctCode[i]) {
                            correct = false;
                            break;
                        }
                    }

                    if (correct && alarmActivated) {
                        alarmActivated = false;
                        alarmLed = 0;
                        buzzer = 0;
                        uartUsb.write("\r\nAlarm deactivated\r\n", 22);
                        break;
                    } else {
                        uartUsb.write("\r\nIncorrect code\r\n", 18);
                        incorrectCodeLed = 1;
                    }
                } else {
                    uartUsb.write("\r\nPlease enter 4 digits\r\n", 25);
                }
                enterDigits = 0;
            }
        }

        if (alarmActivated) {
        buzzer.period(1.0/100);
        buzzer = 10; 
        ThisThread::sleep_for(200ms);
        buzzer.period(1.0/105);
        buzzer = 10;
        ThisThread::sleep_for(200ms);
        buzzer = 0;
        }
    
        ThisThread::sleep_for(1ms);
    }
}
