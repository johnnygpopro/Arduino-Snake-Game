#include <Arduino_LED_Matrix.h>
#include <cmath>
#include <stdlib.h>

// Structs
struct Snake {
  int xDirection;
  int yDirection;
  int *xPosition;
  int *yPosition;
  int xPositionTailPrev;
  int yPositionTailPrev;
  int size;
};

struct Apple {
  int xPosition;
  int yPosition;
  bool createApple;
};

// Constants
const int UPDATE_INTERVAL = 250; // Milliseconds
const int UP_PIN = 6;
const int DOWN_PIN = 5;
const int LEFT_PIN = 4;
const int RIGHT_PIN = 3;
const int PIEZO_PIN = 2;

const int APPLE_EAT_SOUND_FREQUENCY = 700;
const int APPLE_EAT_SOUND_DURATION = 50; // Milliseconds

const int DEATH_SOUND_FREQUENCY = 800;
const int DEATH_SOUND_DURATION = 100;

// Misc
int currentTime = 0; // Used to track the update interval

bool startGame = true;

int xDirectionPrev = 0; // Used to prevent snake from going backwards
int yDirectionPrev = 0;

bool displayMatrix[8][12] = {
  {false, false, false, false, false, false, false, false, false, false, false, false},
  {false, false, false, false, false, false, false, false, false, false, false, false},
  {false, false, false, false, false, false, false, false, false, false, false, false},
  {false, false, false, false, false, false, false, false, false, false, false, false},
  {false, false, false, false, false, false, false, false, false, false, false, false},
  {false, false, false, false, false, false, false, false, false, false, false, false},
  {false, false, false, false, false, false, false, false, false, false, false, false},
  {false, false, false, false, false, false, false, false, false, false, false, false}
};

struct Snake globalSnake;
struct Apple globalApple;

ArduinoLEDMatrix matrix;

uint32_t* convertDisplayMatrixToInts()
{

  uint32_t* startPtr = (uint32_t*)malloc(sizeof(uint32_t) * 3);

  // Clear garbage values
  *startPtr = 0;
  *(startPtr + 1) = 0;
  *(startPtr + 2) = 0;

  int counter = 0;

  for (int i = 7; i > -1; --i)
  {
    for (int j = 11; j > -1; --j)
    {

      int offset = 2 - (counter / 32);
      int exponent = counter % 32;

      if (displayMatrix[i][j] == true)
      {
        *(startPtr + offset) += pow(2, exponent);
      }

      counter += 1;
    }
  }

  return startPtr;

}

void displayCurrentFrame()
{

  uint32_t* startPtr = convertDisplayMatrixToInts();

  uint32_t currentFrame[] = {
    *startPtr,
    *(startPtr + 1),
    *(startPtr + 2),
  };

  matrix.loadFrame(currentFrame);

  // Clear heap memory
  free(startPtr);

}

// Turns off all led (false in the array)
void clearDisplayMatrix()
{
  for (int i = 0; i < 8; ++i)
  {
    for (int j = 0; j < 12; ++j)
    {
      displayMatrix[i][j] = false;
    }
  }
}

// Function to init snake struct
void initGlobalSnake()
{
  globalSnake.size = 0;
  globalSnake.xPosition = nullptr;
  globalSnake.yPosition = nullptr;
  globalSnake.xDirection = 0;
  globalSnake.yDirection = 0;
  addSnakeSegment(0, 0);
}

void initGlobalApple()
{
  globalApple.xPosition = -1;
  globalApple.yPosition = -1;
  globalApple.createApple = true;
}

void updateAppleInDisplayMatrix()
{
  displayMatrix[globalApple.yPosition][globalApple.xPosition] = true;
}

bool checkDeath()
{

  // Predict the next position and check if dead
  int xPositionHead = *globalSnake.xPosition + globalSnake.xDirection;
  int yPositionHead = *globalSnake.yPosition + globalSnake.yDirection;

  // Check if head is out of bounds
  if (xPositionHead < 0 || xPositionHead > 11)
  {
    return true;
  };

  if (yPositionHead < 0 || yPositionHead > 7)
  {
    return true;
  }

  // Check if head will collide with a body part
  for (int i = 1; i < globalSnake.size; ++i)
  {
    int xPositionCurrent = *(globalSnake.xPosition + i);
    int yPositionCurrent = *(globalSnake.yPosition + i);

    if (xPositionCurrent == xPositionHead && yPositionCurrent == yPositionHead)
    {
      return true;
    }

  }

  return false;

}

