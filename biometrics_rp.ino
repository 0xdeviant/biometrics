#include <Adafruit_Fingerprint.h>
#include <ezButton.h>
#define DETECT_PIN 8
#define BUTTON_PIN_ENTRANCE 7
#define DETECT_OFF_PIN 6

ezButton entranceButton(7);

uint8_t id = 0;

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial entranceSerial(2, 3);
SoftwareSerial exitSerial(10, 11);

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1
#endif

// Entrance fingerprint module
Adafruit_Fingerprint finger_entrance = Adafruit_Fingerprint(&entranceSerial);
Adafruit_Fingerprint finger_exit = Adafruit_Fingerprint(&exitSerial);

// Exit fingerprint module

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  entranceButton.setDebounceTime(50);
  
  pinMode(DETECT_PIN, OUTPUT);

  exitSerial.begin(57600);
  finger_exit.LEDcontrol(false);
  exitSerial.end();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(200);
  if (Serial.available() > 0) {
    uint8_t method = Serial.read() - '0';
    switch(method) {
      // DETECT USERS
      case 1:
        entranceSerial.begin(57600);
        while (getFingerprintIDez(finger_entrance) == -1);
        delay(5);
        entranceSerial.end();
        Serial.end();
        Serial.begin(9600);
        break;
      // ENROLL USERS        
      case 2:
        if (Serial.available() > 0) {
          delay(200);
          Serial.flush();
          id = Serial.readStringUntil('\n').toInt();
          // entrance sensor
          entranceSerial.begin(57600);
          while (! getFingerprintEnroll(finger_entrance));
          entranceSerial.end();
          //exit sensor
          exitSerial.begin(57600);
          while (! getFingerprintEnroll(finger_exit));
          finger_exit.LEDcontrol(false);
          exitSerial.end();
          delay(200);
          Serial.end();
          Serial.begin(9600);
          break;
        }
      case 3:
      // DELETE USERS
        if (Serial.available() > 0) {
          Serial.flush();
          id = Serial.readStringUntil('\n').toInt();

//        DELETE for entrance sensor
          entranceSerial.begin(57600);
          while (deleteFingerprint(id, finger_entrance) == -1);     
          entranceSerial.end();     

          exitSerial.begin(57600);
          while (deleteFingerprint(id, finger_exit) == -1);
          exitSerial.end();

          delay(200);
          Serial.end();
          Serial.begin(9600);
          break;
        }
      case 4:
        exitSerial.begin(57600);
        finger_exit.LEDcontrol(true);
        while (getFingerprintIDez(finger_exit) == -1);
        delay(5);
        finger_exit.LEDcontrol(false);
        exitSerial.end();
        Serial.end();
        Serial.begin(9600);
        break;
      default:
        delay(200);
        break;
    }
  }    
}

// ##################
//   DETECT METHOD
// ##################

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez(Adafruit_Fingerprint finger) {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  uint8_t confidence = finger.confidence;
  if (confidence > 100) {
    Serial.println(finger.fingerID);
    Serial.flush();
    digitalWrite(DETECT_PIN, HIGH);
    delay(3000);
    digitalWrite(DETECT_PIN, LOW);
    delay(200);

    return finger.fingerID;
  }
 
  return -1;
}

// ######################
// DELETE METHOD
// ######################
uint8_t deleteFingerprint(uint8_t id, Adafruit_Fingerprint finger) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
    return 1;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}
// ######################
// ENROLL METHOD ENTRANCE
// ######################

uint8_t getFingerprintEnroll(Adafruit_Fingerprint finger) {
  int p = -1;
//  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  finger_exit.LEDcontrol(false);
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  finger_exit.LEDcontrol(true);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}
