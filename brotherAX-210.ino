#include <Keypad.h>
#include "Keyboard.h"

#include <rtttl.h>

const char song_P[] PROGMEM = "PacMan:b=140:32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32c6,32p,32c7,32p,32g6,32p,32e6,32p,32c7,32g6,16p,16e6,16p,32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32d#6,32e6,32f6,32p,32f6,32f#6,32g6,32p,32g6,32g#6,32a6,32p,32b.6";

const byte ROWS = 8; //four rows
const byte COLS = 8; //three columns

byte rowPins[ROWS] = {2, 3, 4, 5, 6, 7, 8, 9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {10, 11, A0, A1, A2, A3, A4, A5 }; //connect to the column pinouts of the keypad
byte spk = 12;
byte led = 13;

byte ledPWMVal = 255; // full turned on

char keyShim[ROWS][COLS] = {
  { 0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 },
  { 8, 9 ,10,11,12,13,14,15 },
  { 16,17,18,19,20,21,22,23 },
  { 24,25,26,27,28,29,30,31 },
  { 32,33,34,35,36,37,38,39 },
  { 40,41,42,43,44,45,46,47 }, 
  { 48,49,50,51,52,53,54,55 },
  { 56,57,58,59,60,61,62,63 }, 
};

static char keys[ROWS][COLS] = { // mapped US keys to DE keys thru scancode comparing
  { 44, 46, '[', ';', ']', 39, 32,  KEY_LEFT_SHIFT },                      //  , . ü ö + ä SPACE SHIFT 
  { 47, 92, 113, 122, 119, 97, KEY_RETURN, KEY_CAPS_LOCK },                //  - # q y w a ENTER CAPS
  { 49, 50, 101, 102, 114, 103, KEY_DOWN_ARROW, KEY_LEFT_ALT },            //  1 2 e f r g CRS-UD, ALT
  { 51, 52, 116, 104, 121, 106, KEY_RIGHT_ARROW, 249 },                    //  3 4 t h z j CRS-LR ?
  { 55, 56, 111, 115, 112, 100, KEY_RIGHT_GUI,  KEY_TAB },                 //  7 8 o s p d ? TAB?nano 
  { 53, 54, 117, 107, 105, 108, KEY_BACKSPACE, KEY_RIGHT_GUI },            //  5 6 u k i l BACKSPACE WIN(WORT)
  { 45, 61, 110, 120, 109, 244, 0, KEY_BACKSPACE },                       //  ß ´ n x m PAPERFEEDLEVELER ALTGR BACKSPACE?
  { 57, 48, 118,  99,  98, KEY_TAB, KEY_LEFT_CTRL, 0 },        //  9 0 v c b TAB STRG(CODE) ALTGR
};

static unsigned int notekeys[128] = { // mapping for tone play
  // TODO classic ARP if multiple keys pressed maybe plain with millis()?
  // keys for ARP speed DEC INC and maybe FIXED values ARP/LFO
  // 0-12-24-36-48 4 Octaves
};




Keypad kpd = Keypad( makeKeymap(keyShim), rowPins, colPins, ROWS, COLS );
ProgmemPlayer player(spk);


void setup() {
    player.setSong(song_P);
    assignNoteKeys();
    Serial.begin(9600);
    Keyboard.begin();
    pinMode(led, OUTPUT);
    pinMode(spk, OUTPUT);
    digitalWrite(spk, HIGH);
    digitalWrite(led, HIGH);
    //player.finishSong();

}

boolean holdSHIFT = false;
boolean holdCTRL = false;
boolean holdWIN = false;
boolean holdALT = false;
boolean holdALTGR = false;

byte workMode = 0; // 0=Keyboard 1=playTones 2=playRingtones 3=toggleMode

static byte maxNotes = 2;
char activeNotes[3] = {-1,-1,-1};

unsigned int activeFreq[3] = {};
int activeNotesLength = 0;
int activeNotesLengthOld = 0;


/// SET KEY FOR NOTE MEMORY SHIFT+KEY=WRITE CAPS+KEY=DELETE
/// if another note is played stop arp until note is released
// CODE=SLOWER WORT=Faster // CRS-LR= nxtArp CRS-UD=prevArp

void checkKeyboard(){
    if (kpd.getKeys()) { // Fills kpd.key[ ] array with up-to 10 active keys. Returns true if there are ANY active keys.
        for (int i=0; i<LIST_MAX; i++) {  // Scan the whole key list.
            if ( kpd.key[i].stateChanged ) {  // Only find keys that have changed state.
                char keyy;
                byte kk = (byte)kpd.key[i].kchar;
                byte c = kk / 8;
                byte r = kk-(c*8);
                keyy = keys[c][r];   

                switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                  case PRESSED:
                    if (kk==54){ holdALTGR = true; }

                  

                    if(workMode==0){ // keyboard mode

                      if(holdALTGR){ // use wasd keys + altGR = cursor keys
                        if(keyy=='w'){ Keyboard.press(KEY_UP_ARROW);}
                        else if(keyy=='s'){ Keyboard.press(KEY_DOWN_ARROW);}
                        else if(keyy=='a'){ Keyboard.press(KEY_LEFT_ARROW);}
                        else if(keyy=='d'){ Keyboard.press(KEY_RIGHT_ARROW);}
                        else if(keyy=='q'){ Keyboard.press(KEY_RIGHT_ALT);Keyboard.press(113);}
                        else if(keyy=='/'||keyy=='ß'){ Keyboard.press(KEY_RIGHT_ALT);Keyboard.press(92);} // for \ sign
                      } else {
                        Keyboard.press(keyy);
                      };
                    
                    } else if (workMode==1){ // play tones on keyboard
                      if(notekeys[kk]>0&&activeNotesLength<maxNotes){ // check if notekey is occupied
                        activeNotes[activeNotesLength] = kk;
                        activeNotesLength++;
                      }
                    }
    
                    break;
                  case HOLD:
                    break;
                  case RELEASED:

                    if(holdALTGR && keyy=='1'){ // turn on toneplayMode
                      Keyboard.releaseAll();
                      activeNotesLength = 0;
                      noTone(spk);
                      if(workMode!=0){ workMode = 0; } else { workMode = 1; }
                    } else if(holdALTGR && keyy=='2'){ // ringtone MODE
                      Keyboard.releaseAll();
                      if(workMode!=0){ workMode = 0; } else { workMode = 2; }
                    } else if(holdALTGR && keyy=='3'){ // toggleMode
                      Keyboard.releaseAll();
                      if(workMode!=0){ workMode = 0; } else { workMode = 3; }
                    }
                  
                    if (kk==54){ holdALTGR = false; }

                    if(workMode==0){ // keyboard mode
                      if(holdALTGR){
                        if(keyy=='w'){ Keyboard.release(KEY_UP_ARROW);}
                        else if(keyy=='s'){ Keyboard.release(KEY_DOWN_ARROW);}
                        else if(keyy=='a'){ Keyboard.release(KEY_LEFT_ARROW);}
                        else if(keyy=='d'){ Keyboard.release(KEY_RIGHT_ARROW);}
                        else if(keyy=='q'){ Keyboard.release(KEY_RIGHT_ALT);Keyboard.release(113);}
                        else if(keyy=='/'||keyy=='ß'){ Keyboard.release(KEY_RIGHT_ALT);Keyboard.release(92);} // for \ sign
                        else if(keyy=='t'){ // special keyboard menu
                          noTone(spk);
                        }
                      } else { Keyboard.release(keyy); };
                    } else if (workMode==1){ // play tones on keyboard
                      for(byte note=0; note<maxNotes; note++){// delete released KEYS
                        if(activeNotes[note]==kk && activeNotesLength>0){
                          activeNotes[note] = -1;
                          activeNotesLength--;
                        }
                      }
                    }
                    break;
                  case IDLE:
                    break;
                }
            }
        }
    }
    //if(holdKeys==0) activeNotesLength=0;
}



