// ==================================================================
// Proverbs 3:5, 1 Samuel 2:30
// ==================================================================
/* WARNING: Use code optimizing at your OWN discretion. */
// It isn't GUARANTEED to speed up your program.
/* CHEAT TO SPEED UP CODE 
#pragma GCC optimize ("-O3")
#pragma GCC push_options*/
// ==================================================================
// BC256 --Byte Code 256-- INSTRUCTIONS (1.0) "STABLE"
// ==================================================================
/* WARNING: PORT control and AREF control is only enabled for ATmega328P */
/* WARNING: EDIT won't restore your command if invalid command is put in */
// NOTE: code currently is stable.
// NOTE: 4 micros are consumed per instruction.
// 1/06/23: initial defines created for RAM and EEPROM control, a few minor flaws fixed.
// 2/06/23: PORT and AREF disabled on all but ATmega328P, error checking for wrong memory sizes. 
// 4/06/23: Millis and Micros implemented.
// 6/06/23: Millis and Micros bugs fixed successfully, optimizing code no longer speeds up execution?
//          Changed interpreter instruction PNMDR & PNMDB to be portable across different platforms.
// 7/06/23: Adjusted Autorun prevent to only be run when both pins are LOW.
//          Changed autorun_prog value to 90(00b01011010), to prevent unformatted EEPROM from autorunning.
//          Boolean NOT, AND, OR, added as a major realisation came with trying to shorten the "blink" program.
// 13/06/23: Added better formatting for outputting values, registers can be inputted and outputted as rA - rK.
//           print_input(now term_setup) changed to be 8 different boolean values set via bits, 
//           only two are in use(bit 7 & 0) for EZY2RD & CLI settings.
// 14/06/23: Removed MSINIT and USINIT, added additional error checking for unchecked register access.
// 19/06/23: Implemented Data spaces. Registers above 5(rF) can't be modified except for push/pop.
//           Note: you can't edit data spaces, but you can delete old ones and insert new ones.
// 21/06/23: Implemented Printing of a data space and Comparing of a data space to another. 
//           Error checking for MILLIS & MICROS overflowing stack. MSRFRSH & USRFRSH changed to MILLIS & MICROS.
// 22/06/23: PNMDD added. which is pinmode for all the numbers in the array to do multiple pinmodes 0-19.
//           Interpreter input format for ezy2rd displayed.

#include <EEPROM.h>

// Pins to use to skip the autorunning of saved program.
/* WARNING => ONLY TWO pins MUST be defined. <= WARNING */
#define prevent_auto1   12
#define prevent_auto2   13

// Set the size of memory. 
/* WARNING => EEPROM_size plus stack_size can't exceed your boards total SRAM space!  <= WARNING */
/* WARNING => stack_size MUST be at least 4 bytes! <= WARNING */
#define EEPROM_size     1024  // 1024 by default.
#define stack_size      256   // 256 by default.
/* WARNING: DON'T CHANGE DEFINES BELOW HERE!  */
#define ram_size        EEPROM_size + stack_size

// ram size subtract one for pop/push.
#define ram_sizes1       ram_size - 1

// Save program to EEPROM helpers.
#define saved_prog_lsb  EEPROM_size - 1
#define saved_prog_msb  EEPROM_size - 2
#define autorun_prog    EEPROM_size - 3

// Terminal output behaviour.
#define term_setup     EEPROM_size - 4

// Error checking for invalid memory values
/* WARNING: DON'T CHANGE ON ERROR, instead change EEPROM_size OR stack_size. */
#if stack_size < 4
  #error stack_size must be at least 4 bytes.
#elif EEPROM_size < 8
  #error EEPROM_size must be at least 8 bytes.
#endif

// Global variable declaration.
byte pRAM[ram_size];
byte bufidx = 0, bufindex = 0, instrargs = 0, hlp_B = 0, reg_cmp = 0;

char buf[16];
word addr = 0, code = 0;
unsigned long benchmark = 0;
bool hasregs = false, speedtest = false;

void setup() {  
  memset(buf, 0, 16);
  randomSeed(analogRead(6));
  Serial.begin(115200);

  hlp_B = EEPROM.read(term_setup);
  pinMode(prevent_auto1, INPUT_PULLUP);
  pinMode(prevent_auto2, INPUT_PULLUP);
  
  if (EEPROM.read(autorun_prog) == 90 and (digitalRead(prevent_auto1) or digitalRead(prevent_auto2))) {
    addr = word(EEPROM.read(saved_prog_msb), EEPROM.read(saved_prog_lsb)); 
    for (code = 0; code < addr; code++) {pRAM[code] = EEPROM.read(code);}
    BC256();
  }
  while (!digitalRead(prevent_auto1) and !digitalRead(prevent_auto2));
  pinMode(prevent_auto1, INPUT);
  pinMode(prevent_auto2, INPUT);
  
  Serial.println(F("Welcome to BC256 --Byte Code 256-- interactive console."));
  if (hlp_B & 0b10000000) {Serial.print(F("BC256 v1.0 EZY2RD-> "));} else {Serial.print(F("BC256 v1.0-> "));}
}

