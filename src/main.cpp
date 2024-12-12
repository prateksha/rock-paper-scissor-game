#include <WiFi.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// WiFi credentials
const char* ssid = "Tempo - Resident";
const char* password = "ChargersRobotAce";

// Pins for touch sensors
const int touchThumbPin = 14;
const int touchIndexPin = 12;
const int touchMiddlePin = 13;
const int touchRingPin = 27;
const int touchLittlePin = 33;

// Servo motor configuration
const int servo1Pin = 18; 
const int servo2Pin = 19; 
const int servo3Pin = 21; 
const int pwmChannel1 = 0;
const int pwmChannel2 = 1;
const int pwmChannel3 = 2;
const int pwmFrequency = 50;
const int pwmResolution = 16;

// Score tracking variables
int humanScore = 0;
int robotScore = 0;

// Game gesture variables
String humanGesture = "unknown";
String robotGesture = "unknown";
String result = "Game not started yet";

// Countdown timer
int countdown = 5;

// Web server setup
AsyncWebServer server(80);

// Function to map angle to duty cycle for servo
void setServoAngle(int pwmChannel, int angle) {
  int dutyCycle = map(angle, 0, 180, 1638, 8192);
  ledcWrite(pwmChannel, dutyCycle);
}

// Function to handle servo gestures
void showRobotGesture(String gesture) {
  if (gesture == "rock") {
    setServoAngle(pwmChannel1, 180);
    setServoAngle(pwmChannel2, 180);
    setServoAngle(pwmChannel3, 180);
  } else if (gesture == "paper") {
    setServoAngle(pwmChannel1, 0);
    setServoAngle(pwmChannel2, 0);
    setServoAngle(pwmChannel3, 0);
  } else if (gesture == "scissors") {
    setServoAngle(pwmChannel1, 180);
    setServoAngle(pwmChannel2, 0);
    setServoAngle(pwmChannel3, 180);
  }
}

// Function to generate random robot gesture
String randomRobotGesture() {
  int randValue = random(0, 3);
  if (randValue == 0) return "rock";
  if (randValue == 1) return "paper";
  return "scissors";
}



// Function to read human gesture
String detectHumanGesture() {
  int thumbTouch = digitalRead(touchThumbPin);
  int indexTouch = digitalRead(touchIndexPin);
  int middleTouch = digitalRead(touchMiddlePin);
  int ringTouch = digitalRead(touchRingPin);
  int littleTouch = digitalRead(touchLittlePin);

  if ((thumbTouch || ringTouch || littleTouch) && !(indexTouch || middleTouch)) {
    return "scissors";
  } else if ((thumbTouch || ringTouch || littleTouch) && (indexTouch || middleTouch)) {
    return "rock";
  } else if (!(thumbTouch || ringTouch || littleTouch) && !(indexTouch || middleTouch)) {
    return "paper";
  } else {
    return "unknown";
  }
}
String getHTML() {
  String html = "<!DOCTYPE html><html><head><title>Rock-Paper-Scissors Game</title><style>"
                "body { font-family: Arial, sans-serif; text-align: center; padding: 20px; }"
                "h1 { font-size: 36px; }"
                "#score { font-size: 24px; margin-top: 20px; }"
                "#gestures { font-size: 20px; margin-top: 20px; }"
                "#countdown { font-size: 20px; margin-top: 20px; }"
                ".btn { padding: 10px 20px; font-size: 18px; cursor: pointer; margin-top: 20px; }</style></head>"
                "<body><h1>Rock-Paper-Scissors Game</h1>"
                "<div id='score'></div>"
                "<div id='gestures'></div>"
                "<div id='countdown'></div>"
                "<button class='btn' onclick='startNewGame()'>Start New Game</button>"
                "<script>"
                "let countdown = 0;"
                "function startNewGame() {"
                "  fetch('/start-game');"
                "  document.getElementById('gestures').innerHTML = 'Game Started!';"
                "  document.getElementById('countdown').innerHTML = 'Get ready! Gesture in: 5';"
                "}"
                "function updateGameStatus() {"
                "  fetch('/game-status')"
                "    .then(response => response.json())"
                "    .then(data => {"
                "      document.getElementById('score').innerText = 'Human: ' + data.humanScore + ' | Robot: ' + data.robotScore;"
                "      document.getElementById('gestures').innerText = 'Human Gesture: ' + data.humanGesture + ' | Robot Gesture: ' + data.robotGesture;"
                "      countdown = data.countdown;"
                "      document.getElementById('countdown').innerHTML = 'Get ready! Gesture in: ' + countdown;"
                "      if (data.gameOver) {"
                "        document.getElementById('gestures').innerHTML = 'Game Over!';"
                "        if(data.humanScore > data.robotScore) {"
                "          document.getElementById('countdown').innerHTML = 'Human Wins!';"
                "        } else {"
                "          document.getElementById('countdown').innerHTML = 'Robot Wins!';"
                "        }"
                "      }"
                "    });"
                "}"
                "setInterval(updateGameStatus, 1000);"
                "</script></body></html>";
  return html;
}

