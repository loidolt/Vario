#include <MenuBackend.h>

/* This is the structure of the menu

    Settings
      Calibrate
      Switch Mode
          Variometer
          Altimeter
      Variometer Config
          Mode
          Display
      System Settings
          Volume
          Time
      Games
*/

//this controls the menu backend and the event generation
    MenuBackend menu = MenuBackend(menuUseEvent,menuChangeEvent);
      //beneath is a list of the menu items needed to build the menu
      MenuItem settings = MenuItem("Settings");
      
        MenuItem calBaro = MenuItem("Calibrate Barometer");
        MenuItem mode = MenuItem("Change Mode");
          MenuItem vario = MenuItem("Variometer");
          MenuItem alti = MenuItem("Altimeter");
          MenuItem modeBack = MenuItem("Back");
        
        MenuItem varioConfig = MenuItem("Configure Variometer");
        
          MenuItem varioMode = MenuItem("Sensitivity");
            MenuItem low = MenuItem("Low sensitivity");
            MenuItem medium = MenuItem("Medium sensitivity");
            MenuItem high = MenuItem("High sensitivity");
            MenuItem varioModeBack = MenuItem("Back");
          
          MenuItem varioDisplay = MenuItem("Display Settings");
            MenuItem largeRate = MenuItem("Large rate, small altitude");
            MenuItem largeAlti = MenuItem("Large altitude, small rate");
            MenuItem varioDispBack = MenuItem("Back");
          
          MenuItem varioBack = MenuItem("Back");
        
        MenuItem sysSettings = MenuItem("System Settings");
      
          MenuItem volume = MenuItem("Volume");
            MenuItem volumeLow = MenuItem("Low volume");
            MenuItem volumeMed = MenuItem("Medium volume");
            MenuItem volumeHigh = MenuItem("High volume");
            
            MenuItem volumeBack = MenuItem("Back");
          
          MenuItem setTime = MenuItem("Set Time");
        
          MenuItem sysBack = MenuItem("Back");
        
        MenuItem games = MenuItem("Games");
          MenuItem breakout = MenuItem("Breakout");
          MenuItem gamesBack = MenuItem("Back");
        
      MenuItem back = MenuItem("Back");
          
//this function builds the menu and connects the correct items together
void menuSetup(){
  
    Serial.println("Setting up menu...");
    //add the file menu to the menu root
    menu.getRoot().add(settings);
    settings.addRight(calBaro);
    
      calBaro.addBefore(back);
      
      calBaro.addAfter(mode);
    
      //setup the mode menu item
      mode.addRight(vario);
      
        //we want looping both up and down
        vario.addBefore(modeBack);
        vario.addAfter(alti);
        alti.addAfter(back);
        modeBack.addAfter(vario);
        //back button goes back
        modeBack.addRight(settings);
        
      mode.addAfter(varioConfig);
    
        varioConfig.addRight(varioMode);
          varioMode.addBefore(varioBack);
          varioMode.addAfter(varioDisplay);
          varioDisplay.addAfter(varioBack);
        
            varioMode.addRight(low);
              low.addBefore(varioModeBack);
              low.addAfter(medium);
              medium.addAfter(high);
              high.addAfter(varioModeBack);
            
              varioModeBack.addRight(varioMode);
            
            varioDisplay.addRight(largeRate);
              largeRate.addBefore(varioDispBack);
              largeRate.addAfter(largeAlti);
              largeAlti.addAfter(varioDispBack);
            
              varioDispBack.addRight(varioMode);
            
            varioBack.addRight(settings);
  
    varioConfig.addAfter(sysSettings);
    
      sysSettings.addRight(volume);
        volume.addBefore(sysBack);
        volume.addAfter(setTime);
        setTime.addAfter(sysBack);
        
        sysBack.addRight(settings);
        
        volume.addRight(volumeLow);
          volumeLow.addBefore(volumeBack);
          volumeLow.addAfter(volumeMed);
          volumeMed.addAfter(volumeHigh);
          volumeHigh.addAfter(volumeBack);
          
          volumeBack.addAfter(sysSettings);
    
    sysSettings.addAfter(games);
      
      games.addRight(breakout);
        breakout.addBefore(gamesBack);
        breakout.addAfter(gamesBack);
        
        gamesBack.addRight(settings);
        
  games.addAfter(back);
  
  back.addRight(settings);
}

/*
    This is an important function
    Here all use events are handled
    
    This is where you define a behavior for a menu item
*/
void menuUseEvent(MenuUseEvent used){
  
  Serial.print("Menu use ");
  Serial.println(used.item.getName());
  }

/*
    This is an important function
    Here we get a notification whenever the user changes the menu
    That is, when the menu is navigated
*/
void menuChangeEvent(MenuChangeEvent changed){
  
  Serial.print("Menu Change");
  Serial.print(changed.from.getName());
  Serial.print(" ");
  Serial.println(changed.to.getName());
}

void setup(){
  
  Serial.begin(9600);

  menuSetup();
  Serial.println("Starting navigation:\r\nUp: a  Down: s  Enter: d");
}

void loop(){
  
  if(Serial.available()) {
    byte read = Serial.read();
    switch (read) {
      case 'a': menu.moveUp(); break;
      case 's': menu.moveDown(); break;
      case 'd': menu.moveRight(); break;
    }
  }
}