void loop() {
  get_input();
  
  if (String(buf) == F("AUTO")) {
    if (EEPROM.read(autorun_prog) == 90) {
      EEPROM.write(autorun_prog, 0);
      Serial.println(F("Auto load & run on."));
    } else {
      EEPROM.write(autorun_prog, 90);
      Serial.println(F("Auto load & run off."));
    }
  }
  else if (String(buf) == F("BENCH")) {
    speedtest = !speedtest; 
    if (speedtest) {Serial.println(F("Program speed timing on."));} else {Serial.println(F("Program speed timing off."));}
  }
  else if (String(buf) == F("BINDEC")) {format_conv(2);}
  else if (String(buf) == F("BLIST")) {list_program(true);}
  else if (String(buf) == F("CLEAR")) {for (code = 0; code < 1024; code++) {EEPROM.update(code, 0);} Serial.println(F("EEPROM erase completed."));}
  else if (String(buf) == F("CLI")) {
    hlp_B = EEPROM.read(term_setup);
    if (hlp_B & 0b00000001) {
      EEPROM.update(term_setup, (hlp_B & 0b11111110));
      Serial.println(F("Input display off.")); 
    } else {
      EEPROM.update(term_setup, (hlp_B | 0b00000001));
      Serial.println(F("Input display on."));
    }
    hlp_B = EEPROM.read(term_setup);
  }
  else if (String(buf) == F("DECBIN")) {format_conv(3);}
  else if (String(buf) == F("DECHEX")) {format_conv(1);}
  else if (String(buf) == F("DEL")) {hasregs = false; delete_add();}
  else if (String(buf) == F("EDIT")) {edit();}
  else if (String(buf) == F("EZY2RD")) {
    hlp_B = EEPROM.read(term_setup);
    if (hlp_B & 0b10000000) {
      EEPROM.update(term_setup, (hlp_B & 0b01111111));
      Serial.println(F("Easy to read register format off.")); 
    } else {
      EEPROM.update(term_setup, (hlp_B | 0b10000000));
      Serial.println(F("Easy to read register format on."));
    }
    hlp_B = EEPROM.read(term_setup);
  }
  else if (String(buf) == F("INSERT")) {hasregs = true; delete_add();}
  else if (String(buf) == F("HELP")) {
    Serial.println(F("auto   - Set EEPROM to automatically load and run program on startup."));
    Serial.println(F("bench  - Displays the time the program took to complete."));
    Serial.println(F("bindec - Converts a number from binary format to decimal. Number MUST be binary format."));
    Serial.println(F("blist  - Display the program totally in hex format. Use 'list' command for better readability."));
    Serial.println(F("clear  - Erase EEPROM."));
    Serial.println(F("cli    - For use with terminal apps. Doesn't display input after entering."));
    Serial.println(F("decbin - Converts a number from decimal format to binary."));
    Serial.println(F("dechex - Converts a number from decimal format to hex."));
    Serial.println(F("del    - Delete an instruction. Address number MUST be hex format."));
    Serial.println(F("edit   - Edit an instruction. Address number MUST be hex format."));
    Serial.println(F("ezy2rd - Registers are displayed as alphabetical letters instead of numbers."));
    Serial.println(F("insert - Adds a new instruction. Address number MUST be hex format."));
    Serial.println(F("help   - Show this help."));
    Serial.println(F("hexdec - Converts a number from hex format to decimal. Number MUST be hex format."));
    Serial.println(F("list   - Display the program in the memory. Program is diplayed in hex format."));
    Serial.println(F("load   - Load the program from EEPROM into memory."));
    Serial.println(F("mem    - Show free memory left."));
    Serial.println(F("memin  - Put a number to memory address. Address number MUST be hex format."));
    Serial.println(F("meminc - Put a char to memory address. Address number MUST be hex format."));
    Serial.println(F("memout - Get a number from memory address. Address number MUST be hex format."));
    Serial.println(F("new    - Clear memory for new program."));
    Serial.println(F("run    - Run the program in memory."));
    Serial.println(F("save   - Save the program from memory into EEPROM."));
    Serial.println(F(""));
    Serial.println(F("When entering instructions, numbers use decimal format unless otherwise specified."));
  }
  else if (String(buf) == F("HEXDEC")) {format_conv(0);}
  else if (String(buf) == F("LIST")) {list_program(false);}
  else if (String(buf) == F("LOAD")) {
    addr = word(EEPROM.read(saved_prog_msb), EEPROM.read(saved_prog_lsb));
    if (addr > 0) {
      for (code = 0; code < addr; code++) {pRAM[code] = EEPROM.read(code);} 
      Serial.println(F("Load done."));
    } else {Serial.println(F("No program in EEPROM to load."));}
  }
  else if (String(buf) == F("MEM")) {Serial.print("Memory free " + String(EEPROM_size-addr)); Serial.print(F("/")); Serial.print(EEPROM_size); Serial.println(F(" bytes."));}
  else if (String(buf) == F("MEMIN")) {memwrite(0);}
  else if (String(buf) == F("MEMINC")) {memwrite(2);}
  else if (String(buf) == F("MEMOUT")) {memwrite(1);}
  else if (String(buf) == F("NEW")) {memset(pRAM, 0, sizeof(pRAM)); addr = 0; Serial.println("Memory clear done.");}
  else if (String(buf) == F("RUN")) {
    if (speedtest) {
      benchmark = micros();
      BC256(); 
      benchmark = micros() - benchmark;
      Serial.print(F("Completed program in "));
      Serial.print(benchmark);
      Serial.println(F(" microseconds."));
    } else {
      BC256(); 
      Serial.println(F("Program done."));
    }
  }
  else if (String(buf) == F("SAVE")) {
    if (addr <= 1024) {
      for (code = 0; code < addr; code++) {EEPROM.update(code, pRAM[code]);} 
      EEPROM.update(saved_prog_msb, addr>>8); 
      EEPROM.update(saved_prog_lsb, addr&0xFF);
      Serial.println(F("Save done."));
    } else {Serial.println(F("Program in memory to big for EEPROM."));}
  }
  else if (bufidx > 0) {process_input();}
  if (hlp_B & 0b10000000) {Serial.print(F("BC256 v1.0 EZY2RD-> "));} else {Serial.print(F("BC256 v1.0-> "));}
  bufidx = 0;
}

