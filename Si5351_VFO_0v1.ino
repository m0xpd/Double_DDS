/*
Si5351_VFo_0v1
 
Simple RF Generation System 
with VFO and BFO 
implemented in a Si5351

CLK0 is the BFO, CLK1 is the VFO

With support for 16*4 or 20*4 LCD
(change the line "int lcdWidth=16;" to "int lcdWidth=20;")

m0xpd
Oct 2014
 
All parts available from Kanga-UK:
http://www.kanga-products.co.uk/
For further information:
http://m0xpd.blogspot.co.uk/
=======================================================

LIBRARY Includes...
You MUST have installed the correct LCD library AND
Jason Milldrum, nt7s' Si5351 library, which is available here:
https://github.com/etherkit/Si5351Arduino
*/
 
#include<stdlib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "si5351.h" 

/*=====================================================
Frequency defaults...
 
The system is set up to use the 
40m, 30m, 20m, 17m, 15m amateur bands 
 
You can edit the band edges and the default "start" 
frequency in any band by changing the following lines... */
 
double BandBases[]={
  3500000, 7000000, 10100000, 14000000, 18068000, 21000000};
double BandTops[]={
  3800000, 7200000, 10150000, 14350000, 18168000, 21450000};
double BandCWFreqs[]={
  3690000, 7090000, 10106000, 14285000, 18130000, 21285000};  
  
/*=====================================================
Modes and Offsets...
 
The system offers the following "modes"...
LSB, USB, which change either side of 10MHz
each of which has its own receive offset
 
You can adjust the receive offset continuously 
(to achieve "Receive Incremental Tuning" or "Clarifier" operation)
and you can change the offset defaults by editing the lines below... */
 
int ModeOffsets[]={
  0, 0};
/*=====================================================
Intermediate Frequency Settings...

The system implements BFO settings for lower and upper sideband
The BFO is changed when the band changes (if required)
*/
double BFOs[]={
  10000400, 10002400};  // set these for YOUR IF !
 
 
/*======================================================================
Defining CONSTANTS...
*/
//======================================
// HARDWARE: Arduino I/O Pin allocations..
// line used for AF sidetone generation
const int DATA = 8;
// input from rig to signal Tx/Rx status
const int TxPin = 12;
// Rotary Encoder...
const int RotEncAPin = 2;
const int RotEncBPin = 3;
const int RotEncSwPin = A3;
// Pushbuttons...
const int modeSw1 = 4;
const int modeSw3 = 5;

// Display...
// the display & Si5313 uses the I2C connection, 
// which (one the UNO, NANO etc) uses 
// A4 for the Data and
// A5 for the Clock
// (the MEGA has different I2C lines)
int lcdWidth=16;
int L2=17;
//======================================
// SOFTWARE: defining other constants...

/* MENU STRUCTURE....

MENU 0: RiT        Scalar
MENU 1: VFO        A or B
MENU 2: SideBand   USB or LSB
MENU 3: Band       80, 40, 30, 20, 17, 15
*/

const int nMenus = 0x03;
byte nMenuOptions[] = {
  1, 1, 1, 5};
char* MenuString[4][12]= {
  {
    "RiT:  ", "      " }
  ,
  {
    "VFO A ", "VFO B  " }
  ,
  {
    "LSB   ", "USB    "}
  ,
  {
    "  80m","  40m", "  30m", "  20m", "  17m", "  15m"}
 
};
 


// scaling factors for freq adjustment
const long deltaf[] = {
  1, 10, 100, 1000, 10000, 100000, 0, 1000000};
// Marketing !
char* Banner="      m0xpd     ";
 
// end of Constants
//=================================================================
 
/*=================================================================
Declare (and initialize) some VARIABLES...
*/
boolean MenuMode = false;
boolean Transmit=false;
char* TxChar;
int OffsetSign=0; 
char OffsetChar='=';
char VFOChar='A';
byte MenuOption[] = {
  0, 0, 0, 1, 0, 0};
