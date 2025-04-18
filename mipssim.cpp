#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <string>
#include <bitset>
#include <fstream>
#include <unordered_map>

using namespace std;

int main(int argc, char *argv[])
{

  struct memItem {
    int I, opcode, rs, rt, rd, shamt, func, imm, jtarget, boffset; 
    int src1, src2, dest;
    unsigned int UI;
    string binstr; 
    string binstrNoSpace;
    string instr, outline;
    memItem():dest(-2) { }
  };

  unordered_map<int, memItem> MEM;
  int firstMem=0, lastMem=0;
  char buffer[4];
  int I;
  char * iPtr;
  iPtr = (char*)(void*) &I;
  int count = 0;
  int FD = open(argv[2], O_RDONLY);
  string outdisstr = string(argv[4] ) + "_dis.txt";
  ofstream outdis( outdisstr );
  int address = 96;
  int amt = 4;
  bool seenBreak = false;
  while( amt != 0 )
  {
    amt = read(FD, buffer, 4);
    if( amt == 4)
    {
      iPtr[0] = buffer[3];
      iPtr[1] = buffer[2];
      iPtr[2] = buffer[1];
      iPtr[3] = buffer[0];
      memItem M;
      M.I= I;
      M.UI = static_cast<unsigned int>( I );
      M.opcode = M.UI >> 26;
      M.rs = M.UI << 6 >> 27;
      M.rt = M.UI << 11 >> 27;
      M.rd = M.UI << 16 >> 27;
      M.imm = M.I << 16 >> 16;
      M.shamt = M.UI << 21 >> 27;
      M.func = M.UI << 26 >> 26;
      M.jtarget = M.UI << 6 >> 4;
      M.boffset = M.I <<16 >> 14;
      M.binstr = bitset<32>(I).to_string();
      M.binstrNoSpace = M.binstr;
      M.binstr.insert(26," ").insert(21," ").insert(16," ").insert(11," ").insert(6," ");
      string instr;
      string outline;
      //binstrSpace = binstrSpace;
      if(! seenBreak ) {  
        outline = M.binstr + "\t";
        // cout << outline;
        if( M.I == 0 ) {  //NOP
          instr += "\tNOP";
          outline += to_string(address) + instr;
          M.src1 = 0;
          M.src2 = 0;
          M.dest = 0;
        } else if( M.opcode == 8 ){  // ADDI
          instr += "\tADDI\tR" + to_string(M.rt) + ", R" + to_string(M.rs) + ", #" + to_string(M.imm);
          outline += to_string(address) + instr; 
          M.src1 = M.rs;
          M.src2 = M.rs;
          M.dest = M.rt;
        } else if( M.opcode == 43 ){ //SW
          instr += "\tSW\tR" + to_string(M.rt) + ", " + to_string(M.imm) + "(R" + to_string(M.rs) + ")";
          outline += to_string(address) + instr;
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = -1;
        } else if( M.opcode == 35 ){ //LW
          instr += "\tLW\tR" + to_string(M.rt) + ", " + to_string(M.imm) + "(R" + to_string(M.rs) + ")";
          outline += to_string(address) + instr;
          M.src1 = M.rs;
          M.src2 = M.rs;
          M.dest = M.rt;
        } else if( M.opcode == 1 ){  // BLTZ
          instr += "\tBLTZ\tR" + to_string(M.rs) + ", #" + to_string(M.boffset);
          outline += to_string(address) + instr;
          M.src1 = M.rs;
          M.src2 = M.rs;
          M.dest = -1;
        } else if( M.opcode == 0 && M.func == 0){  // SLL
          instr += "\tSLL\tR" + to_string(M.rd) + ", R" + to_string(M.rt) + ", #" + to_string(M.shamt);
          outline += to_string(address) + instr;
          M.src1 = M.rt;
          M.src2 = M.rt;
          M.dest = M.rd;
        } else if( M.opcode == 0 && M.func == 2){  // SRL
          instr += "\tSRL\tR" + to_string(M.rd) + ", R" + to_string(M.rt) + ", #" + to_string(M.shamt);
          outline += to_string(address) + instr;
          M.src1 = M.rt;
          M.src2 = M.rt;
          M.dest = M.rd;
        } else if( M.opcode == 0 && M.func == 34){  // SUB
          instr += "\tSUB\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          outline += to_string(address) + instr;
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = M.rd;
        } else if( M.opcode == 28 && M.func == 2){  // MUL
          instr += "\tMUL\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          outline += to_string(address) + instr;
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = M.rd;
        } else if( M.opcode == 0 && M.func == 10){  // MOVZ
          instr += "\tMOVZ\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          outline += to_string(address) + instr;
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = M.rd;
        }  else if( M.opcode == 2 ){  // J
          instr += "\tJ\t#" + to_string(M.jtarget);
          outline += to_string(address) + instr;
          M.src1 = -1;
          M.src2 = -1;
          M.dest = 0;
        }  else if( M.opcode == 0 && M.func == 32){  // ADD
          instr += "\tADD\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          outline += to_string(address) + instr;
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = M.rs;
        }  else if( M.opcode == 0 && M.func == 13 ){  // BREAK
          instr += "\tBREAK\t";
          outline += to_string(address) + instr;
          seenBreak = true;
          firstMem = address+4;
          M.src1 = -1;
          M.src2 = -1;
          M.dest = 0;
        } else if( M.opcode == 0 && M.func == 8){  // JR
          instr += "\tJR\tR" + to_string(M.rs); 
          outline += to_string(address) + instr;
          M.src1 = M.rs;
          M.src2 = M.rs;
          M.dest = 0;
        } 
        else {
          cerr << "Unkown opcode: " << M.opcode << endl;
          exit( 0 );
        }
        M.outline = outline;
      } else // end of processing opcodes,  for th rest process memory
      {
        instr = M.binstrNoSpace + "\t";
        instr += to_string(address) + " " +to_string(M.I); 
        M.outline = instr;
      }
      M.instr = instr;
      MEM[address] = M;
      outdis << M.outline << endl;
      lastMem = address;
      address +=4;
    }
  }
  outdis.close();

  // start sim
  //
  //
  string outsimstr = string(argv[4] ) + "_sim.txt";
  ofstream outsim( outsimstr );
  
  struct processor {
    int PC=96, R[32]={0}, cycle=1, fetch, firstMem, lastMem;
    memItem preIssue[4], preALU[2], preMem[2], postALU, postMem;
    unordered_map<int, memItem> MEM;

    processor( unordered_map<int, memItem> MEM, int _firstMem, int _lastMem ):
      MEM( MEM ), firstMem(_firstMem), lastMem(_lastMem)
    { }
    void IF(){
      fetch=PC;
      for( int i = 0; i < 2; i++ ){
        if( preIssue[3].instr != "" ) break;

        memItem I = MEM[PC];
        // check for branch or jump
        //
        int j =0;
        for( ; j<4; j++ )
          if( preIssue[j].instr == "" ) break;
        preIssue[j] = I;

        PC += 4;
      }
        
    }
    bool XBW( int r, int start){
      for( int i = start; i >=0; i--)
        if( preIssue[i].dest == r ) return true;
      if( preMem[1].instr != "" && preMem[1].dest == r ) return true;
      if( preMem[1].dest == r ) return true;
      if( preALU[0].dest == r ) return true;
      if( preALU[0].dest == r ) return true;
      if( postALU.dest == r ) return true;
      if( postMem.dest == r ) return true;
      return false;
    }
    void ISSUE(){
      for( int i = 0; i< 4; i++ ){
        cout << "preissue " << preIssue[i].instr << endl;
        if( preIssue[i].instr == "" ) continue;
        cout << "0" << endl;
        if( XBW( preIssue[i].src1, i-1 ) ) continue;
        cout << "1" << endl;
        if( XBW( preIssue[i].src2, i-1 ) ) continue;
        cout << "2" << endl;
        if( XBW( preIssue[i].dest, i-1 ) ) continue;
        cout << "3" << endl;
        if( preIssue[i].opcode == 35 || preIssue[i].opcode == 43 ) { //35 = lw 43 = sw
          if( preMem[0].instr == "" ) {
            preMem[0] = preIssue[i];
            preIssue[i] = memItem();
          } else if( preMem[1].instr == "" ) {
            preMem[1] = preIssue[i];
            preIssue[i] = memItem();
          } else continue;
        }

      }
    }
    void PRINT(){

      string C("--------------------\ncycle:");
      C+= to_string(cycle) + "\t" +to_string(fetch);
      C+= "\n\nPre-Issue Buffer:";
      for( int i = 0; i < 4; i++){
        C += "\n\tEntry " + to_string(i) + ":\t" + preIssue[i].instr;
      }
      C+= "\nPre_ALU Queue:";
       for( int i = 0; i < 2; i++){
        C += "\n\tEntry " + to_string(i) + ":\t" + preALU[i].instr;
      }
      C+= "\nPost_ALU Queue:\n\tEntry 0:\t" + postALU.instr;

       C+= "\nPre_MEM Queue:";
       for( int i = 0; i < 2; i++){
        C += "\n\tEntry " + to_string(i) + ":\t" + preMem[i].instr;
      }
      C+= "\nPost_MEM Queue:\n\tEntry 0:\t" + postMem.instr;

      C +="\n\nRegisters:";
      for( int i = 0; i < 32; i++){
        if( i % 8 ==  0 ){
          C+="\nr";
          string num = to_string(i);
          if( i < 10) num.insert(0,"0");
          C += num + ":";
        }
        C+= "\t" + to_string(R[i] );
      }

      C += "\n\ndata:";
      for( int i = firstMem; i <= lastMem; i+=4){
        if( (i-firstMem) % 32 == 0 ){
          C += "\n" + to_string(i) + ":";
        }
        C+= "\t" + to_string(MEM[i].I);
      }
      cout << C<<endl;

    }
  };
  processor P(MEM, firstMem, lastMem);
  seenBreak = false;
  while( true ){
    
    if( P.cycle >2 ) exit(0);

    //P.WB();
    //P.MEM();
    //P.ALU();
    P.ISSUE();
    P.IF();
    P.PRINT();
    //if( seenBreak ) break;
    P.cycle++;
  }


}


    /*string outline;
    fetch = PC;
    M = MEM[PC];
    
      if( M.I == 0 ) {  //NOP
      } else if( M.opcode == 8 ){  // ADDI
        R[M.rt] = R[M.rs]+M.imm;
      } else if( M.opcode == 43 ){ //SW
        MEM[ R[M.rs] + M.imm].I = R[M.rt];
      } else if( M.opcode == 35 ){ //LW
        R[M.rt] = MEM[R[M.rs] + M.imm].I;
      } else if( M.opcode == 1 ){  // BLTZ
        if( R[M.rs] < 0)
          PC += M.boffset;
      } else if( M.opcode == 0 && M.func == 0){  // SLL
        R[M.rd] = R[M.rt] << M.shamt;
      } else if( M.opcode == 0 && M.func == 2){  // SRL
        R[M.rd] = R[M.rt] >> M.shamt;
      } else if( M.opcode == 0 && M.func == 34){  // SUB
        R[M.rd] = R[M.rs] - R[M.rt];
      } else if( M.opcode == 28 && M.func == 2){  // MUL
        R[M.rd] = R[M.rs] * R[M.rt];
        //outline = "\tMUL\tR" + to_string(rd) + ", R" + to_string(rs) + ", R" + to_string(rt);
        //instr += to_string(address) + outline;
      } else if( M.opcode == 0 && M.func == 10){  // MOVZ
        if( R[M.rt] == 0 ) R[M.rd] = R[M.rs];
        //outline = "\tMOVZ\tR" + to_string(rd) + ", R" + to_string(rs) + ", R" + to_string(rt);
        //instr += to_string(address) + outline;
      }  else if( M.opcode == 2 ){  // J
        PC = M.jtarget-4;
      }  else if( M.opcode == 0 && M.func == 32){  // ADD
        R[M.rd] = R[M.rs] + R[M.rt];
      }  else if( M.opcode == 0 && M.func == 13 ){  // BREAK
        seenBreak = true;
      } else if( M.opcode == 0 && M.func == 8){  // JR
        PC = R[M.rs]-4;
      } 
      else {
        cerr << "Unkown opcode: " << M.opcode << endl;
        exit( 0 );
      }
*/