// ==================================================================
// BC256 --Byte Code 256-- INTERACTIVE (1.0) "STABLE"
// ==================================================================
const char instr_0[] PROGMEM = "STOP";
const char instr_1[] PROGMEM = "MOVR"; // => reg=reg
const char instr_2[] PROGMEM = "MOVB"; // => reg=byte
const char instr_3[] PROGMEM = "PRINT"; // => Serial.println(reg)
const char instr_4[] PROGMEM = "ADDR"; // => reg+=reg
const char instr_5[] PROGMEM = "ADDB"; // => reg+=byte
const char instr_6[] PROGMEM = "SUBR"; // => reg-=reg
const char instr_7[] PROGMEM = "SUBB"; // => reg-=byte
const char instr_8[] PROGMEM = "MULR"; // => reg*=reg
const char instr_9[] PROGMEM = "MULB"; // => reg*=byte
const char instr_10[] PROGMEM = "DIVR"; // => reg/=byte
const char instr_11[] PROGMEM = "DIVB"; // => reg/=reg
const char instr_12[] PROGMEM = "MODR"; // => reg%=byte
const char instr_13[] PROGMEM = "MODB"; // => reg%=reg
const char instr_14[] PROGMEM = "SWAP"; // => reg<->reg
const char instr_15[] PROGMEM = "BEGR"; // => loop reg
const char instr_16[] PROGMEM = "BEGB"; // => loop byte
const char instr_17[] PROGMEM = "LOOP"; // => loop end
const char instr_18[] PROGMEM = "PUSH"; // => mem=reg
const char instr_19[] PROGMEM = "POP"; // => reg=mem
const char instr_20[] PROGMEM = "PSHALL"; // => mem=all regs
const char instr_21[] PROGMEM = "POPALL"; // => all regs=mem
const char instr_22[] PROGMEM = "JMPE"; // => jump to address if =
const char instr_23[] PROGMEM = "JMPL"; // => jump to address if <
const char instr_24[] PROGMEM = "JMPG"; // => jump to address if >
const char instr_25[] PROGMEM = "JMPLE"; // => jump to address if <=
const char instr_26[] PROGMEM = "JMPGE"; // => jump to address if >=
const char instr_27[] PROGMEM = "JMPNE"; // => jump to address if !=
const char instr_28[] PROGMEM = "INCR"; // => reg++
const char instr_29[] PROGMEM = "DECR"; // => reg--
const char instr_30[] PROGMEM = "HALTR"; // => pause reg milliseconds
const char instr_31[] PROGMEM = "HALTB"; // => pause byte milliseconds
const char instr_32[] PROGMEM = "HLTMR"; // => pause reg microseconds
const char instr_33[] PROGMEM = "HLTMB"; // => pause reg microseconds
const char instr_34[] PROGMEM = "OUT"; // => OUT = byte, byte
const char instr_35[] PROGMEM = "OUTR"; // => OUTR = reg, reg
const char instr_36[] PROGMEM = "OUTB"; // => OUTB = reg, byte
const char instr_37[] PROGMEM = "INR"; // => INR = reg
const char instr_38[] PROGMEM = "INB"; // => INB = byte
const char instr_39[] PROGMEM = "DGTLOR"; // => Digital out reg
const char instr_40[] PROGMEM = "DGTLOB"; // => Digital out byte
const char instr_41[] PROGMEM = "DGTLIN"; // => reg = Digital in
const char instr_42[] PROGMEM = "PNMDR"; // => Pinmode reg
const char instr_43[] PROGMEM = "PNMDB"; // => Pinmode byte
const char instr_44[] PROGMEM = "NOP"; // => No OPerand
const char instr_45[] PROGMEM = "PUT"; // => put address
const char instr_46[] PROGMEM = "GET"; // => get address
const char instr_47[] PROGMEM = "OUTBR"; // => OUTBR = byte, reg
const char instr_48[] PROGMEM = "AREF"; // => AREF byte
const char instr_49[] PROGMEM = "ANLGOR"; // => Analog out reg
const char instr_50[] PROGMEM = "ANLGOB"; // => Analog out byte
const char instr_51[] PROGMEM = "ANLGIN"; // => reg = Analog in
const char instr_52[] PROGMEM = "RSHFTR"; // => reg>>=reg
const char instr_53[] PROGMEM = "RSHFTB"; // => reg>>=byte
const char instr_54[] PROGMEM = "LSHFTR"; // => reg<<=reg
const char instr_55[] PROGMEM = "LSHFTB"; // => reg<<=byte
const char instr_56[] PROGMEM = "ANDR"; // => reg&=reg
const char instr_57[] PROGMEM = "ANDB"; // => reg&=byte
const char instr_58[] PROGMEM = "ORR"; // => reg|=reg
const char instr_59[] PROGMEM = "ORB"; // => reg|=byte
const char instr_60[] PROGMEM = "XORR"; // => reg^=reg
const char instr_61[] PROGMEM = "XORB"; // => reg^=byte
const char instr_62[] PROGMEM = "NOT"; // => reg=~reg
const char instr_63[] PROGMEM = "RNDR"; // => random reg, reg
const char instr_64[] PROGMEM = "RNDB"; // => random reg, byte
const char instr_65[] PROGMEM = "AVAL"; // => reg=Serial.available()
const char instr_66[] PROGMEM = "READ"; // => reg=Serial.read() read whether available or not
const char instr_67[] PROGMEM = "WREAD"; // => reg=Serial.read() wait till available then read
const char instr_68[] PROGMEM = "PRINTCR"; // => Serial.print(char(reg))
const char instr_69[] PROGMEM = "PRINTCB"; // => Serial.print(char(byte))
const char instr_70[] PROGMEM = "SERCTRL"; // => byte == 1 -> Serial.end() // byte == 0 -> Serial.begin(9600)
const char instr_71[] PROGMEM = "JMP"; // => jump to address unconditional. <= Bug fix.
const char instr_72[] PROGMEM = "CLEAN"; // => Clear serial buffer
const char instr_73[] PROGMEM = "SKPBH"; // => (reg & (1 << reg))  Skip next instruction if bit is high
const char instr_74[] PROGMEM = "SKPBL"; // => !(reg & (1 << reg))  Skip next instruction if bit is low
const char instr_75[] PROGMEM = "DATA"; // => Data area. bytes to reserve.
const char instr_76[] PROGMEM = "WIPE"; // => Wipe a specified data area.
const char instr_77[] PROGMEM = "MILLIS"; // => Get current millis and copy to "pointer" memory address.
const char instr_78[] PROGMEM = "MICROS"; // => Get current micros and copy to "pointer" memory address.
const char instr_79[] PROGMEM = "JMPMS"; // => jump to address if ((millis() - oldMillis) > value)
const char instr_80[] PROGMEM = "JMPUS"; // => jump to address if ((micros() - oldMicros) > value)
const char instr_81[] PROGMEM = "BORR"; // => Boolean OR, reg=reg||reg
const char instr_82[] PROGMEM = "BORB"; // => Boolean OR, reg=reg||byte
const char instr_83[] PROGMEM = "BANDR"; // => Boolean AND, reg=reg&&reg
const char instr_84[] PROGMEM = "BANDB"; // => Boolean AND, reg=reg&&byte
const char instr_85[] PROGMEM = "BNOT"; // => Boolean NOT, reg=!reg
const char instr_86[] PROGMEM = "PRINTDC"; // => Serial.print(char_buf) buffer is printed as characters reg c & d
const char instr_87[] PROGMEM = "PRINTDB"; // => Serial.print(byte_buf) buffer is printed as numbers reg c & d
const char instr_88[] PROGMEM = "JMPDE"; // => jump to address if data buffers are equal
const char instr_89[] PROGMEM = "JMPDNE"; // => jump to address if data buffers aren't equal
const char instr_90[] PROGMEM = "PNMDD"; // => Set data array size of pins to data specified output
const char* const instrlist[] PROGMEM = {
  instr_0,instr_1,instr_2,instr_3,instr_4,instr_5,instr_6,instr_7,instr_8,instr_9,instr_10,instr_11,instr_12,instr_13,instr_14,instr_15,instr_16,instr_17,instr_18,instr_19,instr_20,instr_21,instr_22,instr_23,instr_24,instr_25,instr_26,instr_27,instr_28,instr_29, instr_30,instr_31,instr_32,instr_33,instr_34,instr_35,instr_36,instr_37,instr_38,instr_39,instr_40,instr_41,instr_42,instr_43,instr_44,instr_45,instr_46,instr_47,instr_48,instr_49,instr_50,instr_51,instr_52,instr_53,instr_54,instr_55,instr_56,instr_57,instr_58,instr_59,instr_60,instr_61,instr_62,instr_63,instr_64,instr_65,instr_66,instr_67,instr_68,instr_69,instr_70,instr_71,instr_72,instr_73,instr_74,instr_75,instr_76,instr_77,instr_78,instr_79,instr_80,instr_81,instr_82,instr_83,instr_84,instr_85,instr_86,instr_87,instr_88,instr_89,instr_90
};
const byte params[] PROGMEM = {
//0                  10                  20                  30                  40                  50                  60                  70                  80                  90
//0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
  0,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,1,1,1,1,1,0,1,1,2,1,1,1,1,2,2,2,2,2,2,2,2,2,2,1,2,2,1,1,1,1,1,1,1,0,2,2,1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1
};
const byte regparams[] PROGMEM = {
//0                  10                  20                  30                  40                  50                  60                  70                  80                  90
//0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
  0,1,0,1,1,0,1,0,1,0,1,0,1,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,1,0,1,0,0,1,1,1,0,1,0,1,1,0,0,0,0,1,0,1,0,1,1,0,1,0,1,0,1,0,1,0,1,1,0,1,1,1,1,0,0,0,0,1,1,0,0,1,1,0,0,1,0,1,0,1,0,0,0,0,0
};

String getInstr(byte nb) {
  char cmpstr[8];
  strcpy_P(cmpstr, (char*)pgm_read_word(&(instrlist[nb])));
  return cmpstr;
}