byte oldVFO=MenuOption[1];
int RiT=ModeOffsets[MenuOption[2]];
double freq = BandCWFreqs[MenuOption[3]];
double freqA=freq;
double freqB=freq;
double BFO;
int dfindex = 3;
// Declare and initialise some Rotary Encoder Variables...
boolean OldRotEncA = true;
boolean RotEncA = true;
boolean RotEncB = true;
boolean RotEncSw = true;
 
boolean modeSw1State = HIGH;
boolean modeSw3State = HIGH;
boolean TxPinState = HIGH;

// Display positions...
byte ModePos=16;
byte TxPos=20;
byte RiTSignPos=23;
byte VFOPos=26;
byte BandPos=28;
int BannerPos=0;
// some temporary cursor variables for LCD management...
int c1=0;
int c2=0;
int c3=0; 

int Menu = 0;
 
// end of Variables
//=================================================================
 
 
// Instantiate the LCD display...
LiquidCrystal_I2C lcd(0x20,32,2); 
// (I2C address = 0x20)
// (I can't make it work as a 16*4 display -
// only as a 32*2 display !! )

Si5351 si5351; 

void setup() {
  lcd.init(); 
  delay(200);
  lcd.init();  
  lcd.blink();

// manage LCD width:
  if (lcdWidth==20){
     lcd.backlight();
     ModePos=20;
     TxPos=25;
     RiTSignPos=30;
     VFOPos=34;
     BandPos=35;
     L2=23; 
     BannerPos=2;    
  } 

// Set up I/O pins...  
  pinMode(modeSw1,INPUT);
  pinMode(modeSw3,INPUT);  
  pinMode(TxPin,INPUT);  
  pinMode(RotEncAPin, INPUT);
  pinMode(RotEncBPin, INPUT);
  pinMode(RotEncSwPin, INPUT);
  // set up pull-up resistors on inputs...  
  digitalWrite(modeSw1,HIGH);
  digitalWrite(modeSw3,HIGH); 
  //digitalWrite(TxPin,HIGH); 
  digitalWrite(RotEncAPin,HIGH);
  digitalWrite(RotEncBPin,HIGH);
  digitalWrite(RotEncSwPin,HIGH); 
  // Print opening message to the LCD...
  Normal_Display();
  si5351.init(SI5351_CRYSTAL_LOAD_8PF);  
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
    Serial.begin(9600);
  // start the oscillator...
  sendFrequency(freq);    
  BFO=BFOs[MenuOption[2]];
  sendbfo(BFO);
     Serial.println(BFO);   
}
 
