const int upButton = 3;
const int enterButton = 2;
const int downButton = 4;

void setup (){
  
  Serial.begin(9600);
  
  Serial.println("hi");
  
  pinMode(upButton, INPUT_PULLUP);
  pinMode(enterButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  
}

void loop (){
  
  if (digitalRead(upButton) == LOW){
    Serial.println("up");
    delay(1);
  }
  else if (digitalRead(downButton) == LOW){
    Serial.println("down");
    delay(1);
  }
  else if (digitalRead(enterButton) == LOW){
    Serial.println("enter");
    delay(1);
  }
  }