void process_input() {
  char buf2[8];
  bool errors = false;
  code = 0; bufindex = 0; instrargs = 0; hasregs = false;
  
  while (buf[code] == ' ') {code++;}
  while (buf[code+(bufindex++)] != ' ') {if (buf[code+(bufindex-1)] == 0) {errors = 1; break;}}
  if (errors == 1 and bufindex <= 1) {Serial.println(F("No instruction.")); return;}
  
  if (addr == EEPROM_size) {Serial.println(F("No available memory left.")); return;}

  memset(buf2, 0, 8);
  memcpy(buf2, buf+code, bufindex-1);
 
  code = 0;  
  while (String(buf2) != getInstr(code)) {
    code++; 
    if (code > 90) {Serial.println(String(buf2) + F(" Isn't a valid instruction.")); return;}
  }
  pRAM[addr++] = code;
  
  instrargs = pgm_read_byte_near(params+code);
  hasregs = pgm_read_byte_near(regparams+code);

  errors = 0;
  code = bufindex; bufindex = 0;
  while (buf[code] == ' ') {code++;}
  while (buf[code+bufindex] != ' ') {if (buf[code+bufindex] == 0) {errors = 1; break;} bufindex++;}

  if (instrargs == 1 and buf[code] != 0) {
    memset(buf2, 0, 8);
    memcpy(buf2, buf+code, bufindex);
    
    if (pRAM[addr-1] == 76 or pRAM[addr-1] == 71 or (pRAM[addr-1] >= 22 and pRAM[addr-1] <= 27) or (pRAM[addr-1] >= 45 and pRAM[addr-1] <= 46) or (pRAM[addr-1] >= 79 and pRAM[addr-1] <= 80) or (pRAM[addr-1] >= 86 and pRAM[addr-1] <= 90)) {pRAM[addr++] = atoi(buf2) >> 8;}
    if (hlp_B & 0b10000000 and hasregs) {
      if((buf2[1] - 65) > 10) {
        Serial.print(F("Invalid register "));
        Serial.print(buf2);
        Serial.println(F(", valid registers are rA to rF.")); 
        addr--; 
        return;
      }
      pRAM[addr++] = (buf2[1] - 65) & 0xFF;
    } else {
      pRAM[addr++] = atoi(buf2) & 0xFF;
    }
  }
  else if (instrargs == 2 and errors != 1) {
    memset(buf2, 0, 8);
    memcpy(buf2, buf+code, bufindex);

    if (hlp_B & 0b10000000 and (pRAM[addr-2] != 34 or pRAM[addr-2] != 47)) {
      if((buf2[1] - 65) > 5 or (buf2[1] - 65) < 0) {
        Serial.print(F("Invalid register "));
        Serial.print(buf2);
        Serial.println(F(", valid registers are rA to rF.")); 
        addr--; 
        return;
      }
      pRAM[addr++] = (buf2[1] - 65);
    } else {
      if(atoi(buf2) > 5 and (pRAM[addr-1] != 34 or pRAM[addr-1] != 47)) {
        Serial.print(F("Invalid number "));
        Serial.print(atoi(buf2));
        Serial.println(F(", valid numbers are 0 to 5.")); 
        addr--; 
        return;
      }
      pRAM[addr++] = atoi(buf2);
    }
    
    code += bufindex; bufindex = 0;
    while (buf[code] == ' ') {code++;}
    while (buf[code+bufindex] != 0) {bufindex++;};
    if (!bufindex) {Serial.println(F("Only one argument, two expected.")); addr-=2; return;}

    memset(buf2, 0, 8);
    memcpy(buf2, buf+code, bufindex);

    if (hlp_B & 0b10000000) {
      if((buf2[1] - 65) > 10 and hasregs) {
        Serial.print(F("Invalid register "));
        Serial.print(buf2);
        Serial.println(F(", valid registers are rA to rK.")); 
        addr-=2; 
        return;
      }
      pRAM[addr++] = (buf2[1] - 65);
    } else {
      if(pRAM[addr] > 10 and hasregs) {
        Serial.print(F("Invalid number "));
        Serial.print(atoi(buf2));
        Serial.println(F(", valid numbers are 0 to 10.")); 
        addr-=2; 
        return;
      }
      pRAM[addr++] = atoi(buf2) & 0xFF;
    }
    
  } else if (errors == 1 and instrargs > 0) {Serial.println(F("Not enough arguments preceding instruction.")); addr--;}
  if ((pRAM[addr-2] == 77 or pRAM[addr-2] == 78) and pRAM[addr-1]+4 > stack_size) {
    Serial.println(F("Not enough stack space available."));
    addr -= instrargs + 1;
  }
  if (pRAM[addr-2] == 75) {addr += pRAM[addr-1];}
  if (addr > EEPROM_size) {
    Serial.println(F("Memory overflow.")); 
    addr -= instrargs + 1;
    if(pRAM[addr-1] == 75) {addr -= pRAM[addr];}
    if(pRAM[addr-1] == 76 or pRAM[addr-1] == 71 or (pRAM[addr-1] >= 22 and pRAM[addr-1] <= 27) or (pRAM[addr-1] >= 45 and pRAM[addr-1] <= 46) or (pRAM[addr-1] >= 79 and pRAM[addr-1] <= 80) or (pRAM[addr-1] >= 86 and pRAM[addr-1] <= 90)) {addr--;}
  }
}

void delete_add() {
  word tmp_addr = 0, tmp_code = 0;
  if (addr == 0) {Serial.println(F("Nothing to modify.")); return;}
  
  Serial.print(F("Enter address number: "));
  get_input();
  code = (byte)strtol(buf, (char **)NULL, 16);

  if (code > addr) {Serial.println(F("Insert address can not be bigger then program address.")); return;}
  
  if (hasregs) {
    Serial.print(F("New instruction to insert: "));
    get_input();
    
    bufidx = code + 3;    
    memmove(pRAM+bufidx, pRAM+code, addr);
    pRAM[code] = 0; pRAM[code+1] = 0; pRAM[code+2] = 0;

    tmp_addr = addr;
    addr = code;
    tmp_code = code;
    
    process_input();
    
    code = tmp_addr - (bufidx - 3);
    if (pRAM[tmp_code] == 75 and code >= (tmp_addr - (addr - pRAM[tmp_code + 1]))) {
      memcpy(pRAM+addr, pRAM+bufidx, code);
      if (pRAM[tmp_code+1] > 1) {pRAM[code+1] = 0;}
      if (pRAM[tmp_code+1] > 2) {pRAM[code+2] = 0;}
    } else if (code >= (tmp_addr - addr)) {memcpy(pRAM+addr, pRAM+bufidx, code);}
    addr += code;
  } else {
    instrargs = pgm_read_byte_near(params+pRAM[code]);    
    if (pRAM[code] == 76 or pRAM[code] == 71 or (pRAM[code] >= 22 and pRAM[code] <= 27) or (pRAM[code] >= 45 and pRAM[code] <= 46) or (pRAM[code] >= 79 and pRAM[code] <= 80) or (pRAM[code] >= 86 and pRAM[code] <= 90)) {instrargs++;}
    if (pRAM[code] == 75) {instrargs += pRAM[code+1];}
    
    bufindex = (code + instrargs) + 1;    
    memcpy(pRAM+code, pRAM+bufindex, addr);
    addr -= (instrargs + 1);
  }
}