void loop() {
  // First, we check for Transmit...
   TxPinState = digitalRead(TxPin);
  if (TxPinState==0){
    if  (Transmit==false){
      Transmit=true;
      setTxDisplay();
    }
  }
  else{
  // No :- we're in "Receive" 
   if (Transmit==true){
    Transmit=false;
    setTxDisplay();
  }
  
  }
  // Read the inputs...
  RotEncA = digitalRead(RotEncAPin);
  RotEncB = digitalRead(RotEncBPin); 
  RotEncSw = digitalRead(RotEncSwPin);
  modeSw1State=digitalRead(modeSw1);
  modeSw3State=digitalRead(modeSw3);
  if (MenuMode){
    // Here we're in "Menu" mode...
    if (Menu==0){
      // We're in Menu 0, so read the RiT...
      if ((RotEncA == HIGH)&&(OldRotEncA == LOW)){
        if (RotEncB == LOW) {
          RiT=constrain(RiT+1,-100000,10000);
        }
        else {
          RiT=constrain(RiT-1,-100000,10000);
        }
    LCD_Display_RiT(RiT);    
    sendFrequency(freq); 
    if (RiT>ModeOffsets[MenuOption[2]]) {
    OffsetSign=1;
    } 
    else{
    if (RiT==ModeOffsets[MenuOption[2]]){
    OffsetSign=0;
    }
    else{
    OffsetSign=-1;
    }
    } 
    } 
   
      OldRotEncA=RotEncA;    
    } 
    // we're not in menu 0, so manage the menu options 
    // for all other menus...
    if ((RotEncA == HIGH)&&(OldRotEncA == LOW)){
      if (RotEncB == LOW) {
        MenuOption[Menu]=constrain(MenuOption[Menu]+1,0,nMenuOptions[Menu]); 
        LCD_String_Display(7, 0, (MenuString[Menu][MenuOption[Menu]]));
      }
      else {
 
        MenuOption[Menu]=constrain(MenuOption[Menu]-1,0,nMenuOptions[Menu]);
        LCD_String_Display(7, 0, (MenuString[Menu][MenuOption[Menu]]));
      }   
    } 
    OldRotEncA=RotEncA;
 
    if (modeSw1State==LOW){
      // decrement Menu
      Menu=constrain(Menu-1,0,nMenus);
      if(Menu==2){
        // skip the mode change for the moment
        Menu = 1;
      }     
      LCD_Int_Display(5, 0, Menu);
      LCD_String_Display(7, 0, (MenuString[Menu][MenuOption[Menu]]));
      delay(500);
    } 
 
    if (modeSw3State==LOW){
      // Increment Menu
      Menu=constrain(Menu+1,0,nMenus);
       if(Menu==2){
        // skip the mode change for the moment
        Menu = 3;
      }      
      LCD_Int_Display(5, 0, Menu); 
      LCD_String_Display(7, 0, (MenuString[Menu][MenuOption[Menu]]));     
      delay(500);
 
    }   
  } 
// End of MenuMode==1
// ========================================================================
// Beginning of normal mode (MenuMode==0)...
else{
  if ((RotEncA == HIGH)&&(OldRotEncA == LOW)){
    // adjust frequency
    if (RotEncB == LOW) {
      freq=constrain(freq+deltaf[dfindex],BandBases[MenuOption[3]],BandTops[MenuOption[3]]);
    }
    else {
      freq=constrain(freq-deltaf[dfindex],BandBases[MenuOption[3]],BandTops[MenuOption[3]]);
    }
  
    LCD_Display_Freq(freq);
    sendFrequency(freq); 
    OldRotEncA=RotEncA;     
 
  }
      if (modeSw1State==LOW){
      // point to a higher digit
      dfindex=constrain(dfindex+1,0,7);
      LCD_Display_Freq(freq);
      delay(500);
 
    } 
 
    if (modeSw3State==LOW){
      // point to a lower digit
      dfindex=constrain(dfindex-1,0,7);
      LCD_Display_Freq(freq);       
      delay(500);
 
    }  
    // End of Normal Mode
}
 
  if (RotEncSw==LOW){
  // Toggle in and out of menu mode
    if (MenuMode == false){
      // enter menu mode
      MenuMode = true;
      LCD_String_Display(0, 0, "Menu         ");
      LCD_Int_Display(5, 0, Menu);
      LCD_String_Display(7, 0, (MenuString[Menu][MenuOption[Menu]]));
    }
    else{
      // leave menu mode
      MenuMode = false;
      //====================
      // specific actions on leaving menus...
 
  switch (Menu){
  // Leaving RiT adjustment
  case 0:
    switch (OffsetSign) {
      case 1:
      OffsetChar='+';
      break;
      case -1:
      OffsetChar='-';
      break;
      default: 
      OffsetChar='=';
    } 
 
  break;  
  // Leaving VFO swap
  case 1:
  if(oldVFO==0){
  freqA=freq;
  if (MenuOption[1]==1){
  freq=freqB;
  } 
  }
  else{
  freqB=freq;
  if (MenuOption[1]==0){
  freq=freqA;
  }
  }
  oldVFO=MenuOption[1];
  break;
  case 2:
  // leaving mode change
  RiT=ModeOffsets[MenuOption[2]];
  OffsetChar='=';
  break;
  case 3:
  // Leaving Band Change
    freq=BandCWFreqs[MenuOption[3]];
    freqA=freq;
    freqB=freq;
    MenuOption[1]=0;
    if(MenuOption[3]>1){
    //put into USB mode...
    MenuOption[2]=1;
    }
    else{
   // put into LSB
    MenuOption[2]=0;
    }
  }
    //====================
    // 
    sendFrequency(freq);
    Normal_Display();
      BFO=BFOs[MenuOption[2]];
  sendbfo(BFO);
     Serial.println(BFO); 
    } 
  
    delay(500);
// End of Toggle In and Out of Menu Mode
  }    
    OldRotEncA=RotEncA; 

// End of loop()
}
 
 
//==============================================================
// SUBROUTINES...
 
