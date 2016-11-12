/*
 * Author: Nathan Heskia
 * Program: A PDP 8 Interpreter
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TRUE            1
#define FALSE           0
#define MEMORY_SIZE     1 << 16
#define BUFFER_SIZE     200
#define MAX_INT         (1 << 15) - 1
#define MIN_INT         -(1 << 15)

FILE *OBJECT_FILE = NULL;

long long int time_counter = 0;
int a_reg = 0, b_reg = 0, c_reg = 0, d_reg = 0, link = 0;
int registers[8];
int SP = 0, SPL = 0, PC = 0, SPW = 0, next_address = 0;
int pdp_memory[MEMORY_SIZE];
int non_memory_masks[10] = {0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1}; // Group 1 masks
char register_symbols[8][4] = { "A","B","C","D","PC","PSW","SP","SPL" };
char buffer[BUFFER_SIZE];
char VERBOSE = FALSE, SKIP = FALSE, ROTATE_SELECTED = FALSE, OVERFLOW = FALSE, \
     PREV_SKIP = FALSE, ROTATE = 0;
char *symbolic = NULL, *regs = NULL; 

void remove_trailing_char(char* string, const char elim)
{
  char *temp = (char *)malloc(strlen(string) * sizeof(char) + 1);
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

void add_symbolic(char *s)
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

void add_registers(char *s)
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

void ADD(int register_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp_memory[memory_operand]);
    add_registers(buffer);
  }
  
  int result = (short int)registers[register_number] + (short int)pdp_memory[memory_operand];
  
  if(result > MAX_INT || result < (MIN_INT) )
  {
    OVERFLOW = TRUE;
    link = 1;
  }
  
  registers[register_number] = result & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    if(OVERFLOW)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}

void SUB(int register_number, int memory_operand)
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp_memory[memory_operand]);
    add_registers(buffer);
  }
  
  int result = (short int) registers[register_number] - (short int)(pdp_memory[memory_operand]);
  
  if(result > MAX_INT || result < (MIN_INT) )
  {
    OVERFLOW = TRUE;
    link = 1;
  }
  
  registers[register_number] = result & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    if(OVERFLOW)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}

void MUL(int register_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp_memory[memory_operand]);
    add_registers(buffer);
  }
  
  short int a = (short int) registers[register_number];
  short int b = (short int) pdp_memory[memory_operand];
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
  else if( int_result > MAX_INT || int_result < (MIN_INT) )
  {
    link = 1;
    OVERFLOW = TRUE;
  }

  registers[register_number] = result & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    if(OVERFLOW)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}

void DIV(int register_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp_memory[memory_operand]);
    add_registers(buffer);
  }
  
  if(pdp_memory[memory_operand] == 0)
  {
    registers[register_number] = 0;  
    link = 1;
    OVERFLOW = TRUE;
  }
  else
  {
    registers[register_number] = (short int) registers[register_number] / ((short int) (pdp_memory[memory_operand] & 0xFFFF));
  }
    
  registers[register_number] = registers[register_number] & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    if(OVERFLOW)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}

void AND(int register_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp_memory[memory_operand]);
    add_registers(buffer);
  }
  
  registers[register_number] = registers[register_number] & (pdp_memory[memory_operand] & 0xFFFF);
  registers[register_number] = registers[register_number] & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    sprintf (buffer, " 0x%04X -> %s", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }

}

void OR(int register_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp_memory[memory_operand]);
    add_registers(buffer);
  }
  
  registers[register_number] = (registers[register_number] & 0xFFFF) | (pdp_memory[memory_operand] & 0xFFFF);
  registers[register_number] = registers[register_number] & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    sprintf (buffer, " 0x%04X -> %s", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}
  
void XOR(int register_number, int memory_operand) 
{ 
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", memory_operand, pdp_memory[memory_operand]);
    add_registers(buffer);
  }
  
  registers[register_number] = registers[register_number] ^ (pdp_memory[memory_operand] & 0xFFFF);
  registers[register_number] = registers[register_number] & 0xFFFF;
  time_counter += 2;
  
  if(VERBOSE)
  {
    sprintf (buffer, " 0x%04X -> %s", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}

void LD(int register_number, int memory_operand) 
{
  if(VERBOSE)
  {
    sprintf (buffer, "M[0x%04X] -> 0x%04X,", memory_operand, pdp_memory[memory_operand]);
    add_registers(buffer);
    sprintf (buffer, " 0x%04X -> %s", pdp_memory[memory_operand], register_symbols[register_number]);
    add_registers(buffer);
  }
  
  registers[register_number] = pdp_memory[memory_operand] & 0xFFFF;
  time_counter += 2;
}

void ST(int register_number, int memory_operand) 
{
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
    sprintf (buffer, " 0x%04X -> M[0x%04X]", registers[register_number], memory_operand);
    add_registers(buffer);
  }
  
  pdp_memory[memory_operand] = registers[register_number] & 0xFFFF;
  time_counter += 2;
}

void READ_CHAR(int register_number)
{
  int ch = getchar();
  
  if(ch == EOF)
  {
    registers[register_number] = 0xFFFF;
  }
  else
  {
    registers[register_number] = ch & 0x00FF;
  }
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
  
  time_counter+=1;
}

void WRITE_CHAR(int register_number)
{
  char ch = registers[register_number] & 0xFF;
  putchar(ch);
  
  if(VERBOSE)
  { 
    sprintf (buffer, "%s -> 0x%04X", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
  }
  
  time_counter+=1;
}

void ISZ(int address)
{ 
  if(VERBOSE)
  { 
    sprintf (buffer, "M[0x%04X] -> 0x%04X,", address, pdp_memory[address]);
    add_registers(buffer);
  }
  
  pdp_memory[address] = (pdp_memory[address] + 1) & 0xFFFF;
  
  if(pdp_memory[address] == 0)
  {
    SKIP = TRUE;
    next_address = (next_address + 1) & 0xFFFF;
  }
  
  if(VERBOSE)
  { 
    sprintf (buffer, " 0x%04X -> M[0x%04X]", pdp_memory[address], address);
    add_registers(buffer);
  }
  time_counter += 3;
}

void JMP(int address)
{ 
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s", address, register_symbols[4]);
    add_registers(buffer);
  }
  
  next_address = address & 0xFFFF;
  time_counter += 1;
}

void CALL(int address)
{ 
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> M[0x%04X],", next_address, registers[6]);
    add_registers(buffer);
  }
  
  pdp_memory[registers[6]] = next_address;
  short int temp_register = registers[6];
  registers[6]--;
  
  if( temp_register < 0 && (short int) registers[6] > 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }

  registers[6] = registers[6] & 0xFFFF;
  next_address = (address & 0xFFFF);
  
  if(VERBOSE)
  {
    if(OVERFLOW)
    {
      sprintf (buffer, "0x%04X -> %s, ", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s,", registers[6], register_symbols[6]);
    add_registers(buffer);
    sprintf (buffer, " 0x%04X -> %s", next_address, register_symbols[4]);
    add_registers(buffer);
  }
  time_counter += 2;
}

void POP(int address)
{
  if( !(registers [6] < registers[7]) )
  { 
    if(VERBOSE)
    { 
      sprintf (buffer, "%s -> 0x%04X,", register_symbols[6], registers[6]);
      add_registers(buffer);
    }
  
    int temp_link = (registers[6] & 0x10000);
    registers[6]++;
  
    if( (registers[6] & 0x10000) != temp_link )
    {
      link = 1;
    }

    registers[6] = registers[6] & 0xFFFF;
    pdp_memory[address] = pdp_memory[registers[6]];
  
    if(VERBOSE)
    { 
      sprintf (buffer, " 0x%04X -> %s,", registers[6], register_symbols[6]);
      add_registers(buffer);
      sprintf (buffer, " M[0x%04X] -> 0x%04X,", registers[6], pdp_memory[registers[6]]);
      add_registers(buffer);
      sprintf (buffer, " 0x%04X -> M[0x%04X]", pdp_memory[registers[6]], address);
      add_registers(buffer);
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
      sprintf (buffer, "0x%04X -> %s, ", registers[6], register_symbols[6]);
      add_registers(buffer);
      sprintf (buffer, "%s -> 0x%04X, ", register_symbols[5], registers[5]);
      add_registers(buffer);
      registers[5] = 0;
      sprintf (buffer, "0x%04X -> %s", registers[5], register_symbols[5]);
      add_registers(buffer);
      add_symbolic("POP Stack Underflow");
    }
    
    next_address = MEMORY_SIZE;
    time_counter+=2;
  }
}

void PUSH(int address)
{
  if( !(registers [6] < registers[7]) )
  {
    if(VERBOSE)
    { 
      sprintf (buffer, "M[0x%04X] -> 0x%04X,", address, pdp_memory[address]);
      add_registers(buffer);
      sprintf (buffer, " 0x%04X -> M[0x%04X],", pdp_memory[address], registers[6]);
      add_registers(buffer);
    }
  
    pdp_memory[registers[6]]=pdp_memory[address];
  
    int temp_link = (registers[6] & 0x10000);
    registers[6]--;
  
    if( (registers[6] & 0x10000) != temp_link )
    {
      link = 1;
    }
  
    registers[6] = registers[6] & 0xFFFF;

    if(VERBOSE)
    { 
      sprintf (buffer, " 0x%04X -> %s", registers[6], register_symbols[6]);
      add_registers(buffer);
    }
    
    time_counter += 3;  
  }
  else
  {
    fprintf(stderr, "Stack Pointer = 0x%04X; Stack Limit = 0x%04X\n", registers[6], registers[7]);
    
    if(VERBOSE)
    { 
      sprintf (buffer, "M[0x%04X] -> 0x%04X, ", address, pdp_memory[address]);
      add_registers(buffer);
      sprintf (buffer, "%s -> 0x%04X, ", register_symbols[5], registers[5]);
      add_registers(buffer);
      registers[5] = 0;
      sprintf (buffer, "0x%04X -> %s", registers[5], register_symbols[5]);
      add_registers(buffer);
      add_symbolic("PUSH Stack Overflow");
    }
    next_address = MEMORY_SIZE;
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
  
  if(result > MAX_INT || result < MIN_INT)
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
  
  if(result > MAX_INT || result < MIN_INT)
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
  else if( int_result > MAX_INT || int_result < MIN_INT)
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
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[6], registers[6]);
    add_registers(buffer);
  }
  
  short int temp_register = (short int)registers[6];
  registers[6]++;
  
  if(temp_register > 0 && (short int)registers[6] < 0)
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  
  registers[6] = registers[6] & 0xFFFF;
  next_address = pdp_memory[registers[6]];
  
  if(VERBOSE)
  { 
    sprintf (buffer, " 0x%04X -> %s,", registers[6], register_symbols[6]);
    add_registers(buffer);
    if(OVERFLOW)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, " M[0x%04X] -> 0x%04X,", registers[6], pdp_memory[registers[6]]);
    add_registers(buffer);
    sprintf (buffer, " 0x%04X -> %s", pdp_memory[registers[6]], register_symbols[4]);
    add_registers(buffer);
  }
  time_counter += 2;
}

void HLT()
{ 
  if(VERBOSE)
  { 
    sprintf (buffer, "%s -> 0x%04X, ", register_symbols[5], registers[5]);
    add_registers(buffer);
  }
  next_address = MEMORY_SIZE;
  registers[5] = 0;
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s", registers[5], register_symbols[5]);
    add_registers(buffer);
  }
  time_counter += 1;
}

void SM(int register_number)
{ 
  if (((registers[register_number] & 0x1000) == 0x1000) && SKIP == FALSE)
  {
    SKIP = TRUE;
  }
  
  if(VERBOSE)
  {
     sprintf (buffer, "%s -> 0x%04X, ", register_symbols[register_number], registers[register_number]);
     add_registers(buffer);
  }
}

void SZ(int register_number)
{
  
  if (((registers[register_number] & 0xFFFF) == 0) && SKIP == FALSE) 
  {
    SKIP = TRUE;
  }
  
  if( VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X, ", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
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
    add_registers(buffer);
  }
}

void RSS()
{
  SKIP = !SKIP;
}

void CL(int register_number)
{
  registers[register_number] = 0;
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s, ", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}

void CLL()
{
  link = 0;
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s, ", link, "L");
    add_registers(buffer);
  }
}

void CM(int register_number)
{
  if(VERBOSE)
  {     
    sprintf (buffer, "%s -> 0x%04X, ", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
  }
  
  registers[register_number] = (~registers[register_number]) & 0xFFFF;
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s, ", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}

void CML()
{
  if(VERBOSE)
  {     
    sprintf (buffer, "%s -> 0x%04X, ", "L", link);
    add_registers(buffer);
  }
  
  link = (~link) & 0x1;
  
  if(VERBOSE)
  { 
    sprintf (buffer, "0x%04X -> %s, ", link, "L");
    add_registers(buffer);
  }
}

void DC(int register_number)
{
  if(VERBOSE)
  {     
    sprintf (buffer, "%s -> 0x%04X, ", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
  }
  
  short int temp_register = registers[register_number];
  registers[register_number]--;
  
  if(temp_register < 0 && (short int) registers[register_number] > 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }

  registers[register_number] = registers[register_number] & 0xFFFF;
  
  if(VERBOSE)
  { 
    if(OVERFLOW)
    {
      sprintf (buffer, "0x%04X -> %s, ", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, "0x%04X -> %s, ", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  }
}

void IN(int register_number)
{
  if(VERBOSE)
  {     
    sprintf (buffer, "%s -> 0x%04X, ", register_symbols[register_number], registers[register_number]);
    add_registers(buffer);
  }
  
  short int temp_register = registers[register_number];
  registers[register_number]++;
  
  if( temp_register > 0 && (short int) registers[register_number] < 0 )
  {
    link = 1;
    OVERFLOW = TRUE;
  }
  
  registers[register_number] = registers[register_number] & 0xFFFF;
  
  if(VERBOSE)
  { 
    if(OVERFLOW)
    {
      sprintf (buffer, "0x%04X -> %s, ", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, "0x%04X -> %s, ", registers[register_number], register_symbols[register_number]);
    add_registers(buffer);
  } 
}

void non_register_non_memory(int sub_opcode)
{ 
  switch(sub_opcode)
  {
    case 0x0: 
      time_counter += 1 ; 
      if(VERBOSE) 
      {
        add_symbolic("NOP");  
      } 
      break;
    case 0x1: 
      HLT(); 
      if(VERBOSE) 
      {
        add_symbolic("HLT");  
      } 
      break;
    case 0x2: 
      RET(); 
      if(VERBOSE) 
      {
        add_symbolic("RET");  
      } 
      break;
    default: 
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }
}
void register_memory(int opcode, int register_number, int di_bit, int zc_bit, int page)
{ 
  int memory_address = 0;
  
  if(zc_bit == 1)
  memory_address = (PC & 0xFF00);
  memory_address += page;
  
  if(di_bit == 1)
  {
    int prev_memory_address = memory_address;
    memory_address = pdp_memory[memory_address];
    time_counter+=1;
    
    if(VERBOSE)
    {
      sprintf (buffer, "M[0x%04X] -> 0x%04X, ", prev_memory_address, memory_address);
      add_registers(buffer);  
    }
  }
  
  switch(opcode)
  {
    case 0x1: 
      ADD(register_number, memory_address);  
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          add_symbolic("I");  
        }
        add_symbolic("ADD"); 
        add_symbolic(register_symbols[register_number]); 
      } 
      break;
    case 0x2: 
      SUB(register_number, memory_address);  
      if(VERBOSE)
      { 
        if(di_bit == 1)
        { 
          add_symbolic("I");  
        }
        add_symbolic("SUB");
        add_symbolic(register_symbols[register_number]); 
      }
      break;
    case 0x3: 
      MUL(register_number, memory_address);  
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          add_symbolic("I");  
        }
        add_symbolic("MUL"); 
        add_symbolic(register_symbols[register_number]);
      }
      break;
    case 0x4: 
      DIV(register_number, memory_address);  
      if(VERBOSE)
      { 
        if(di_bit == 1)
        {
          add_symbolic("I");  
        }
        add_symbolic("DIV"); 
        add_symbolic(register_symbols[register_number]); 
      }
      break;
    case 0x5: 
      AND(register_number, memory_address);  
      if(VERBOSE)
      {
        if(di_bit == 1) 
        {
          add_symbolic("I");  
        }
        add_symbolic("AND"); 
        add_symbolic(register_symbols[register_number]);
      }
      break;
    case 0x6: 
      OR(register_number, memory_address);   
      if(VERBOSE)
      {
        if(di_bit == 1) 
        {
          add_symbolic("I");  
        }
        add_symbolic("OR");
        add_symbolic(register_symbols[register_number]);
      }
      break;
    case 0x7: 
      XOR(register_number, memory_address);
      if(VERBOSE)
      {
        if(di_bit == 1)
        { 
          add_symbolic("I");  
        }
        add_symbolic("XOR"); 
        add_symbolic(register_symbols[register_number]); 
      }
      break;
    case 0x8: 
      LD(register_number, memory_address);
      if(VERBOSE)
      { 
        if(di_bit == 1)
        { 
          add_symbolic("I"); 
        } 
        add_symbolic("LD"); 
        add_symbolic(register_symbols[register_number]); 
      }
      break;
    case 0x9: 
      ST(register_number, memory_address);
      if(VERBOSE)
      {
        if(di_bit == 1) 
        {
          add_symbolic("I");  
        }
        add_symbolic("ST");
        add_symbolic(register_symbols[register_number]); 
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
        add_symbolic("IOT 3");  
      }
      break;
    case 0x4: 
      WRITE_CHAR((instruction & 0xC00) >> 10 ); 
      if(VERBOSE)
      {
        add_symbolic("IOT 4");
      }
      break;
    default:
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }
}

void non_register_memory(int opcode, int di_bit, int zc_bit, int page)
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
    memory_address = pdp_memory[memory_address];
    time_counter+=1;
    
    if(VERBOSE)
    {
      sprintf (buffer, "M[0x%04X] -> 0x%04X, ", prev_memory_address, memory_address);
      add_registers(buffer);  
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
          add_symbolic("I"); 
        }
        add_symbolic("ISZ"); 
      }  
      break;
    case 0x2D:
      JMP(memory_address);
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          add_symbolic("I"); 
        }
        add_symbolic("JMP"); 
      }    
      break;
    case 0x2E:
      CALL(memory_address); 
      if(VERBOSE) 
      {
        if(di_bit == 1) 
        {
          add_symbolic("I"); 
        }
        add_symbolic("CALL"); 
      }   
      break;
    case 0x30:
      PUSH(memory_address); 
      if(VERBOSE && registers[5]==1) 
      {
        if(di_bit == 1)
        {
          add_symbolic("I"); 
        }
        add_symbolic("PUSH"); 
      }
      break;
    case 0x31:  
      POP(memory_address);
      if(VERBOSE && registers[5]==1)
      {
        if(di_bit == 1) 
        {
          add_symbolic("I"); 
        }
        add_symbolic("POP"); 
      }
      break;
    default:
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }
  
  if(SKIP == TRUE && VERBOSE)
  {       
    sprintf (buffer, ", 0x%04X -> %s", next_address, "PC");
    add_registers(buffer);
    SKIP = FALSE;
  }
  
  OVERFLOW = FALSE;
}
void register_operation(int sub_opcode, int i, int j, int k)
{
  if(VERBOSE)
  {
    sprintf (buffer, "%s -> 0x%04X,", register_symbols[j], registers[j]);
    add_registers(buffer);
    sprintf (buffer, " %s -> 0x%04X,", register_symbols[k], registers[k]);
    add_registers(buffer);
  }
  
  switch(sub_opcode)
  {
    case 0x0: 
      REG_MOD(i, j, k); 
      if(VERBOSE) 
      { 
        add_symbolic("MOD"); 
      } 
      break;
    case 0x1: 
      REG_ADD(i, j, k); 
      if(VERBOSE) 
      { 
        add_symbolic("ADD"); 
      } 
      break;
    case 0x2: 
      REG_SUB(i, j, k); 
      if(VERBOSE) 
      { 
        add_symbolic("SUB"); 
      } 
      break;
    case 0x3: 
      REG_MUL(i, j, k); 
      if(VERBOSE) 
      { 
        add_symbolic("MUL"); 
      } 
      break;
    case 0x4: 
      REG_DIV(i, j, k); 
      if(VERBOSE) 
      { 
        add_symbolic("DIV"); 
      }
      break;
    case 0x5: 
      REG_AND(i, j, k); 
      if(VERBOSE) 
      { 
        add_symbolic("AND"); 
      } 
      break;
    case 0x6: 
      REG_OR(i, j, k);  
      if(VERBOSE) 
      { 
        add_symbolic("OR"); 
      }
      break;
    case 0x7: 
      REG_XOR(i, j, k); 
      if(VERBOSE)
      { 
        add_symbolic("XOR"); 
      }
      break;
    default:
      fprintf(stderr, "Invalid instruction.\n"); 
      break;
  }

  if(VERBOSE)
  {
    if(OVERFLOW)
    {
      sprintf (buffer, " 0x%04X -> %s,", link, "L");
      add_registers(buffer);  
    }
    sprintf (buffer, " 0x%04X -> %s", registers[i], register_symbols[i]);
    add_registers(buffer);
  }
  
  if(i == 4)
  {
    next_address = registers[i];
  }
  
  OVERFLOW = FALSE;
}

void non_memory_register(int register_number, int instruction)
{ 
  int i = 0;

  while(i<10)
  {
      switch( (instruction & non_memory_masks[i]) )
      {
        case 0x200: 
          if(SKIP == FALSE) 
          {
            SM(register_number); 
          }
          if(VERBOSE) 
          { 
            add_symbolic("SM");  
            add_symbolic(register_symbols[register_number]); 
          } 
          break;
        case 0x100: 
          if(SKIP == FALSE)
          { 
            SZ(register_number); 
          }
          if(VERBOSE) 
          { 
            add_symbolic("SZ");  
            add_symbolic(register_symbols[register_number]); 
          } 
          break;
        case 0x80:  
          if(SKIP == FALSE) 
          {
            SNL(); 
          }
          if(VERBOSE) 
          { 
            add_symbolic("SNL");
          } 
          break;
        case 0x40:  
          RSS();        
          if(VERBOSE) 
          { 
            add_symbolic("RSS");
          } 
          break;
        case 0x20:  
          CL(register_number);   
          if(VERBOSE) 
          { 
            add_symbolic("CL");  
            add_symbolic(register_symbols[register_number]); 
          } 
          break;
        case 0x10:  
          CLL();        
          if(VERBOSE) 
          { 
            add_symbolic("CLL");
          } 
          break;
        case 0x8: 
          CM(register_number);   
          if(VERBOSE) 
          { 
            add_symbolic("CM");  
            add_symbolic(register_symbols[register_number]); 
          } 
          break;
        case 0x4: 
          CML();        
          if(VERBOSE) 
          { 
            add_symbolic("CML");
          } 
          break;
        case 0x2: 
          DC(register_number);   
          if(VERBOSE) 
          { 
            add_symbolic("DC");  
            add_symbolic(register_symbols[register_number]); 
          } 
          break;
        case 0x1: 
          IN(register_number);   
          if(VERBOSE) 
          { 
            add_symbolic("IN");  
            add_symbolic(register_symbols[register_number]); 
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
      add_registers(buffer);  
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
      non_register_non_memory(value & 0x03FF); 
      break;
    case 0x1 : 
      register_memory((value & 0xF000) >> 12, (value & 0x0C00) >> 10, (value & 0x0200) >> 9, (value & 0x0100) >> 8, (value & 0x00FF)); 
      break;
    case 0xA : 
      iot(value); 
      break;
    case 0xB : 
      non_register_memory((value & 0xFC00) >> 10, (value & 0x0200) >> 9, (value & 0x0100) >> 8, (value & 0x00FF)); 
      break;
    case 0xE : 
      register_operation((value & 0x0E00) >> 9, (value & 0x01C0) >> 6, (value & 0x0038) >> 3, (value & 0x7) ); 
      break;
    case 0xF : 
      non_memory_register( (value & 0x0C00) >> 10, value & 0x3FF); 
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
  
  while( 0 <= next_address && next_address < MEMORY_SIZE && registers[5] == 1)
  {
    next_address = (PC + 1) & 0xFFFF;
    registers[4] = next_address;
    decode(pdp_memory[PC]);
    
    if(VERBOSE && symbolic == NULL) 
    {
      add_symbolic("");
    } 
    
    if(VERBOSE)
    {
      fprintf(stderr, "Time %3lld: PC=0x%04X instruction = 0x%04X (%s)", 
          time_counter, PC, pdp_memory[PC], symbolic);
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
  char found_object = FALSE, entry_point = FALSE;
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
            if(0 <= start_address && start_address < MEMORY_SIZE)
            {
              pdp_memory[start_address] = temp_instruct[m];
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
  int j = 0, file_given = FALSE, i = 0;
  
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
      file_given = TRUE;
      OBJECT_FILE = fopen(*argv, "r");
      
      if (OBJECT_FILE == NULL)
      {
        fprintf(stderr, "Can't open file %s\n", *argv);
        exit(1);
      }
      
      int k = 0;
      for(k = 0; k < MEMORY_SIZE; k++)
      {
        pdp_memory[k] = 0;
      }
      
      read_binary_object_file(OBJECT_FILE);
      process();
      fclose(OBJECT_FILE);
      exit(0);
    }
  }
  
  if(!file_given)
  {
    fprintf(stderr, "Object file not provided.\n");
    exit(1);
  }
}