// Game state variables
bool gameOver = false; // New flag to track if the game is over

// Updated function to reset the game
void startNewGame(AsyncWebServerRequest *request) {
  humanScore = 0;
  robotScore = 0;
  humanGesture = "unknown";
  robotGesture = "unknown";
  result = "Game not started yet";
  countdown = 5;
  gameOver = false; // Reset game over state
  request->send(200, "text/plain", "Game Started");
}

// Updated function to determine the winner and stop the game at 3 points
String determineWinner(String humanGesture, String robotGesture) {
  if (humanGesture == robotGesture) return "Draw!";
  if (humanGesture == "rock" && robotGesture == "scissors") return "Human Wins!";
  if (humanGesture == "paper" && robotGesture == "rock") return "Human Wins!";
  if (humanGesture == "scissors" && robotGesture == "paper") return "Human Wins!";
  return "Robot Wins!";
}

void handleGameStatus(AsyncWebServerRequest *request) {
  String gameStatus = "{\"humanScore\": " + String(humanScore) +
                      ", \"robotScore\": " + String(robotScore) +
                      ", \"humanGesture\": \"" + humanGesture + "\"" +
                      ", \"robotGesture\": \"" + robotGesture + "\"" +
                      ", \"gameOver\": " + (gameOver ? "true" : "false") + 
                      ", \"countdown\": " + String(countdown) + "}";
  request->send(200, "application/json", gameStatus);
}


void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  pinMode(touchThumbPin, INPUT);
  pinMode(touchIndexPin, INPUT);
  pinMode(touchMiddlePin, INPUT);
  pinMode(touchRingPin, INPUT);
  pinMode(touchLittlePin, INPUT);

  ledcSetup(pwmChannel1, pwmFrequency, pwmResolution);
  ledcAttachPin(servo1Pin, pwmChannel1);
  ledcSetup(pwmChannel2, pwmFrequency, pwmResolution);
  ledcAttachPin(servo2Pin, pwmChannel2);
  ledcSetup(pwmChannel3, pwmFrequency, pwmResolution);
  ledcAttachPin(servo3Pin, pwmChannel3);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getHTML());
  });
  server.on("/game-status", HTTP_GET, handleGameStatus);
  server.on("/start-game", HTTP_GET, startNewGame);

  server.begin();
}

unsigned long lastTime = 0;
const unsigned long interval = 1000;


// Main game loop logic
void loop() {
  static bool gameInProgress = false;

  // Exit game logic if game is already over
  if (gameOver) return;

  // Handle countdown timer
  if (millis() - lastTime >= interval && countdown > 0) {
    lastTime = millis();
    countdown--;
    Serial.print("Countdown: ");
    Serial.println(countdown);
  }

  // When countdown reaches 0, process the game logic
  if (countdown == 0 && !gameInProgress) {
    gameInProgress = true;

    // Detect human gesture
    humanGesture = detectHumanGesture();
    Serial.print("Human Gesture: ");
    Serial.println(humanGesture);

    // Generate random robot gesture
    robotGesture = randomRobotGesture();
    Serial.print("Robot Gesture: ");
    Serial.println(robotGesture);

    // Move servos to display the robot gesture
    showRobotGesture(robotGesture);

    // Determine the winner
    result = determineWinner(humanGesture, robotGesture);
    Serial.println(result);

    // Update scores
    if (result == "Human Wins!") humanScore++;
    else if (result == "Robot Wins!") robotScore++;

    // Check if game is over
    if (humanScore == 3 || robotScore == 3) {
      gameOver = true;
      Serial.println("Game Over!");
    } else {
      // Reset countdown for next round
      countdown = 5;
    }

    gameInProgress = false;
  }
}