void loop() {
  if(Serial.available()>0){ // debug on key request
    while(Serial.available()>0){Serial.read();}
    
    Serial.print("activeNotesLength:");
    Serial.println(activeNotesLength);

      for(byte note=0; note<maxNotes; note++){
        Serial.print("noteNr:");
        Serial.print(note);
        Serial.print(" note:");
        Serial.println(activeNotes[note], DEC);
      }
    }
    
  

  checkKeyboard();
  if(activeNotesLength!=activeNotesLengthOld){
    activeNotesLengthOld = activeNotesLength;
    for(byte note=0; note<maxNotes; note++){
      byte activeFreqCnt=0;
      byte noteVal = activeNotes[note];
      if(noteVal>-1){
        activeFreq[activeFreqCnt] = noteVal;
        activeFreqCnt++;
      }
    }
  }
  if(activeNotesLength>0){
    tone(spk, activeFreq[0], 50);
    for(byte freqNr=0; freqNr<activeNotesLength; freqNr++){

    }
  } else { noTone(spk); }
}



void assignNoteKeys (){
    // mapping // OCTAVE-0 // "Y" - "ENTER"
  notekeys[11] = 65;
  notekeys[51] = 69;
  notekeys[59] = 73;
  notekeys[58] = 77;
  notekeys[60] = 82;
  notekeys[50] = 87;
  notekeys[52] = 92;
  notekeys[0] = 98;
  notekeys[1] = 104;
  notekeys[8] = 110;
  notekeys[7] = 116;
  notekeys[14] = 123;
  // mapping // OCTAVE-1 // "A" - "'"
  notekeys[13] = 33;
  notekeys[35] = 35;
  notekeys[37] = 37;
  notekeys[19] = 39;
  notekeys[21] = 41;
  notekeys[27] = 44;
  notekeys[29] = 46;
  notekeys[43] = 49;
  notekeys[45] = 52;
  notekeys[3] = 55;
  notekeys[5] = 58;
  notekeys[9] = 62;
  // mapping // OCTAVE-2 // "Q" - "+"
  notekeys[10] = 265;
  notekeys[12] = 365;
  notekeys[18] = 465;
  notekeys[20] = 565;
  notekeys[26] = 665;
  notekeys[28] = 765;
  notekeys[42] = 865;
  notekeys[44] = 965;
  notekeys[34] = 065;
  notekeys[36] = 65;
  notekeys[2] = 65;
  notekeys[4] = 65;
  // mapping // OCTAVE-3 // "1" - "´"
  notekeys[16] = 165;
  notekeys[17] = 265;
  notekeys[24] = 365;
  notekeys[25] = 465;
  notekeys[40] = 565;
  notekeys[41] = 665;
  notekeys[32] = 765;
  notekeys[33] = 865;
  notekeys[56] = 965;
  notekeys[57] = 65;
  notekeys[48] = 65;
  notekeys[49] = 65;
  // mapping // other keys for changing different stuff
  notekeys[0] = 65;

}