void list_program(boolean instrinhex) {
  bufindex = 0, instrargs = 0; hasregs = false;
  if (addr == 0) {Serial.println(F("Nothing to show.")); return;}
  Serial.println(F("Address Opcode  Arguments"));
  for (int lstprog = 0; lstprog < addr; lstprog++) {
    if (bufindex == 75 and !hasregs and lstprog < code) {
      if (lstprog < 16) {Serial.print(F("000"));} 
      else if (lstprog < 256) {Serial.print(F("00"));}
      else if (lstprog > 255) {Serial.print(F("0"));}
      
      Serial.print(lstprog, HEX);
      Serial.print("    ");
      Serial.println(pRAM[lstprog], HEX);
    } else if (!hasregs) {
      if (lstprog < 16) {Serial.print(F("000"));} 
      else if (lstprog < 256) {Serial.print(F("00"));}
      else if (lstprog > 255) {Serial.print(F("0"));}
      
      Serial.print(lstprog, HEX);
      Serial.print("    ");
      bufindex = pRAM[lstprog];
      memset(buf, 0, 16);

      if (bufindex > 90) {bufindex = 44;}
      
      if (!instrinhex) {memset(buf, 32, 8-strlen_P((char*)pgm_read_word(&(instrlist[bufindex]))));}
      else if (bufindex < 16) {memset(buf, 32, 6); Serial.print(F("0"));}
      else {memset(buf, 32, 6);}
      
      if (!instrinhex) {Serial.print(getInstr(bufindex)+String(buf));} else {Serial.print(bufindex, HEX); Serial.print(buf);}
      if (pgm_read_byte_near(params+bufindex)) {hasregs = true;} else {Serial.println("");} instrargs = 0;
    } else {
      if (bufindex == 76 or bufindex == 71 or (bufindex >= 22 and bufindex <= 27) or (bufindex >= 45 and bufindex <= 46) or (bufindex >= 79 and bufindex <= 80) or (bufindex >= 86 and bufindex <= 90)) {
        if (instrargs == 1) {
          code = word(pRAM[lstprog-1], pRAM[lstprog]);
          if (code < 16) {Serial.print(F("000"));} 
          else if (code < 256) {Serial.print(F("00"));}
          else if (code > 255) {Serial.print(F("0"));}
          Serial.print(code, HEX);
        }
      } else {
        if ((hlp_B & 0b10000000) and pgm_read_byte_near(regparams+bufindex)) {
          Serial.print(F("r"));
          Serial.print(char(pRAM[lstprog] + 65));
          Serial.print(F(" "));
          instrargs++;
        } else if ((hlp_B & 0b10000000) and !pgm_read_byte_near(regparams+bufindex) and instrargs == 0) {
          Serial.print("r");
          Serial.print(char(pRAM[lstprog] + 65));
          Serial.print(F(" "));
          instrargs++;
        } else {
          if (pRAM[lstprog] < 16) {Serial.print(F("0"));}
          Serial.print(pRAM[lstprog], HEX);
          Serial.print(F(" "));
          instrargs++;
        }
      }
      if (instrargs == pgm_read_byte_near(params+bufindex)) {
        if (bufindex == 75) {code = lstprog + pRAM[lstprog]; code++;}
        Serial.println(""); 
        hasregs = false;
      }
      if (bufindex == 76 or bufindex == 71 or (bufindex >= 22 and bufindex <= 27) or (bufindex >= 45 and bufindex <= 46)or (bufindex >= 79 and bufindex <= 80) or (bufindex >= 86 and bufindex <= 90)) {instrargs++;}
    }
  }
}

void format_conv(byte format) {
    if (format == 0) {Serial.print(F("Number to convert from hex: "));}
    else if (format == 1) {Serial.print(F("Number to convert to hex: "));}
    else if (format == 2) {Serial.print(F("Number to convert from binary: "));}
    else if (format == 3) {Serial.print(F("Number to convert to binary: "));}

    get_input();

    Serial.print(F("The converted number is: "));
    if (format == 0) {Serial.println(strtol(buf, (char **)NULL, 16));}
    else if (format == 1) {Serial.println(atoi(buf), HEX);}
    else if (format == 2) {Serial.println(strtol(buf, (char **)NULL, 2));}
    else if (format == 3) {Serial.println(atoi(buf), BIN);}
}

void memwrite(byte get_put) {
    Serial.print(F("Enter address number: "));

    get_input();

    code = (word)strtol(buf, (char **)NULL, 16);
    if (code > addr) {Serial.println(F("Memory address can not be larger then program address.")); return;}

    if (!get_put) {
      memset(buf, 0, 16);
      Serial.print(F("Number to write to address: "));

      get_input();

      pRAM[code] = atoi(buf);
    } else if (get_put == 1) {
      Serial.print(F("Number at address is: ")); 
      Serial.println(pRAM[code]);
    } else {
      memset(buf, 0, 16);
      Serial.print(F("Character to write to address: "));

      memset(buf, 0, 16);
      while (!Serial.available());
  
      bufidx = Serial.readBytesUntil(10, buf, 16);
      
      if ((hlp_B & 0b00000001) == 0) {Serial.println(buf);}

      pRAM[code] = buf[0];
    }
}

void get_input() {
  memset(buf, 0, 16);
  while (!Serial.available());
  
  bufidx = Serial.readBytesUntil(10, buf, 16);
    
  for (byte mod = 0; mod < bufidx; mod++) {
    buf[mod] = toupper(buf[mod]);
    if (buf[mod] == 13 or buf[mod] == 10) {buf[mod] = 0;}
  }
  if ((hlp_B & 0b00000001) == 0) {Serial.println(buf);}
}

void edit() {
  word tmp_addr = 0;
  if (addr == 0) {Serial.println(F("Nothing to modify.")); return;}
  
  Serial.print(F("Enter address number: "));
  get_input();
  code = (byte)strtol(buf, (char **)NULL, 16);

  if (code > addr) {Serial.println(F("Edit address can not be bigger then program address.")); return;}
  if (pRAM[code] == 75) {Serial.println(F("Cannot edit data space.")); return;}
  
  Serial.print(F("Edited instruction to insert: "));
  get_input();

  instrargs = pgm_read_byte_near(params+pRAM[code]);  
  if (pRAM[code] == 76 or pRAM[code] == 71 or (pRAM[code] >= 22 and pRAM[code] <= 27) or (pRAM[code] >= 45 and pRAM[code] <= 46) or (pRAM[code] >= 79 and pRAM[code] <= 80) or (pRAM[code] >= 86 and pRAM[code] <= 90)) {instrargs++;}

  bufindex = (code + instrargs) + 1;    
  memcpy(pRAM+code, pRAM+bufindex, addr);
  addr -= (instrargs + 1);
  
  bufidx = code + 3;    
  memmove(pRAM+bufidx, pRAM+code, addr);
  pRAM[code] = 0; pRAM[code+1] = 0; pRAM[code+2] = 0;

  tmp_addr = addr;
  addr = code;
    
  process_input();
    
  code = tmp_addr - (bufidx - 3);
  if (code >= (tmp_addr - addr)) {memcpy(pRAM+addr, pRAM+bufidx, code);}
  addr += code;
}

