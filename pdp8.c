/*
 * Author: Nathan Heskia
 * Program: A PDP 8 Interpreter
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef short Boolean;
#define TRUE 1
#define FALSE 0

FILE *objectFile = NULL;

int a_reg = 0, b_reg = 0, c_reg = 0, d_reg = 0, link = 0;
int registers[8];
int SP = 0, SPL = 0, PC = 0, SPW = 0, next_address = 0;
int pdp429memory[65536];
char reg_symbols[8][4] = { "A","B","C","D","PC","PSW","SP","SPL" };
char buffer[200];

short int VERBOSE = FALSE, SKIP = FALSE, ROTATE_SELECTED = FALSE, OVERFLOW = FALSE, PREV_SKIP = FALSE, ROTATE = 0;
long long int time_counter = 0;

int non_memory_masks[10] = {0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1}; // Group 1 masks

char *symbolic = NULL;
char *regs = NULL; 
int numberInstruct = 0;

void remove_trailing_char(char* string, const char elim)
{
  char *temp = (char *)malloc(strlen(string) * sizeof(char) + 1);
  int tempcounter = 0, stringcounter = 0, foundNotSpace = FALSE;
  
  int string_length = strlen(string);
  
  if(string_length != 0 && string != NULL)
  {
    int i = 0;
    
    for(i = 0; i < string_length; i++)
    {
      temp[i] = string[i];
    }
  
    temp[i] = '\0';
    
    while( (temp[i-1] == elim || temp[i-1] == ' ') && i >= 0)
    {
      i--;
    }
  
    temp[i] = '\0';
    strcpy(string, temp); 
    free(temp);
  }
  else
  {
    free(temp);
  }
}

void addSymbolic(char *s)
{  
  if(symbolic == NULL)
  {
    symbolic=(char *)malloc(strlen(s)*sizeof(char));
    strcpy(symbolic, s);
  }
  
  else if( strcmp("A", s) == 0 || strcmp("B", s) == 0 || strcmp("C", s) == 0 || strcmp("D", s) == 0)
  {
    int length = strlen(symbolic);
    symbolic = (char *)realloc(symbolic, 3 * length * sizeof(char) + 1);
    strcat(strcat(symbolic,""), s);
  }
  else
  {
    int length = strlen(symbolic);
    symbolic = (char *)realloc(symbolic, 3 * length * sizeof(char) + 1);
    strcat(strcat(symbolic," "), s);
  }
}

void addRegs(char *s)
{  
  if(regs == NULL)
  {
    regs = (char *)malloc(strlen(s) * sizeof(char) + 1);
    strcpy(regs, s);
  }
  else
  {
    int length = strlen(regs);
    regs = (char *)realloc(regs, 3 * length * sizeof(char) + 1);
    strcat(regs, s);
  }
}

void ADD(int reg_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp429memory[memory_operand]);
    addRegs(buffer);
  }
  
  int result = (short int)registers[reg_number] + (short int)pdp429memory[memory_operand];
  
  if(result > 32767 || result < (-32768) )
  {
    OVERFLOW = TRUE;
    link = 1;
  }
  
  registers[reg_number] = result & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}

void SUB(int reg_number, int memory_operand)
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp429memory[memory_operand]);
    addRegs(buffer);
  }
  
  int result = (short int) registers[reg_number] - (short int)(pdp429memory[memory_operand]);
  
  if(result > 32767 || result < (-32768) )
  {
    OVERFLOW = TRUE;
    link = 1;
  }
  
  registers[reg_number] = result & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}

void MUL(int reg_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp429memory[memory_operand]);
    addRegs(buffer);
  }
  
  short int a = (short int) registers[reg_number];
  short int b = (short int) pdp429memory[memory_operand];
  short int result = a * b;
  int int_result = a*b;
  
  if( a > 0 && b > 0 && result < 0)
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  else if ( a < 0 && b < 0 && result < 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  else if( int_result > 32767 || int_result < (-32768) )
  {
    link = 1;
    OVERFLOW = TRUE;
  }

  registers[reg_number] = result & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}

void DIV(int reg_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp429memory[memory_operand]);
    addRegs(buffer);
  }
  
  if(pdp429memory[memory_operand] == 0)
  {
    registers[reg_number] = 0;  
    link = 1;
    OVERFLOW = TRUE;
  }
  else
  {
    registers[reg_number] = (short int) registers[reg_number] / ((short int) (pdp429memory[memory_operand] & 0xFFFF));
  }
    
  registers[reg_number] = registers[reg_number] & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}

void AND(int reg_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp429memory[memory_operand]);
    addRegs(buffer);
  }
  
  registers[reg_number] = registers[reg_number] & (pdp429memory[memory_operand] & 0xFFFF);
  registers[reg_number] = registers[reg_number] & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    sprintf (buffer, " 0x%04X -> %s", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }

}

void OR(int reg_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp429memory[memory_operand]);
    addRegs(buffer);
  }
  
  registers[reg_number] = (registers[reg_number] & 0xFFFF) | (pdp429memory[memory_operand] & 0xFFFF);
  registers[reg_number] = registers[reg_number] & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    sprintf (buffer, " 0x%04X -> %s", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}
  
void XOR(int reg_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp429memory[memory_operand]);
    addRegs(buffer);
  }
  
  registers[reg_number] = registers[reg_number] ^ (pdp429memory[memory_operand] & 0xFFFF);
  registers[reg_number] = registers[reg_number] & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    sprintf (buffer, " 0x%04X -> %s", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}

void LD(int reg_number, int memory_operand) 
{
  if(VERBOSE)
  {
    sprintf (buffer, "M[0x%04X] -> 0x%04X,", memory_operand, pdp429memory[memory_operand]);
    addRegs(buffer);
    sprintf (buffer, " 0x%04X -> %s", pdp429memory[memory_operand], reg_symbols[reg_number]);
    addRegs(buffer);
  }
  
  registers[reg_number] = pdp429memory[memory_operand] & 0xFFFF;
  time_counter += 2;
}

void ST(int reg_number, int memory_operand) 
{
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
    sprintf (buffer, " 0x%04X -> M[0x%04X]", registers[reg_number], memory_operand);
    addRegs(buffer);
  }
  
  pdp429memory[memory_operand] = registers[reg_number] & 0xFFFF;
  time_counter += 2;
}

void READ_CHAR(int reg_number)
{
  int ch = getchar();
  
  if(ch == EOF)
  {
    registers[reg_number] = 0xFFFF;
  }
  else
  {
    registers[reg_number] = ch & 0x00FF;
  }
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
  
  time_counter+=1;
}

void WRITE_CHAR(int reg_number)
{
  char ch = registers[reg_number] & 0xFF;
  putchar(ch);
  
  if(VERBOSE)
  { 
    sprintf (buffer, "%s -> 0x%04X", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
  }
  
  time_counter+=1;
}

void ISZ(int address)
{ 
  if(VERBOSE)
  { 
    sprintf (buffer, "M[0x%04X] -> 0x%04X,", address, pdp429memory[address]);
    addRegs(buffer);
  }
  
  pdp429memory[address] = (pdp429memory[address] + 1) & 0xFFFF;
  
  if(pdp429memory[address] == 0)
  {
    SKIP = TRUE;
    next_address = (next_address + 1) & 0xFFFF;
  }
  
  if(VERBOSE)
  { 
    sprintf (buffer, " 0x%04X -> M[0x%04X]", pdp429memory[address], address);
    addRegs(buffer);
  }
  time_counter += 3;
}

void JMP(int address)
{ 
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s", address, reg_symbols[4]);
    addRegs(buffer);
  }
  
  next_address = address & 0xFFFF;
  time_counter += 1;
}

void CALL(int address)
{ 
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> M[0x%04X],", next_address, registers[6]);
    addRegs(buffer);
  }
  
  pdp429memory[registers[6]] = next_address;
  short int temp_reg = registers[6];
  registers[6]--;
  
  if( temp_reg < 0 && (short int) registers[6] > 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }

  registers[6] = registers[6] & 0xFFFF;
  next_address = (address & 0xFFFF);
  
  if(VERBOSE)
  {
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, "0x%04X -> %s, ", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s,", registers[6], reg_symbols[6]);
    addRegs(buffer);
    sprintf (buffer, " 0x%04X -> %s", next_address, reg_symbols[4]);
    addRegs(buffer);
  }
  time_counter += 2;
}

void POP(int address)
{
  if( !(registers [6] < registers[7]) )
  { 
    if(VERBOSE)
    { 
      sprintf (buffer, "%s -> 0x%04X,", reg_symbols[6], registers[6]);
      addRegs(buffer);
    }
  
    int temp_link = (registers[6] & 0x10000);
    registers[6]++;
  
    if( (registers[6] & 0x10000) != temp_link )
    {
      link = 1;
    }

    registers[6] = registers[6] & 0xFFFF;
    pdp429memory[address] = pdp429memory[registers[6]];
  
    if(VERBOSE)
    { 
      sprintf (buffer, " 0x%04X -> %s,", registers[6], reg_symbols[6]);
      addRegs(buffer);
      sprintf (buffer, " M[0x%04X] -> 0x%04X,", registers[6], pdp429memory[registers[6]]);
      addRegs(buffer);
      sprintf (buffer, " 0x%04X -> M[0x%04X]", pdp429memory[registers[6]], address);
      addRegs(buffer);
    }
    time_counter += 3;
  }
  
  else
  {
    fprintf(stderr, "Stack Pointer = 0x%04X;", registers[6]);
    registers[6]++;
    registers[6] = registers[6] & 0xFFFF;
    
    if(VERBOSE)
    { 
      sprintf (buffer, "0x%04X -> %s, ", registers[6], reg_symbols[6]);
      addRegs(buffer);
      sprintf (buffer, "%s -> 0x%04X, ", reg_symbols[5], registers[5]);
      addRegs(buffer);
      registers[5] = 0;
      sprintf (buffer, "0x%04X -> %s", registers[5], reg_symbols[5]);
      addRegs(buffer);
      addSymbolic("POP Stack Underflow");
    }
    
    next_address = 65536;
    time_counter+=2;
  }
}

void PUSH(int address)
{
  if( !(registers [6] < registers[7]) )
  {
    if(VERBOSE)
    { 
      sprintf (buffer, "M[0x%04X] -> 0x%04X,", address, pdp429memory[address]);
      addRegs(buffer);
      sprintf (buffer, " 0x%04X -> M[0x%04X],", pdp429memory[address], registers[6]);
      addRegs(buffer);
    }
  
    pdp429memory[registers[6]]=pdp429memory[address];
  
    int temp_link = (registers[6] & 0x10000);
    registers[6]--;
  
    if( (registers[6] & 0x10000) != temp_link )
    {
      link = 1;
    }
  
    registers[6] = registers[6] & 0xFFFF;

    if(VERBOSE)
    { 
      sprintf (buffer, " 0x%04X -> %s", registers[6], reg_symbols[6]);
      addRegs(buffer);
    }
    
    time_counter += 3;  
  }
  else
  {
    fprintf(stderr, "Stack Pointer = 0x%04X; Stack Limit = 0x%04X\n", registers[6], registers[7]);
    
    if(VERBOSE)
    { 
      sprintf (buffer, "M[0x%04X] -> 0x%04X, ", address, pdp429memory[address]);
      addRegs(buffer);
      sprintf (buffer, "%s -> 0x%04X, ", reg_symbols[5], registers[5]);
      addRegs(buffer);
      registers[5] = 0;
      sprintf (buffer, "0x%04X -> %s", registers[5], reg_symbols[5]);
      addRegs(buffer);
      addSymbolic("PUSH Stack Overflow");
    }
    next_address = 65536;
    time_counter+=2;
  }
}

void REG_MOD(int i, int j, int k)
{ 
  if((short int) registers[k] == 0)
  {
    registers[i] = 0; 
    link = 1;
    OVERFLOW = TRUE;
  }
  else
  {
    registers[i] = (short int) registers[j] % (short int) registers[k];
    registers[i] = registers[i] & 0xFFFF;
  }
  time_counter += 1;
}

void REG_ADD(int i, int j, int k)
{ 
  int result = (short int)registers[j] + (short int)registers[k];
  
  if(result > 32767 || result < (-32768) )
  {
    OVERFLOW = TRUE;
    link = 1;
  }
  registers[i] = result & 0xFFFF;
  time_counter += 1;  
}

void REG_SUB(int i, int j, int k)
{ 
  int result = (short int)registers[j] - (short int)registers[k];
  
  if(result > 32767 || result < (-32768) )
  {
    OVERFLOW = TRUE;
    link = 1;
  }
  registers[i] = result & 0xFFFF;
  time_counter += 1;  
}

void REG_MUL(int i, int j, int k)
{ 
  
  short int a = (short int) registers[j];
  short int b = (short int) registers[k];
  short int result = a * b;
  int int_result = a*b;
  
  if( a > 0 && b > 0 && result < 0)
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  else if ( a < 0 && b < 0 && result < 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  else if( int_result > 32767 || int_result < (-32768) )
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  registers[i] = result & 0xFFFF;
  time_counter += 1;
}

void REG_DIV(int i, int j, int k)
{ 
  if((short int) registers[k] == 0)
  {
    registers[i] = 0; 
    link = 1;
    OVERFLOW = TRUE;
  }
  else
  {
    registers[i] = (short int) registers[j] / (short int) registers[k];
    registers[i] = registers[i] & 0xFFFF;
  }
  time_counter += 1;
}

void REG_AND(int i, int j, int k)
{   
  registers[i] = (registers[j] & 0xFFFF)&(registers[k] & 0xFFFF);
  registers[i] = registers[i] & 0xFFFF;
  time_counter += 1;
}

void REG_OR(int i, int j, int k)
{ 
  registers[i] = (registers[j] & 0xFFFF)|(registers[k] & 0xFFFF);
  registers[i] = registers[i] & 0xFFFF;
  time_counter += 1;
}

void REG_XOR(int i, int j, int k)
{ 
  registers[i] = (registers[j] & 0xFFFF)^(registers[k] & 0xFFFF);
  registers[i] = registers[i] & 0xFFFF;
  time_counter += 1;  
}

void RET()
{
  if(VERBOSE)
  { 
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[6], registers[6]);
    addRegs(buffer);
  }
  
  short int temp_reg = (short int) registers[6];
  registers[6]++;
  
  if( temp_reg > 0 && (short int) registers[6] < 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  
  registers[6] = registers[6] & 0xFFFF;
  next_address = pdp429memory[registers[6]];
  
  if(VERBOSE)
  { 
    sprintf (buffer, " 0x%04X -> %s,", registers[6], reg_symbols[6]);
    addRegs(buffer);
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", registers[6], pdp429memory[registers[6]]);
    addRegs(buffer);
    sprintf (buffer, " 0x%04X -> %s", pdp429memory[registers[6]], reg_symbols[4]);
    addRegs(buffer);
  }
  time_counter += 2;
}

void HLT()
{ 
  if(VERBOSE)
  { 
    sprintf (buffer, "%s -> 0x%04X, ", reg_symbols[5], registers[5]);
    addRegs(buffer);
  }
  next_address = 65536;
  registers[5] = 0;
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s", registers[5], reg_symbols[5]);
    addRegs(buffer);
  }
  time_counter += 1;
}

void SM(int reg_number)
{ 
  if (((registers[reg_number] & 0x1000) == 0x1000) && SKIP == FALSE)
  {
    SKIP = TRUE;
  }
  
  if(VERBOSE)
  {
     sprintf (buffer, "%s -> 0x%04X, ", reg_symbols[reg_number], registers[reg_number]);
     addRegs(buffer);
  }
}

void SZ(int reg_number)
{
  
  if (((registers[reg_number] & 0xFFFF) == 0) && SKIP == FALSE) 
  {
    SKIP = TRUE;
  }
  
  if( VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X, ", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
  }
}

void SNL()
{
  if (((link & 0x1) == 0x1) && SKIP == FALSE)
  {
    SKIP = TRUE;  
  }
  
  if( VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X, ", "L", link);
    addRegs(buffer);
  }
}

void RSS()
{
  SKIP = (SKIP==FALSE) ? TRUE : FALSE;
}

void CL(int reg_number)
{
  registers[reg_number] = 0;
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s, ", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}

void CLL()
{
  link = 0;
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s, ", link, "L");
    addRegs(buffer);
  }
}

void CM(int reg_number)
{
  if(VERBOSE)
  {     
    sprintf (buffer, "%s -> 0x%04X, ", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
  }
  
  registers[reg_number] = (~registers[reg_number]) & 0xFFFF;
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s, ", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}

void CML()
{
  if(VERBOSE)
  {     
    sprintf (buffer, "%s -> 0x%04X, ", "L", link);
    addRegs(buffer);
  }
  
  link = (~link) & 0x1;
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s, ", link, "L");
    addRegs(buffer);
  }
}

void DC(int reg_number)
{
  if(VERBOSE)
  {     
    sprintf (buffer, "%s -> 0x%04X, ", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
  }
  
  short int temp_reg = registers[reg_number];
  registers[reg_number]--;
  
  if( temp_reg < 0 && (short int) registers[reg_number] > 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }

  registers[reg_number] = registers[reg_number] & 0xFFFF;
  
  if(VERBOSE)
  { 
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, "0x%04X -> %s, ", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, "0x%04X -> %s, ", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  }
}

void IN(int reg_number)
{
  if(VERBOSE)
  {     
    sprintf (buffer, "%s -> 0x%04X, ", reg_symbols[reg_number], registers[reg_number]);
    addRegs(buffer);
  }
  
  short int temp_reg = registers[reg_number];
  registers[reg_number]++;
  
  if( temp_reg > 0 && (short int) registers[reg_number] < 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  
  registers[reg_number] = registers[reg_number] & 0xFFFF;
  
  if(VERBOSE)
  { 
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, "0x%04X -> %s, ", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, "0x%04X -> %s, ", registers[reg_number], reg_symbols[reg_number]);
    addRegs(buffer);
  } 
}

void non_reg_non_memory(int sub_opcode)
{ 
  switch(sub_opcode)
  {
    case 0x0: 
      time_counter += 1 ; 
      if(VERBOSE) 
      {
        addSymbolic("NOP");  
      } 
      break;
    case 0x1: 
      HLT(); 
      if(VERBOSE) 
      {
        addSymbolic("HLT");  
      } 
      break;
    case 0x2: 
      RET(); 
      if(VERBOSE) 
      {
        addSymbolic("RET");  
      } 
      break;
    default: 
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }
}
void reg_memory(int opcode, int reg_number, int di_bit, int zc_bit, int page)
{ 
  int memory_address = 0;
  
  if(zc_bit == 1)
  memory_address = (PC & 0xFF00);
  memory_address += page;
  
  if(di_bit == 1)
  {
    int prev_memory_address = memory_address;
    memory_address = pdp429memory[memory_address];
    time_counter+=1;
    
    if(VERBOSE)
    {
      sprintf (buffer, "M[0x%04X] -> 0x%04X, ", prev_memory_address, memory_address);
      addRegs(buffer);  
    }
  }
  
  switch(opcode)
  {
    case 0x1: 
      ADD(reg_number, memory_address);  
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          addSymbolic("I");  
        }
        addSymbolic("ADD"); 
        addSymbolic(reg_symbols[reg_number]); 
      } 
      break;
    case 0x2: 
      SUB(reg_number, memory_address);  
      if(VERBOSE)
      { 
        if(di_bit == 1)
        { 
          addSymbolic("I");  
        }
        addSymbolic("SUB");
        addSymbolic(reg_symbols[reg_number]); 
      }
      break;
    case 0x3: 
      MUL(reg_number, memory_address);  
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          addSymbolic("I");  
        }
        addSymbolic("MUL"); 
        addSymbolic(reg_symbols[reg_number]);
      }
      break;
    case 0x4: 
      DIV(reg_number, memory_address);  
      if(VERBOSE)
      { 
        if(di_bit == 1)
        {
          addSymbolic("I");  
        }
        addSymbolic("DIV"); 
        addSymbolic(reg_symbols[reg_number]); 
      }
      break;
    case 0x5: 
      AND(reg_number, memory_address);  
      if(VERBOSE)
      {
        if(di_bit == 1) 
        {
          addSymbolic("I");  
        }
        addSymbolic("AND"); 
        addSymbolic(reg_symbols[reg_number]);
      }
      break;
    case 0x6: 
      OR(reg_number, memory_address);   
      if(VERBOSE)
      {
        if(di_bit == 1) 
        {
          addSymbolic("I");  
        }
        addSymbolic("OR");
        addSymbolic(reg_symbols[reg_number]);
      }
      break;
    case 0x7: 
      XOR(reg_number, memory_address);
      if(VERBOSE)
      {
        if(di_bit == 1)
        { 
          addSymbolic("I");  
        }
        addSymbolic("XOR"); 
        addSymbolic(reg_symbols[reg_number]); 
      }
      break;
    case 0x8: 
      LD(reg_number, memory_address);
      if(VERBOSE)
      { 
        if(di_bit == 1)
        { 
          addSymbolic("I"); 
        } 
        addSymbolic("LD"); 
        addSymbolic(reg_symbols[reg_number]); 
      }
      break;
    case 0x9: 
      ST(reg_number, memory_address);
      if(VERBOSE)
      {
        if(di_bit == 1) 
        {
          addSymbolic("I");  
        }
        addSymbolic("ST");
        addSymbolic(reg_symbols[reg_number]); 
      }
      break;
    default:
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }
  
  OVERFLOW = FALSE;
}

void iot(int instruction)
{ 
  switch( (instruction & 0x3F8) >> 3 )
  {
    case 0x3: 
      READ_CHAR((instruction & 0xC00) >> 10)  ; 
      if(VERBOSE) 
      {
        addSymbolic("IOT 3");  
      }
      break;
    case 0x4: 
      WRITE_CHAR((instruction & 0xC00) >> 10 ); 
      if(VERBOSE)
      {
        addSymbolic("IOT 4");
      }
      break;
    default:
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }
}

void non_reg_memory(int opcode, int di_bit, int zc_bit, int page)
{ 
  int memory_address = 0;
  
  if(zc_bit == 1)
  {
    memory_address = (PC & 0xFF00);
  }
  memory_address += page;
  
  if(di_bit == 1)
  {
    int prev_memory_address = memory_address;
    memory_address = pdp429memory[memory_address];
    time_counter+=1;
    
    if(VERBOSE)
    {
      sprintf (buffer, "M[0x%04X] -> 0x%04X, ", prev_memory_address, memory_address);
      addRegs(buffer);  
    }
  }
  
  switch(opcode)
  {
    case 0x2C:  
      ISZ(memory_address);  
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          addSymbolic("I"); 
        }
        addSymbolic("ISZ"); 
      }  
      break;
    case 0x2D:
      JMP(memory_address);
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          addSymbolic("I"); 
        }
        addSymbolic("JMP"); 
      }    
      break;
    case 0x2E:
      CALL(memory_address); 
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          addSymbolic("I"); 
        }
        addSymbolic("CALL"); 
      }   
      break;
    case 0x30:
      PUSH(memory_address); 
      if(VERBOSE && registers[5]==1) 
      {
        if(di_bit == 1)
        {
          addSymbolic("I"); 
        }
        addSymbolic("PUSH"); 
      }
      break;
    case 0x31:  
      POP(memory_address);
      if(VERBOSE && registers[5]==1)
      {
        if(di_bit == 1) 
        {
          addSymbolic("I"); 
        }
        addSymbolic("POP"); 
      }
      break;
    default:
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }
  
  if(SKIP == TRUE && VERBOSE)
  {       
    sprintf (buffer, ", 0x%04X -> %s", next_address, "PC");
    addRegs(buffer);
    SKIP = FALSE;
  }
  
  OVERFLOW = FALSE;
}
void reg_reg(int sub_opcode, int i, int j, int k)
{
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", reg_symbols[j], registers[j]);
    addRegs(buffer);
    sprintf (buffer, " %s -> 0x%04X,", reg_symbols[k], registers[k]);
    addRegs(buffer);
  }
  
  switch(sub_opcode)
  {
    case 0x0: 
      REG_MOD(i, j, k); 
      if(VERBOSE) 
      { 
        addSymbolic("MOD"); 
      } 
      break;
    case 0x1: 
      REG_ADD(i, j, k); 
      if(VERBOSE) 
      { 
        addSymbolic("ADD"); 
      } 
      break;
    case 0x2: 
      REG_SUB(i, j, k); 
      if(VERBOSE) 
      { 
        addSymbolic("SUB"); 
      } 
      break;
    case 0x3: 
      REG_MUL(i, j, k); 
      if(VERBOSE) 
      { 
        addSymbolic("MUL"); 
      } 
      break;
    case 0x4: 
      REG_DIV(i, j, k); 
      if(VERBOSE) 
      { 
        addSymbolic("DIV"); 
      }
      break;
    case 0x5: 
      REG_AND(i, j, k); 
      if(VERBOSE) 
      { 
        addSymbolic("AND"); 
      } 
      break;
    case 0x6: 
      REG_OR(i, j, k);  
      if(VERBOSE) 
      { 
        addSymbolic("OR"); 
      }
      break;
    case 0x7: 
      REG_XOR(i, j, k); 
      if(VERBOSE)
      { 
        addSymbolic("XOR"); 
      }
      break;
    default:
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }

  if(VERBOSE)
  {
    if(OVERFLOW == TRUE)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      addRegs(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[i], reg_symbols[i]);
    addRegs(buffer);
  }
  
  if(i == 4)
  {
    next_address = registers[i];
  }
  
  OVERFLOW = FALSE;
}

void non_memory_reg(int reg_number, int instruction)
{ 
  int i = 0;

  while(i<10)
  {
      switch( (instruction & non_memory_masks[i]) )
      {
        case 0x200: 
          if(SKIP == FALSE) 
          {
            SM(reg_number); 
          }
          if(VERBOSE) 
          { 
            addSymbolic("SM");  
            addSymbolic(reg_symbols[reg_number]); 
          } 
          break;
        case 0x100: 
          if(SKIP == FALSE)
          { 
            SZ(reg_number); 
          }
          if(VERBOSE) 
          { 
            addSymbolic("SZ");  
            addSymbolic(reg_symbols[reg_number]); 
          } 
          break;
        case 0x80:  
          if(SKIP == FALSE) 
          {
            SNL(); 
          }
          if(VERBOSE) 
          { 
            addSymbolic("SNL");
          } 
          break;
        case 0x40:  
          RSS();        
          if(VERBOSE) 
          { 
            addSymbolic("RSS");
          } 
          break;
        case 0x20:  
          CL(reg_number);   
          if(VERBOSE) 
          { 
            addSymbolic("CL");  
            addSymbolic(reg_symbols[reg_number]); 
          } 
          break;
        case 0x10:  
          CLL();        
          if(VERBOSE) 
          { 
            addSymbolic("CLL");
          } 
          break;
        case 0x8: 
          CM(reg_number);   
          if(VERBOSE) 
          { 
            addSymbolic("CM");  
            addSymbolic(reg_symbols[reg_number]); 
          } 
          break;
        case 0x4: 
          CML();        
          if(VERBOSE) 
          { 
            addSymbolic("CML");
          } 
          break;
        case 0x2: 
          DC(reg_number);   
          if(VERBOSE) 
          { 
            addSymbolic("DC");  
            addSymbolic(reg_symbols[reg_number]); 
          } 
          break;
        case 0x1: 
          IN(reg_number);   
          if(VERBOSE) 
          { 
            addSymbolic("IN");  
            addSymbolic(reg_symbols[reg_number]); 
          } 
          break;
        default: 
          break;
      }
    i++;  
  }
  
  if(regs != NULL)
  {
    remove_trailing_char(regs, ',');
  }

  if(SKIP == TRUE)
  { 
    next_address = (next_address + 1) & 0xFFFF;
    SKIP = FALSE;
    
    if(VERBOSE)
    {       
      sprintf (buffer, ", 0x%04X -> %s", next_address, "PC");
      addRegs(buffer);  
    }
  }
  time_counter += 1;
  
  OVERFLOW = FALSE;
  PREV_SKIP = FALSE;
}

void decode(int value)
{
  int case_number = 0;
  
  if( ( (value & 0xF000) >> 10) == 0)
  {
    case_number = 0x0;
  }
  else if( 1 <= ( (value & 0xF000) >> 12) && ( (value & 0xF000) >> 12) <= 9 )
  {
    case_number = 0x1;
  }
  else if( ( (value & 0xF000) >> 12) == 0xA )
  {
    case_number = 0xA;
  }
  else if( ( (value & 0xF000) >> 12) == 0xB || ( (value & 0xF000) >> 12) == 0xC)
  {
    case_number = 0xB;
  }
  else if( ( (value & 0xF000) >> 12) == 0xE)
  {
    case_number = 0xE;
  }
  else if( ( (value & 0xF000) >> 12) == 0xF)
  {
    case_number = 0xF;
  }
  
  switch(case_number)
  {
    case 0x0 : 
      non_reg_non_memory(value & 0x03FF); 
      break;
    case 0x1 : 
      reg_memory((value & 0xF000) >> 12, (value & 0x0C00) >> 10, (value & 0x0200) >> 9, (value & 0x0100) >> 8, (value & 0x00FF)); 
      break;
    case 0xA : 
      iot(value); 
      break;
    case 0xB : 
      non_reg_memory((value & 0xFC00) >> 10, (value & 0x0200) >> 9, (value & 0x0100) >> 8, (value & 0x00FF)); 
      break;
    case 0xE : 
      reg_reg((value & 0x0E00) >> 9, (value & 0x01C0) >> 6, (value & 0x0038) >> 3, (value & 0x7) ); 
      break;
    case 0xF : 
      non_memory_reg( (value & 0x0C00) >> 10, value & 0x3FF); 
      break;
    default : 
      fprintf(stderr, "Unrecognized instruction type.\n"); 
      break;
  }
}


void process()
{
  next_address = PC & 0xFFFF;
  registers[5] = 1;
  
  while( 0 <= next_address && next_address < 65536 && registers[5] == 1)
  {
    next_address = (PC + 1) & 0xFFFF;
    registers[4] = next_address;
    decode(pdp429memory[PC]);
    
    if(VERBOSE && symbolic == NULL) 
    {
      addSymbolic("");
    } 
    
    if(VERBOSE)
    {
      fprintf(stderr, "Time %3lld: PC=0x%04X instruction = 0x%04X (%s)", 
          time_counter, PC, pdp429memory[PC], symbolic);
      if (regs != NULL) 
      {
        fprintf(stderr, ": %s", regs);
      }          
      fprintf(stderr, "\n");
    }
    
    if(symbolic != NULL)
    { 
      free(symbolic); 
      symbolic = NULL;
    }
    if(regs != NULL)
    { 
      free(regs); 
      regs = NULL;
    }
    
    PC = next_address;
  }
}


void read_binary_object_file(FILE* objFile)
{
  int c = 0, i = 0, numberchars = 0, objg_counter = 0, ep_counter = 0;
  int EP = 0, b_size = 0, bsizes = 0, counter = 0;
  Boolean found_object = FALSE, entry_point = FALSE;
  char OBJG[5];
  
  while( (c = getc(objFile)) != EOF )
  {
    if(objg_counter < 4)
    {
      if(objg_counter < 4)
      {
        OBJG[objg_counter] = c;
        objg_counter++;
        
        if(objg_counter == 4)
        {
          OBJG[objg_counter] = '\0';
        }
      }
      
      if( strcmp(OBJG, "OBJG") == 0 )
      {
        found_object = TRUE;
      }
      
      if(objg_counter == 4 && strcmp(OBJG,"OBJG") != 0 )
      {
        fprintf(stderr, "OBJG not found!\n - %s", OBJG);
        exit(1);
      }
    }
    else if(entry_point == FALSE)
    {
      if(ep_counter == 0)
      {
        EP = (c << 8);
        ep_counter++;
      }
      else 
      {
        EP = EP + c;
        entry_point = TRUE;
      }
    }
    else if(entry_point == TRUE)
    {     
      if(c >= 5)
      {
        b_size = c;
        bsizes += c;

        int j = 0, instruct = 0, instruct_counter = 0, m = 0, start_address = 0;
        int *temp_instruct = NULL;
        temp_instruct = (int *)calloc(b_size, sizeof(int)); 
        
        while( (j < b_size - 1) && ((c = getc(objFile)) != EOF))
        {
          if(counter == 0)
          {
            instruct = (c << 8);  
            counter = 1;
          }
          else
          {
            instruct += c;
            counter = 0;
            temp_instruct[instruct_counter] = instruct;
            instruct_counter++;
            
          }
          j++;
        }
                
        while( m < instruct_counter)
        {
          if(m == 0)
          {
            start_address = temp_instruct[m];
          }
          else 
          {
            if(0 <= start_address && start_address < 65536)
            {
              pdp429memory[start_address] = temp_instruct[m];
              start_address++;  
            }
            else
            {
              fprintf(stderr, "Memory reference out of bounds! \n");
              exit(1);
            }
          }
          m++;
        }
        
        if(temp_instruct != NULL)
        {
          free(temp_instruct);
          temp_instruct = NULL;
        }
      }
    }
  }
  
  PC = EP;
  
}

int main(int argc, char** argv)
{
  int j = 0, objectFileNotGiven = TRUE, i = 0;
  
  while (argc > 1)
  {
    argc--, argv++;
    
    if (**argv == '-')
    {
      if(strcmp("-v",*argv) == 0 || strcmp("-V",*argv) == 0)
      {
        VERBOSE = TRUE;
      }
    }
    else
    {
      objectFileNotGiven = FALSE;
      objectFile = fopen(*argv, "r");
      
      if (objectFile == NULL) //if the file wasn't opened.
      {
        fprintf(stderr, "Can't open file %s\n", *argv);
        exit(1);
      }
      
      int k = 0;
      for(k = 0; k < 65536; k++)
      {
        pdp429memory[k] = 0;
      }
      
      read_binary_object_file(objectFile);
      process();
      fclose(objectFile);
      exit(0);
    }
  }
  
  if(objectFileNotGiven == TRUE)
  {
    fprintf(stderr, "Object file not provided.\n");
    exit(1);
  }
}