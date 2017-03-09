#include <String.h>

#define redLed 10
#define greenLed 9
#define blueLed 6

#define b1 2
#define b2 3
#define b3 4
#define b4 5
#define var A0

const int varSampleSize = 5;
const int baudRate = 19200;
const int delayVal = 1;

const int fadeSpeed = 50;

int speedCounter = 0;
int ledColor[3] = {255, 0, 0};
int colorStep = 0;

struct State {
    int key1;
    int key2;
    int key3;
    int key4;
    int var;
};

struct State state = {.key1 = 0, .key2 = 0, .key3 = 0, .key4 = 0, .var = 0};

void setLedState(int color[3]) {
    analogWrite(redLed, color[0]);
    analogWrite(greenLed, color[1]);
    analogWrite(blueLed, color[2]);
}

void stepColor(int color[3]) {
    if (color[0] >= 255) {
        colorStep = 0;
    } else if (color[1] >= 255) {
        colorStep = 1;
    } else if (color[2] >= 255) {
        colorStep = 2;
    }

    if (colorStep == 0) {
        color[0]--;
        color[1]++;
    } else if (colorStep == 1) {
        color[1]--;
        color[2]++;
    } else if (colorStep == 2) {
        color[2]--;
        color[0]++;
    }
}

String serialize(struct State &state) {
    String data = String();

    data += state.key1;
    data += "$";
    data += state.key2;
    data += "$";
    data += state.key3;
    data += "$";
    data += state.key4;
    data += "$";
    data += state.var;
    data += "$\n";

    return data;
}

void sendState(struct State &state) {
    Serial.flush();
    if (Serial.availableForWrite())
        Serial.print(serialize(state));
}

bool checkState(struct State &state) {
    int send = 0;

    if (state.key1 != !digitalRead(b1)) {
        state.key1 = !digitalRead(b1);
        send = 1;
    }

    if (state.key2 != !digitalRead(b2)) {
        state.key2 = !digitalRead(b2);
        send = 1;
    }

    if (state.key3 != !digitalRead(b3)) {
        state.key3 = !digitalRead(b3);
        send = 1;
    }

    if (state.key4 != !digitalRead(b4)) {
        state.key4 = !digitalRead(b4);
        send = 1;
    }

    int varVal = 0;
    for (int i = 0; i < varSampleSize; i++) varVal += analogRead(var);
        varVal /= varSampleSize;

    varVal = 1024 - varVal;
    if (state.var >= varVal + 2 || state.var < varVal - 2) {
        state.var = varVal;
        send = 1;
    }

    return send;
}

void setup() {
    Serial.begin(baudRate);

    pinMode(redLed, OUTPUT);
    pinMode(greenLed, OUTPUT);
    pinMode(blueLed, OUTPUT);

    setLedState(ledColor);

    pinMode(b1, INPUT);
    digitalWrite(b1, HIGH); // pull the pin high to avoid floating
    pinMode(b2, INPUT);
    digitalWrite(b2, HIGH); // pull the pin high to avoid floating
    pinMode(b3, INPUT);
    digitalWrite(b3, HIGH); // pull the pin high to avoid floating
    pinMode(b4, INPUT);
    digitalWrite(b4, HIGH); // pull the pin high to avoid floating
}

void loop() {
    speedCounter++;

    delay(delayVal);

    if (checkState(state))
        sendState(state);

    if (speedCounter >= fadeSpeed) {
        stepColor(ledColor);
        setLedState(ledColor);
        speedCounter = 0;
    }
}