// ==================================================================
// BC256 --Byte Code 256-- INTEPRETER (1.0) "STABLE"
// ==================================================================
void BC256() {
  // A = [0], B = [1], C = [2], D = [3], ? = [4], ?? = [5], sp = [6], lpt = [7], lpa = [8], lpa = [9], lpc = [10]
  byte regs[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  word progaddr = 0;
  byte a = 0, b = 0, c = 0, instr = 0, instmu = 0;
  
  byte tmpmu[4] = {0, 0 , 0, 0};
  
  #define nxtc() pRAM[progaddr++]
  static void* labels[]= {&&instr_0,&&instr_1,&&instr_2,&&instr_3,&&instr_4,&&instr_5,&&instr_6,&&instr_7,&&instr_8,&&instr_9,&&instr_10,&&instr_11,&&instr_12,&&instr_13,&&instr_14,&&instr_15,&&instr_16,&&instr_17,&&instr_18,&&instr_19,&&instr_20,&&instr_21,&&instr_22,&&instr_23,&&instr_24,&&instr_25,&&instr_26,&&instr_27,&&instr_28,&&instr_29, &&instr_30,&&instr_31,&&instr_32,&&instr_33,&&instr_34,&&instr_35,&&instr_36,&&instr_37,&&instr_38,&&instr_39,&&instr_40,&&instr_41,&&instr_42,&&instr_43,&&instr_44,&&instr_45,&&instr_46,&&instr_47,&&instr_48,&&instr_49,&&instr_50,&&instr_51,&&instr_52,&&instr_53,&&instr_54,&&instr_55,&&instr_56,&&instr_57,&&instr_58,&&instr_59,&&instr_60,&&instr_61,&&instr_62,&&instr_63,&&instr_64,&&instr_65,&&instr_66,&&instr_67,&&instr_68,&&instr_69,&&instr_70,&&instr_71,&&instr_72,&&instr_73,&&instr_74,&&instr_75,&&instr_76,&&instr_77,&&instr_78,&&instr_79,&&instr_80,&&instr_81,&&instr_82,&&instr_83,&&instr_84,&&instr_85,&&instr_86,&&instr_87,&&instr_88,&&instr_89,&&instr_90};
  #define doinstr() {goto *(labels[instr]);}
     
run_prog:
  instr = nxtc();
  doinstr();

    instr_0: // => End program
      return;
      goto run_prog;
    instr_1: // => Mov reg <- reg
      a = nxtc();
      b = nxtc();
      regs[a] = regs[b];
      goto run_prog;
    instr_2: // => Mov reg <- byte
      a = nxtc();
      b = nxtc();
      regs[a] = b;
      goto run_prog;
    instr_3:
      a = nxtc();
      Serial.println(regs[a]);
      goto run_prog;
    instr_4: // => Add reg += reg
      a = nxtc();
      b = nxtc();
      regs[a] += regs[b];
      goto run_prog;
    instr_5: // => Add reg += byte
      a = nxtc();
      b = nxtc();
      regs[a] += b;
      goto run_prog;
    instr_6: // => Sub reg += reg
      a = nxtc();
      b = nxtc();
      regs[a] -= regs[b];
      goto run_prog;
    instr_7: // => Sub reg += byte
      a = nxtc();
      b = nxtc();
      regs[a] -= b;
      goto run_prog;
    instr_8: // => Mul reg *= reg
      a = nxtc();
      b = nxtc();
      regs[a] *= regs[b];
      goto run_prog;
    instr_9: // => Mul reg *= byte
      a = nxtc();
      b = nxtc();
      regs[a] *= b;
      goto run_prog;
    instr_10: // => Div reg /= reg
      a = nxtc();
      b = nxtc();
      regs[a] /= regs[b];
      goto run_prog;
    instr_11: // => Div reg /= byte
      a = nxtc();
      b = nxtc();
      regs[a] /= b;
      goto run_prog;
    instr_12: // => Mod reg %= reg
      a = nxtc();
      b = nxtc();
      regs[a] %= regs[b];
      goto run_prog;
    instr_13: // => Mod reg %= byte
      a = nxtc();
      b = nxtc();
      regs[a] %= b;
      goto run_prog;
    instr_14: // => Swap reg <-> reg
      a = nxtc();
      b = nxtc();
      c = regs[a];
      regs[a] = regs[b];
      regs[b] = c;
      goto run_prog;
    instr_15: // => Loop reg
      a = nxtc();
      regs[10] = 0;
      regs[7] = regs[a];
      regs[8] = progaddr >> 8;
      regs[9] = progaddr & 0xFF;
      goto run_prog;
    instr_16: // => Loop byte
      a = nxtc();
      regs[10] = 0;
      regs[7] = a;
      regs[8] = progaddr >> 8;
      regs[9] = progaddr & 0xFF;
      goto run_prog;
    instr_17: // => Loop end
      regs[10]++;
      if (regs[10] <= regs[7] and regs[10] != 0) {progaddr = word(regs[8], regs[9]);}
      goto run_prog;
    instr_18: // => Push reg
      a = nxtc();
      b = ram_sizes1 - regs[6];
      pRAM[b] = regs[a];
      regs[6]++;
      regs[a] = 0;
      goto run_prog;
    instr_19: // => Pop reg
      regs[6]--;
      a = nxtc();
      b = ram_sizes1 - regs[6];
      regs[a] = pRAM[b];
      goto run_prog;
    instr_20: // => Push all
      a = ram_sizes1 - regs[6];
      for (byte pshlp = 0; pshlp < 11; pshlp++) {if (pshlp != 6) {pRAM[a-pshlp] = regs[pshlp]; regs[pshlp] = 0;}}
      regs[6] += 11;
      goto run_prog;
    instr_21: // => Pop all
      a = ram_size - regs[6];
      for (byte poplp = 0; poplp < 11; poplp++) {if (poplp != 4) {regs[10-poplp] = pRAM[a+poplp];}}
      regs[6] -= 11;
      goto run_prog;
    instr_22: // => Jmp equal
      a = nxtc();
      b = nxtc();
      if (regs[4] == regs[5]) {progaddr = word(a, b);}
      goto run_prog;
    instr_23: // => Jmp less then
      a = nxtc();
      b = nxtc();
      if (regs[4] < regs[5]) {progaddr = word(a, b);}
      goto run_prog;
    instr_24: // => Jmp greater then
      a = nxtc();
      b = nxtc();
      if (regs[4] > regs[5]) {progaddr = word(a, b);}
      goto run_prog;
    instr_25: // => Jmp less or equal then
      a = nxtc();
      b = nxtc();
      if (regs[4] <= regs[5]) {progaddr = word(a, b);}
      goto run_prog;
    instr_26: // => Jmp greater or equal then
      a = nxtc();
      b = nxtc();
      if (regs[4] >= regs[5]) {progaddr = word(a, b);}
      goto run_prog;
    instr_27: // => Jmp not equal
      a = nxtc();
      b = nxtc();
      if (regs[4] != regs[5]) {progaddr = word(a, b);}
      goto run_prog;
    instr_28: // => Increase reg by one
      a = nxtc();
      regs[a]++;
      goto run_prog;
    instr_29: // => Decrease reg by one
      a = nxtc();
      regs[a]--;
      goto run_prog;
    instr_30: // => Delay millis reg
      a = nxtc();
      delay(regs[a]);
      goto run_prog;
    instr_31: // => Delay millis byte
      a = nxtc();
      delay(a);
      goto run_prog;
    instr_32: // => Delay micros reg
      a = nxtc();
      delayMicroseconds(regs[a]);
      goto run_prog;
    instr_33: // => Delay micros byte
      a = nxtc();
      delayMicroseconds(a);
      goto run_prog;
    instr_34: // => OUT byte byte
      a = nxtc();
      b = nxtc();
#if defined(__AVR_ATmega328P__) // Only enable PORT control on ATmega328P
      if (a == 0) {DDRB = b;}
      else if (a == 1) {PORTB = b;}
      else if (a == 2) {DDRC = b;}
      else if (a == 3) {PORTC = b;}
      else if (a == 4) {DDRD = b;}
      else if (a == 5) {PORTD = b;}
#endif
      goto run_prog;
    instr_35: // => OUTR reg reg
      a = nxtc();
      b = nxtc();
#if defined(__AVR_ATmega328P__) // Only enable PORT control on ATmega328P
      if (regs[a] == 0) {DDRB = regs[b];}
      else if (regs[a] == 1) {PORTB = regs[b];}
      else if (regs[a] == 2) {DDRC = regs[b];}
      else if (regs[a] == 3) {PORTC = regs[b];}
      else if (regs[a] == 4) {DDRD = regs[b];}
      else if (regs[a] == 5) {PORTD = regs[b];}
#endif
      goto run_prog;
    instr_36: // => OUTR reg byte
      a = nxtc();
      b = nxtc();
#if defined(__AVR_ATmega328P__) // Only enable PORT control on ATmega328P
      if (regs[a] == 0) {DDRB = b;}
      else if (regs[a] == 1) {PORTB = b;}
      else if (regs[a] == 2) {DDRC = b;}
      else if (regs[a] == 3) {PORTC = b;}
      else if (regs[a] == 4) {DDRD = b;}
      else if (regs[a] == 5) {PORTD = b;}
#endif
      goto run_prog;
    instr_37: // => INR reg reg
      b = nxtc();
      a = nxtc();
#if defined(__AVR_ATmega328P__) // Only enable PORT control on ATmega328P
      if (regs[a] == 0) {regs[b] = PINB;}
      else if (regs[a] == 1) {regs[b] = PINC;}
      else if (regs[a] == 2) {regs[b] = PIND;}
#endif
      goto run_prog;
    instr_38: // => INB reg byte
      b = nxtc();
      a = nxtc();
#if defined(__AVR_ATmega328P__) // Only enable PORT control on ATmega328P
      if (a == 0) {regs[b] = PINB;}
      else if (a == 1) {regs[b] = PINC;}
      else if (a == 2) {regs[b] = PIND;}
#endif
      goto run_prog;
    instr_39: // => Digital out reg
      a = nxtc();
      digitalWrite(regs[0], regs[a]);
      goto run_prog;
    instr_40: // => Digital out byte
      a = nxtc();
      digitalWrite(regs[0], a);
      goto run_prog;
    instr_41: // => Digital in reg
      a = nxtc();
      regs[a] = digitalRead(regs[0]);
      goto run_prog;
    instr_42: // => Pinmode reg
      a = nxtc();
      if (regs[a] == 0) {pinMode(regs[0], INPUT);}
      else if (regs[a] == 1) {pinMode(regs[0], OUTPUT);}
      else if (regs[a] == 2) {pinMode(regs[0], INPUT_PULLUP);}
      goto run_prog;
    instr_43: // => Pinmode byte
      a = nxtc();
      if (a == 0) {pinMode(regs[0], INPUT);}
      else if (a == 1) {pinMode(regs[0], OUTPUT);}
      else if (a == 2) {pinMode(regs[0], INPUT_PULLUP);}
      goto run_prog;
    instr_44: // => NOP
      goto run_prog;
    instr_45: // => put regs[1] to address + regs[0]
      a = nxtc();
      b = nxtc();
      pRAM[word(a, b)+regs[0]] = regs[1];
      goto run_prog;
    instr_46: // => get from address + regs[0] to regs[1]
      a = nxtc();
      b = nxtc();
      regs[1] = pRAM[word(a, b)+regs[0]];
      goto run_prog;
    instr_47: // => OUTBR byte reg
      a = nxtc();
      b = nxtc();
#if defined(__AVR_ATmega328P__) // Only enable PORT control on ATmega328P
      if (a == 0) {DDRB = regs[b];}
      else if (a == 1) {PORTB = regs[b];}
      else if (a == 2) {DDRC = regs[b];}
      else if (a == 3) {PORTC = regs[b];}
      else if (a == 4) {DDRD = regs[b];}
      else if (a == 5) {PORTD = regs[b];}
#endif
      goto run_prog;
    instr_48: // => AREF byte
      a = nxtc();
#if defined(__AVR_ATmega328P__) // Only enable AREF control on ATmega328P
      if (a == 0) {analogReference(DEFAULT);}
      else if (a == 1) {analogReference(INTERNAL);}
      else if (a == 2) {analogReference(EXTERNAL);}
#endif
      goto run_prog;
    instr_49: // => Analog out reg
      a = nxtc();
      analogWrite(regs[0], regs[a]);
      goto run_prog;
    instr_50: // => Analog out byte
      a = nxtc();
      analogWrite(regs[0], a);
      goto run_prog;
    instr_51: // => Analog in reg
      a = nxtc();
      regs[a] = analogRead(regs[0]) / 4;
      goto run_prog;
    instr_52: // => Right shift reg reg
      a = nxtc();
      b = nxtc();
      regs[a] >>= regs[b];
      goto run_prog;
    instr_53: // => Right shift reg byte
      a = nxtc();
      b = nxtc();
      regs[a] >>= b;
      goto run_prog;
    instr_54: // => Left shift reg reg
      a = nxtc();
      b = nxtc();
      regs[a] <<= regs[b];
      goto run_prog;
    instr_55: // => Left shift reg byte
      a = nxtc();
      b = nxtc();
      regs[a] <<= b;
      goto run_prog;
    instr_56: // => And reg &= reg
      a = nxtc();
      b = nxtc();
      regs[a] &= regs[b];
      goto run_prog;
    instr_57: // => And reg &= byte
      a = nxtc();
      b = nxtc();
      regs[a] &= b;
      goto run_prog;
    instr_58: // => Or reg |= reg
      a = nxtc();
      b = nxtc();
      regs[a] |= regs[b];
      goto run_prog;
    instr_59: // => Or reg |= byte
      a = nxtc();
      b = nxtc();
      regs[a] |= b;
      goto run_prog;
    instr_60: // => Xor reg ^= reg
      a = nxtc();
      b = nxtc();
      regs[a] ^= regs[b];
      goto run_prog;
    instr_61: // => Xor reg ^= byte
      a = nxtc();
      b = nxtc();
      regs[a] ^= b;
      goto run_prog;
    instr_62: // => Not reg ~= byte
      a = nxtc();
      regs[a] = ~regs[a];
      goto run_prog;
    instr_63: // => reg = random(reg, reg)
      a = nxtc();
      b = nxtc();
      regs[a] = random(regs[a], regs[b]);
      goto run_prog;
    instr_64: // => reg = random(reg, byte)
      a = nxtc();
      b = nxtc();
      regs[a] = random(regs[a], b);
      goto run_prog;
    instr_65: // => Aval reg
      a = nxtc();
      regs[a] = Serial.available();
      goto run_prog;
    instr_66: // => Read reg
      a = nxtc();
      regs[a] = Serial.read();
      goto run_prog;
    instr_67: // => Wread reg
      a = nxtc();
      while (!Serial.available());
      regs[a] = Serial.read();
      goto run_prog;
    instr_68: // => Printc reg
      a = nxtc();
      Serial.println(char(regs[a]));
      goto run_prog;
    instr_69: // => Printc byte
      a = nxtc();
      Serial.println(char(a));
      goto run_prog;
    instr_70: // => Serial start/stop
      a = nxtc();
      if (a) {Serial.end();} else {Serial.begin(115200);}
      goto run_prog;
    instr_71: // => Jmp Unconditional
      a = nxtc();
      b = nxtc();
      progaddr = word(a, b);
      goto run_prog;
    instr_72: // => Clear serial till 0 is found
      Serial.find(char(255));
      goto run_prog;
    instr_73: // => Skip next instruction if (reg & (1 << reg)) == 1
      a = nxtc();
      b = nxtc();
      if (regs[a] & (1 << regs[b])) {
        if (pRAM[progaddr] == 76 or pRAM[progaddr] == 71 or (pRAM[progaddr] >= 22 and pRAM[progaddr] <= 27) or (pRAM[progaddr] >= 45 and pRAM[progaddr] <= 46) or (pRAM[progaddr] >= 79 and pRAM[progaddr] <= 80) or (pRAM[progaddr] >= 86 and pRAM[progaddr] <= 90)) {progaddr+=3;}
        else {progaddr += pgm_read_byte_near(params+pRAM[progaddr]) + 1;}
      }
      goto run_prog;
    instr_74: // => Skip next instruction if (reg & (1 << reg)) == 0
      a = nxtc();
      b = nxtc();
      if (!(regs[a] & (1 << regs[b]))) {
        if (pRAM[progaddr] == 76 or pRAM[progaddr] == 71 or (pRAM[progaddr] >= 22 and pRAM[progaddr] <= 27) or (pRAM[progaddr] >= 45 and pRAM[progaddr] <= 46) or (pRAM[progaddr] >= 79 and pRAM[progaddr] <= 80) or (pRAM[progaddr] >= 86 and pRAM[progaddr] <= 90)) {progaddr+=3;}
        else {progaddr += pgm_read_byte_near(params+pRAM[progaddr]) + 1;}
      }
      goto run_prog;
    instr_75: // DATA => Automatically skip data space.
      progaddr += nxtc();
      goto run_prog;
    instr_76: // WIPE => Set a entire data space to zero.
      a = nxtc();
      b = nxtc();
      code = word(a, b);
      if (pRAM[code] == 75) {
        c = pRAM[code+1]; code += 2;
        for (byte clrlp = 0; clrlp < c; clrlp++) {pRAM[code + clrlp] = 0;}
      }
      goto run_prog;
    instr_77: // MSRFRSH => copy current millis into top address of stack
      a = nxtc();
      *(uint32_t *)tmpmu = millis();
      pRAM[(EEPROM_size + (regs[a] * 4))] = tmpmu[0];
      pRAM[(EEPROM_size + (regs[a] * 4)) + 1] = tmpmu[1];
      pRAM[(EEPROM_size + (regs[a] * 4)) + 2] = tmpmu[2];
      pRAM[(EEPROM_size + (regs[a] * 4)) + 3] = tmpmu[3];
      goto run_prog;
    instr_78: // USRFRSH => copy current micros into top address of stack
      a = nxtc();
      *(uint32_t *)tmpmu = micros();
      pRAM[(EEPROM_size + (regs[a] * 4))] = tmpmu[0];
      pRAM[(EEPROM_size + (regs[a] * 4)) + 1] = tmpmu[1];
      pRAM[(EEPROM_size + (regs[a] * 4)) + 2] = tmpmu[2];
      pRAM[(EEPROM_size + (regs[a] * 4)) + 3] = tmpmu[3];
      goto run_prog;
    instr_79: // JMPMS => jmp if millis > word(?, ??)
      tmpmu[0] = pRAM[(EEPROM_size + (regs[3] * 4))];
      tmpmu[1] = pRAM[(EEPROM_size + (regs[3] * 4)) + 1];
      tmpmu[2] = pRAM[(EEPROM_size + (regs[3] * 4)) + 2];
      tmpmu[3] = pRAM[(EEPROM_size + (regs[3] * 4)) + 3];
      
      a = nxtc();
      b = nxtc();
      if ((millis() - *(uint32_t *)tmpmu) > word(regs[4], regs[5])) {progaddr = word(a, b);}
      goto run_prog;
    instr_80: // JMPUS => jmp if micros > word(?, ??)
      tmpmu[0] = pRAM[(EEPROM_size + (regs[3] * 4))];
      tmpmu[1] = pRAM[(EEPROM_size + (regs[3] * 4)) + 1];
      tmpmu[2] = pRAM[(EEPROM_size + (regs[3] * 4)) + 2];
      tmpmu[3] = pRAM[(EEPROM_size + (regs[3] * 4)) + 3];
 
      a = nxtc();
      b = nxtc();
      if ((micros() - *(uint32_t *)tmpmu) > word(regs[4], regs[5])) {progaddr = word(a, b);}
      goto run_prog;
    instr_81: // => Boolean Or reg ||= reg
      a = nxtc();
      b = nxtc();
      regs[a] = regs[a] || regs[b];
      goto run_prog;
    instr_82: // => Boolean Or reg ||= byte
      a = nxtc();
      b = nxtc();
      regs[a] = regs[a] || b;
      goto run_prog;
    instr_83: // => Boolean And &&= reg
      a = nxtc();
      b = nxtc();
      regs[a] = regs[a] && regs[b];
      goto run_prog;
    instr_84: // => Boolean And reg &&= byte
      a = nxtc();
      b = nxtc();
      regs[a] = regs[a] && b;
      goto run_prog;
    instr_85: // => Boolean Not reg != byte
      a = nxtc();
      regs[a] = !regs[a];
      goto run_prog;
    instr_86: // => Print data array as characters
      a = nxtc();
      b = nxtc();
      code = word(a, b);
      if (pRAM[code] == 75) {
        c = pRAM[code+1]; code += 2;
        for (byte clrlp = 0; clrlp < c; clrlp++) {
          if (pRAM[code + clrlp] != 0) {Serial.print(char(pRAM[code + clrlp]));} else {break;}
        }
      }
      goto run_prog;
    instr_87: // => Print data array as numbers
      a = nxtc();
      b = nxtc();
      code = word(a, b);
      if (pRAM[code] == 75) {
        c = pRAM[code+1]; code += 2;
        for (byte clrlp = 0; clrlp < c; clrlp++) {
          if (pRAM[code + clrlp] < 16) {Serial.print(F("0"));}
          Serial.print(pRAM[code + clrlp], HEX);
          Serial.print(F(" "));
        }
        Serial.println();
      }
      goto run_prog;
    instr_88: // => Jmp if data array equal
      a = nxtc();
      b = nxtc();
      c = 0;
      if (pRAM[word(regs[2], regs[3])] == 75 and pRAM[word(regs[4], regs[5])] == 75) {
        c = pRAM[word(regs[2], regs[3])+1];
        for (byte clrlp = 0; clrlp < c; clrlp++) {
          if (pRAM[(word(regs[2], regs[3]) + 2) + clrlp] != pRAM[(word(regs[4], regs[5]) + 2) + clrlp]) {c = 1; break;}
          else if (pRAM[(word(regs[2], regs[3]) + 2) + clrlp] == 0) {break;}
         }
      }
      if (c == 0) {progaddr = word(a, b);}
      goto run_prog;
    instr_89: // => Jmp if data array not equal
      a = nxtc();
      b = nxtc();
      c = 0;
      if (pRAM[word(regs[2], regs[3])] == 75 and pRAM[word(regs[4], regs[5])] == 75) {
        c = pRAM[word(regs[2], regs[3])+1];
        for (byte clrlp = 0; clrlp < c; clrlp++) {
          if (pRAM[(word(regs[2], regs[3]) + 2) + clrlp] != pRAM[(word(regs[4], regs[5]) + 2) + clrlp]) {c = 1; break;}
          else if (pRAM[(word(regs[2], regs[3]) + 2) + clrlp] == 0) {break;}
         }
      }
      if (c == 1) {progaddr = word(a, b);}
      goto run_prog;
    instr_90: // => Pinmode for a data array
      a = nxtc();
      b = nxtc();
      code = word(a, b);
      if (pRAM[code] == 75) {
        c = pRAM[code+1]; code += 2;
#if defined(__AVR_ATmega328P__) // Only enable 20 IO pins on ATmega328P
        if (c > 20) {c = 20;}
#endif
        for (byte clrlp = 0; clrlp < c; clrlp++) {
          if (pRAM[code + clrlp] == 0) {pinMode(clrlp, INPUT);}
          else if (pRAM[code + clrlp] == 1) {pinMode(clrlp, OUTPUT);}
          else if (pRAM[code + clrlp] == 2) {pinMode(clrlp, INPUT_PULLUP);}
        }
      }
      goto run_prog;
}
//#pragma GCC pop_options
