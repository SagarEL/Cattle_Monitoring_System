#include <SoftwareSerial.h>
#include <TinyGPS++.h>
 
// SoftwareSerial for GSM and GPS communication
SoftwareSerial gsmSerial(0, 2); // RX, TX for GSM 800L
SoftwareSerial gpsSerial(5, 4); // RX, TX for GPS Module

TinyGPSPlus gps; // GPS object

String registeredNumber = "+917676160339"; // Store the mobile number for SMS
unsigned long previousMillis = 0; // Track the last time SMS was sent
const unsigned long interval = 1800000; // 30 minutes in milliseconds

void setup() {
  Serial.begin(115200);        // Serial monitor for debugging
  gsmSerial.begin(9600);       // GSM baud rate
  gpsSerial.begin(9600);       // GPS baud rate

  // Initialize GSM module
  gsmSerial.println("AT");       // Test GSM connection
  delay(100);
  gsmSerial.println("AT+CMGF=1"); // Set SMS to text mode
  delay(100);
  gsmSerial.println("AT+CNMI=1,2,0,0,0"); // Enable SMS notification
  delay(100);

  Serial.println("System Initialized...");
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if 30 minutes have passed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Fetch and send the GPS location
    String location = fetchGPSLocation();
    if (location != "") {
      String googleMapsLink = generateGoogleMapsLink(location);
      sendSMS(registeredNumber, googleMapsLink); // Send location link
    } else {
      sendSMS(registeredNumber, "Unable to fetch GPS location.");
    }
  }

  // Handle incoming SMS or calls
  if (gsmSerial.available()) {
    String inputString = gsmSerial.readString();
    Serial.println(inputString);

    // Handle Incoming Call
    if (inputString.indexOf("RING") != -1) {
      // Hang up the call
      delay(1000);
      gsmSerial.println("ATH"); // Send hang-up command to the GSM module
      delay(1000);

      // Send location link when a call is detected
      String location = fetchGPSLocation();
      if (location != "") {
        String googleMapsLink = generateGoogleMapsLink(location);
        sendSMS(registeredNumber, googleMapsLink); // Send location link
      } else {
        sendSMS(registeredNumber, "Unable to fetch GPS location.");
      }
    }

    // Handle SMS Message
    if (inputString.indexOf("+CMT:") != -1) {
      String senderNumber = extractSenderNumber(inputString);
      String messageBody = extractMessageBody(inputString);

      // If the message is "Getloc", process the request
      if (messageBody.equalsIgnoreCase("Getloc")) {
        if (registeredNumber == "" || registeredNumber == senderNumber) {
          registeredNumber = senderNumber;
          String location = fetchGPSLocation();

          if (location != "") {
            String googleMapsLink = generateGoogleMapsLink(location);
            sendSMS(registeredNumber, googleMapsLink); // Send location link
          } else {
            sendSMS(registeredNumber, "Unable to fetch GPS location.");
          }
        } else {
          sendSMS(senderNumber, "Access Denied.");
        }
      }
    }
  }
}

// Extract sender number from the SMS response
String extractSenderNumber(String smsResponse) {
  int start = smsResponse.indexOf("\"") + 1; // Find the first quote
  int end = smsResponse.indexOf("\"", start); // Find the second quote
  String senderNumber = smsResponse.substring(start, end); // Extract number
  return senderNumber;
}

// Extract SMS message body
String extractMessageBody(String smsResponse) {
  int bodyStart = smsResponse.indexOf("\n", smsResponse.indexOf("+CMT:")) + 1;
  String message = smsResponse.substring(bodyStart);
  message.trim(); // Trim the message
  return message;
}

// Fetch GPS location from GPS module
String fetchGPSLocation() {
  String location = "";
  unsigned long timeout = millis() + 5000; // 5 seconds timeout to get GPS data

  while (millis() < timeout) {
    if (gpsSerial.available()) {
      gps.encode(gpsSerial.read()); // Encode GPS data

      if (gps.location.isUpdated()) {
        // Format location as Latitude, Longitude
        location = String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
        break;
      }
    }
  }

  return location;
}

// Generate Google Maps link from location coordinates
String generateGoogleMapsLink(String location) {
  String googleMapsLink = "https://www.google.com/maps?q=" + location;
  return googleMapsLink;
}

// Send SMS with location info or any message
void sendSMS(String number, String message) {
  gsmSerial.println("AT+CMGS=\"" + number + "\"");
  delay(100);
  gsmSerial.println(message);
  delay(100);
  gsmSerial.write(26); // Send SMS with Ctrl+Z
  delay(1000);
  Serial.println("SMS sent to " + number + ": " + message);
}