// Removes any used heap memory
void cleanUp()
{

  free(globalSnake.xPosition);
  free(globalSnake.yPosition);

}

bool processDeath()
{

  // Check if dead
  if (checkDeath() == false)
  {
    return false;
  }

  // Play death sound
  tone(PIEZO_PIN, DEATH_SOUND_FREQUENCY, DEATH_SOUND_DURATION);

  // Death animation
  clearDisplayMatrix();
  updateSnakeInDisplayMatrix();
  displayCurrentFrame();
  delay(300);

  for (int i = globalSnake.size - 1; i > -1; --i)
  {
    displayMatrix[*(globalSnake.yPosition + i)][*(globalSnake.xPosition + i)] = false;
    displayCurrentFrame();
    tone(PIEZO_PIN, APPLE_EAT_SOUND_FREQUENCY, APPLE_EAT_SOUND_DURATION);
    delay(100);
  }

  for (int i = 0; i < 4; ++i)
  {

    updateSnakeInDisplayMatrix();
    displayCurrentFrame();
    tone(PIEZO_PIN, APPLE_EAT_SOUND_FREQUENCY, APPLE_EAT_SOUND_DURATION);
    delay(300);

    for (int j = globalSnake.size - 1; j > -1; --j)
    {
      displayMatrix[*(globalSnake.yPosition + j)][*(globalSnake.xPosition + j)] = false;
    }
    displayCurrentFrame();

    delay(300);
  }

  // Clean up
  cleanUp();

  // Indicate new game start
  startGame = true;

  return true;

}

bool processWin()
{

  if (globalSnake.size != 8 * 12)
  {
    return false;
  }

  // Clean up
  cleanUp();
  
  // Indicate new game start
  startGame = true;

  return true;

}

void updateSnakeDirection()
{

  if (digitalRead(UP_PIN) == HIGH && (yDirectionPrev != 1 || globalSnake.size == 1))
  {
    globalSnake.xDirection = 0;
    globalSnake.yDirection = -1;
  }

  if (digitalRead(DOWN_PIN) == HIGH && (yDirectionPrev != -1 || globalSnake.size == 1))
  {
    globalSnake.xDirection = 0;
    globalSnake.yDirection = 1;
  }

  if (digitalRead(LEFT_PIN) == HIGH && (xDirectionPrev != 1 || globalSnake.size == 1))
  {
    globalSnake.xDirection = -1;
    globalSnake.yDirection = 0;
  }

  if (digitalRead(RIGHT_PIN) == HIGH && (xDirectionPrev != -1 || globalSnake.size == 1))
  {
    globalSnake.xDirection = 1;
    globalSnake.yDirection = 0;
  }

  xDirectionPrev = globalSnake.xDirection;
  yDirectionPrev = globalSnake.yDirection;

}

// Adds a new segment to the snake
void addSnakeSegment(int segmentXPosition, int segmentYPosition)
{

  // Reserve new memory space
  int* newXPosition = (int *)malloc(sizeof(int) * (globalSnake.size + 1));
  int* newYPosition = (int *)malloc(sizeof(int) * (globalSnake.size + 1));

  // Add segment
  *newXPosition = segmentXPosition;
  *newYPosition = segmentYPosition;

  // Write old positions into new address
  for (int i = 0; i < globalSnake.size; ++i)
  {
    *(newXPosition + i + 1) = *(globalSnake.xPosition + i);
    *(newYPosition + i + 1) = *(globalSnake.yPosition + i);
  }

  // Clear old memory space
  if (globalSnake.xPosition != nullptr)
  {
    free(globalSnake.xPosition);
  }

  if (globalSnake.yPosition != nullptr)
  {
    free(globalSnake.yPosition);
  }

  // Replace
  globalSnake.xPosition = newXPosition;
  globalSnake.yPosition = newYPosition;

  // Update size
  globalSnake.size += 1;

  // Update snake in array
  updateSnakeInDisplayMatrix();

}