// subroutine to display the frequency...
void LCD_Display_Freq(double frequency) {  
  lcd.setCursor(L2, 0);// was   lcd.setCursor(18, 0);
  if (frequency<10000000){
  lcd.print("  ");
  lcd.print(frequency,0);
  lcd.print(" MHz");
  lcd.setCursor(L2+1,0);
  lcd.print(frequency/1e6,0);  
  lcd.print(".");
  }  
  else{
  lcd.setCursor(L2+1,0);    
  lcd.print(frequency,0);  
  lcd.print(" MHz");
  lcd.setCursor(L2,0);
  lcd.print(frequency/1e6,0);  
  lcd.print(".");  
  }
  // establish the cursor position
  int c_position=L2+8-dfindex;
  lcd.setCursor(c_position, 0); 
}   
 
// subroutine to display the RiT...
void LCD_Display_RiT(int RiT) {
  lcd.setCursor(5, 1);
  lcd.print(RiT,DEC);  
  lcd.print(" Hz   "); 
}  
 
// subroutine to clear the RiT display...
void LCD_Clear_RiT() {
  lcd.setCursor(0, 1);
  lcd.print("               "); 
}  
 
// subroutine to display a string at a specified position...
void LCD_String_Display(int c1, int c2, char Str[ ]){
      lcd.setCursor(c1, c2);
      lcd.print(Str);
}
// subroutine to display a number at a specified position...
void LCD_Int_Display(int c1, int c2, int num){
      lcd.setCursor(c1, c2);
      lcd.print(num);
}
// subroutine to display a character at a specified position...
void LCD_Char_Display(int c1, int c2, char Str){
      lcd.setCursor(c1, c2);
      lcd.print(Str);
}
 
 
// subroutine to set up the normal display...
void Normal_Display(){
      LCD_String_Display(ModePos, 1, MenuString[2][MenuOption[2]]);
      LCD_String_Display(BandPos, 1, MenuString[3][MenuOption[3]]);     
      switch (MenuOption[1]) {
        case 0:
          VFOChar='A';
        break;
        case 1:
          VFOChar='B';
        break;
        default: 
        VFOChar=' ';    
       }  
     LCD_Char_Display(VFOPos, 1, VFOChar);      
     LCD_Clear_RiT(); 
     LCD_String_Display(BannerPos, 0, Banner);     
     setTxDisplay();
     LCD_Char_Display(RiTSignPos, 1, OffsetChar);
     LCD_Display_Freq(freq);
}
 
// Subrouting to display the Transmit Status
void setTxDisplay(){
      switch (Transmit) {
        case false:
        TxChar="Rx";
        break;
        case true:
        TxChar="Tx";
        break; 
       }  
      LCD_String_Display(TxPos, 1,TxChar); 
      // re-establish the cursor position
      int c_position=25-dfindex;
      lcd.setCursor(c_position, 0);
}
 

 
// calculate and send frequency code to Si5351...
// CLK1 is the VFO
void sendFrequency(double frequency) {
  double freq1=0;
  if(MenuOption[2]==0){
  freq1=BFOs[MenuOption[2]]-frequency;  
  }
  else{
  freq1=frequency-BFOs[MenuOption[2]];
  }
  if(Transmit==false){
  freq1=freq1-RiT;
  }
  si5351.set_freq(freq1, 0, SI5351_CLK1);
}

void sendbfo(double frequency) {
  // CLK0 is the BFO
  si5351.set_freq(frequency, SI5351_PLL_FIXED, SI5351_CLK0);   
}