// Turns on all led to represent the snake (true in the array)
void updateSnakeInDisplayMatrix()
{

  int size = globalSnake.size;

  for (int i = 0; i < size; ++i)
  {
    int xPosition = *(globalSnake.xPosition + i);
    int yPosition = *(globalSnake.yPosition + i);
    displayMatrix[yPosition][xPosition] = true;
  }

}

void updateSnakePosition()
{

  // Update tail position
  globalSnake.xPositionTailPrev = *(globalSnake.xPosition + (globalSnake.size - 1));
  globalSnake.yPositionTailPrev = *(globalSnake.yPosition + (globalSnake.size - 1));

  // Update snake body before head
  for (int i = globalSnake.size - 1; i > 0; --i)
  {
    int xPositionNext = *(globalSnake.xPosition + (i-1));
    int yPositionNext = *(globalSnake.yPosition + (i-1));
    *(globalSnake.xPosition + i) = xPositionNext;
    *(globalSnake.yPosition + i) = yPositionNext;
  }

  // Update head
  *(globalSnake.xPosition) = *(globalSnake.xPosition) + globalSnake.xDirection;
  *(globalSnake.yPosition) = *(globalSnake.yPosition) + globalSnake.yDirection;

}

bool checkAppleEaten()
{

  // Check if the head eats the apple
  if (*globalSnake.xPosition + globalSnake.xDirection == globalApple.xPosition && *globalSnake.yPosition + globalSnake.yDirection == globalApple.yPosition)
  {
    addSnakeSegment(globalApple.xPosition, globalApple.yPosition);
    globalApple.createApple = true;

    // Play sound
    tone(PIEZO_PIN, APPLE_EAT_SOUND_FREQUENCY, APPLE_EAT_SOUND_DURATION);

    return true;

  }

  return false;

}

void createApple()
{

  // Change seed
  srand(millis());

  bool applePlaced = false;

  while (applePlaced == false)
  {

    applePlaced = true;

    // Generate random coordinates
    int xPositionRandom = random() % 12;
    int yPositionRandom = random() % 8;

    // Check if this position is already occupied by the snake
    for (int i = 0; i < globalSnake.size; ++i)
    {
      
      int xPositionCurrent = *(globalSnake.xPosition + i);
      int yPositionCurrent = *(globalSnake.yPosition + i);

      // Position already occupied
      if (xPositionCurrent == xPositionRandom && yPositionCurrent == yPositionRandom)
      {
        applePlaced = false;
      }

    }

    // Update apple position if successful
    if (applePlaced == true)
    {
      globalApple.xPosition = xPositionRandom;
      globalApple.yPosition = yPositionRandom;
    }

  }

  globalApple.createApple = false;

  updateAppleInDisplayMatrix();

}

void initGame()
{
  clearDisplayMatrix();
  initGlobalSnake();
  initGlobalApple();
  startGame = false;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Input
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(LEFT_PIN, INPUT);
  pinMode(RIGHT_PIN, INPUT);

  // Matrix
  matrix.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

  // Start the game
  if (startGame == true)
  {
    initGame();
  }

  // Process input
  updateSnakeDirection();

  // Update screen
  if (millis() - currentTime >= UPDATE_INTERVAL)
  {

    // Update time
    currentTime = millis();

    // Check if lost
    if (processDeath() == true)
    {
      return;
    }

    // Clear matrix
    clearDisplayMatrix();

    // Check if won
    if (processWin() == true)
    {
      return;
    }

    // Check if apple eaten
    if (checkAppleEaten() == false)
    {
      // Update position if no apple eaten
      updateSnakePosition();
    }

    // Generate new apple if necessary
    if (globalApple.createApple == true)
    {
      createApple();
    }
    else
    {
      updateAppleInDisplayMatrix();
    }

    updateSnakeInDisplayMatrix();
    displayCurrentFrame();

  }

}
