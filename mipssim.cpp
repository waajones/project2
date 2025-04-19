#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <string>
#include <bitset>
#include <fstream>
#include <unordered_map>
#include <queue>

using namespace std;

struct memItem {
  int I, opcode, rs, rt, rd, shamt, func, imm, jtarget, boffset; 
  int src1, src2, dest;
  unsigned int UI;
  string binstr; 
  string binstrNoSpace;
  string instr, outline;
  int cycle;
  memItem():I(0), opcode(0), rs(0), rt(0), rd(0), shamt(0), func(0), imm(0), jtarget(0), boffset(0), src1(0), src2(0), dest(-2), UI(0), cycle(0) { }
};

class Simulator {
private:
  unordered_map<int, memItem> MEM; 
  int firstMem, lastMem;
  int PC;
  int R[32];
  int cycle;
  memItem preIssue[4];
  memItem preALU[2];
  memItem preMem[2];
  memItem postALU;
  memItem postMem;
  bool seenBreak;
  ofstream& out;

  bool hasWAWHazard(memItem& instr, int start) {
    if(instr.dest < 0) return false;
    for(int i = 0; i < start; i++) {
      if(preIssue[i].dest == instr.dest) return true;
    }
    if(preALU[0].dest == instr.dest || preALU[1].dest == instr.dest) return true;
    if(preMem[0].dest == instr.dest || preMem[1].dest == instr.dest) return true;
    if(postALU.dest == instr.dest || postMem.dest == instr.dest) return true;
    return false;
  }

  bool hasRAWHazard(memItem& instr, int start) {
    if(instr.src1 < 0) return false;
    for(int i = 0; i < start; i++) {
      if(preIssue[i].dest == instr.src1 || preIssue[i].dest == instr.src2) return true;
    }
    if(preALU[0].dest == instr.src1 || preALU[1].dest == instr.src1) return true;
    if(preMem[0].dest == instr.src1 || preMem[1].dest == instr.src1) return true;
    if(postALU.dest == instr.src1 || postMem.dest == instr.src1) return true;
    return false;
  }

  bool hasWARHazard(memItem& instr, int start) {
    if(instr.dest < 0) return false;
    for(int i = 0; i < start; i++) {
      if(preIssue[i].src1 == instr.dest || preIssue[i].src2 == instr.dest) return true;
    }
    return false;
  }

  void writeBack() {
    if(postALU.dest >= 0) {
      R[postALU.dest] = postALU.I;
      postALU.dest = -2;
    }
    if(postMem.dest >= 0) {
      R[postMem.dest] = postMem.I;
      postMem.dest = -2;
    }
  }

  void execute() {
    // Move from preALU to postALU
    if(preALU[0].dest >= 0) {
      if(preALU[0].opcode == 0 && preALU[0].func == 32) { // ADD
        preALU[0].I = R[preALU[0].src1] + R[preALU[0].src2];
      } else if(preALU[0].opcode == 8) { // ADDI
        preALU[0].I = R[preALU[0].src1] + preALU[0].imm;
      }
      // Add other ALU operations here
      postALU = preALU[0];
      preALU[0] = preALU[1];
      preALU[1] = memItem();
    }
    
    // Handle memory operations
    if(preMem[0].dest >= 0) {
      if(preMem[0].opcode == 35) { // LW
        preMem[0].I = MEM[R[preMem[0].src1] + preMem[0].imm].I;
        postMem = preMem[0];
      } else if(preMem[0].opcode == 43) { // SW
        MEM[R[preMem[0].src1] + preMem[0].imm].I = R[preMem[0].src2];
      }
      preMem[0] = preMem[1];
      preMem[1] = memItem();
    }
  }

  void issue() {
    int issued = 0;
    for(int i = 0; i < 4 && issued < 2; i++) {
      if(preIssue[i].dest == -2) continue;
      
      if(hasWAWHazard(preIssue[i], i) || 
         hasRAWHazard(preIssue[i], i) ||
         hasWARHazard(preIssue[i], i)) continue;

      // Issue to ALU
      if(preIssue[i].opcode == 0 || preIssue[i].opcode == 8) {
        if(preALU[1].dest == -2) {
          if(preALU[0].dest == -2) preALU[0] = preIssue[i];
          else preALU[1] = preIssue[i];
          preIssue[i] = memItem();
          issued++;
        }
      }
      // Issue to MEM
      else if(preIssue[i].opcode == 35 || preIssue[i].opcode == 43) {
        if(preMem[1].dest == -2) {
          if(preMem[0].dest == -2) preMem[0] = preIssue[i];
          else preMem[1] = preIssue[i];
          preIssue[i] = memItem();
          issued++;
        }
      }
    }
    
    // Compact preIssue buffer
    for(int i = 0; i < 3; i++) {
      if(preIssue[i].dest == -2) {
        preIssue[i] = preIssue[i+1];
        preIssue[i+1] = memItem();
      }
    }
  }

  void fetch() {
    if(seenBreak) return;
    
    int fetched = 0;
    while(fetched < 2 && preIssue[3].dest == -2) {
      memItem& M = MEM[PC];
      
      if(M.opcode == 0 && M.func == 13) { // BREAK
        seenBreak = true;
        break;
      }
      
      // Skip NOPs and branch instructions
      if(M.I != 0 && M.opcode != 1 && M.opcode != 2) {
        for(int i = 0; i < 4; i++) {
          if(preIssue[i].dest == -2) {
            preIssue[i] = M;
            break;
          }
        }
      }
      
      PC += 4;
      fetched++;
    }
  }

  void printState() {
    out << "--------------------\n";
    out << "Cycle:" << cycle << "\tPC:" << PC << "\n\n";
    
    out << "Pre-Issue Buffer:\n";
    for(int i = 0; i < 4; i++) {
      out << "\tEntry " << i << ":\t" << preIssue[i].instr << "\n";
    }
    
    out << "Pre-ALU Queue:\n";
    for(int i = 0; i < 2; i++) {
      out << "\tEntry " << i << ":\t" << preALU[i].instr << "\n";  
    }
    
    out << "Post-ALU Queue:\n";
    out << "\tEntry 0:\t" << postALU.instr << "\n";
    
    out << "Pre-MEM Queue:\n";
    for(int i = 0; i < 2; i++) {
      out << "\tEntry " << i << ":\t" << preMem[i].instr << "\n";
    }
    
    out << "Post-MEM Queue:\n";
    out << "\tEntry 0:\t" << postMem.instr << "\n";
    
    out << "\nRegisters\n";
    for(int i = 0; i < 32; i += 8) {
      out << "R" << (i<10?"0":"") << i << ":\t";
      for(int j = 0; j < 8; j++) {
        out << R[i+j] << "\t";
      }
      out << "\n";
    }
    
    out << "\nData\n";
    for(int addr = firstMem; addr <= lastMem; addr += 4) {
      out << addr << ":\t" << MEM[addr].I << "\t";
      if((addr - firstMem) % 32 == 28) out << "\n";
    }
    out << "\n";
  }

public:
  Simulator(unordered_map<int, memItem>& memory, int fMem, int lMem, ofstream& output) 
    : MEM(memory), firstMem(fMem), lastMem(lMem), PC(96), cycle(1), seenBreak(false), out(output) {
    for(int i = 0; i < 32; i++) R[i] = 0;
  }

  void run() {
    while(!seenBreak || 
          preIssue[0].dest != -2 || preALU[0].dest != -2 || 
          preMem[0].dest != -2 || postALU.dest != -2 || 
          postMem.dest != -2) {
      writeBack();
      execute();
      issue();
      fetch();
      printState();
      cycle++;
    }
  }
};

int main(int argc, char *argv[]) {
  unordered_map<int, memItem> MEM;
  int firstMem = 0, lastMem = 0;
  char buffer[4];
  int I;
  char* iPtr = (char*)(void*)&I;
  int FD = open(argv[2], O_RDONLY);
  string outdisstr = string(argv[4]) + "_dis.txt";
  string outsimstr = string(argv[4]) + "_pipelineNC.txt";
  ofstream outdis(outdisstr);
  ofstream outsim(outsimstr);
  int address = 96;
  int amt = 4;
  bool seenBreak = false;

  // Read and parse instructions
  while(amt != 0) {
    amt = read(FD, buffer, 4);
    if(amt == 4) {
      iPtr[0] = buffer[3];
      iPtr[1] = buffer[2];
      iPtr[2] = buffer[1];
      iPtr[3] = buffer[0];
      
      memItem M;
      M.I = I;
      M.UI = static_cast<unsigned int>(I);
      M.opcode = M.UI >> 26;
      M.rs = M.UI << 6 >> 27;
      M.rt = M.UI << 11 >> 27;
      M.rd = M.UI << 16 >> 27;
      M.imm = M.I << 16 >> 16;
      M.shamt = M.UI << 21 >> 27;
      M.func = M.UI << 26 >> 26;
      M.jtarget = M.UI << 6 >> 4;
      M.boffset = M.I << 16 >> 14;
      M.binstr = bitset<32>(I).to_string();
      M.binstrNoSpace = M.binstr;
      M.binstr.insert(26," ").insert(21," ").insert(16," ").insert(11," ").insert(6," ");
      
      string instr;
      string outline = M.binstr + "\t";
      
      // Generate disassembly
      if(!seenBreak) {
        outline += to_string(address);
        if(M.I == 0) {
          instr += "\tNOP";
          M.src1 = M.src2 = M.dest = 0;
        } else if(M.opcode == 8) {
          instr += "\tADDI\tR" + to_string(M.rt) + ", R" + to_string(M.rs) + ", #" + to_string(M.imm);
          M.src1 = M.src2 = M.rs;
          M.dest = M.rt;
        } else if(M.opcode == 43) {
          instr += "\tSW\tR" + to_string(M.rt) + ", " + to_string(M.imm) + "(R" + to_string(M.rs) + ")";
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = -1;
        } else if(M.opcode == 35) {
          instr += "\tLW\tR" + to_string(M.rt) + ", " + to_string(M.imm) + "(R" + to_string(M.rs) + ")";
          M.src1 = M.src2 = M.rs;
          M.dest = M.rt;
        } else if(M.opcode == 1) {
          instr += "\tBLTZ\tR" + to_string(M.rs) + ", #" + to_string(M.boffset);
          M.src1 = M.src2 = M.rs;
          M.dest = -1;
        } else if(M.opcode == 0 && M.func == 0) {
          instr += "\tSLL\tR" + to_string(M.rd) + ", R" + to_string(M.rt) + ", #" + to_string(M.shamt);
          M.src1 = M.src2 = M.rt;
          M.dest = M.rd;
        } else if(M.opcode == 0 && M.func == 2) {
          instr += "\tSRL\tR" + to_string(M.rd) + ", R" + to_string(M.rt) + ", #" + to_string(M.shamt);
          M.src1 = M.src2 = M.rt;
          M.dest = M.rd;
        } else if(M.opcode == 0 && M.func == 34) {
          instr += "\tSUB\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = M.rd;
        } else if(M.opcode == 28 && M.func == 2) {
          instr += "\tMUL\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = M.rd;
        } else if(M.opcode == 0 && M.func == 10) {
          instr += "\tMOVZ\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = M.rd;
        } else if(M.opcode == 2) {
          instr += "\tJ\t#" + to_string(M.jtarget);
          M.src1 = M.src2 = -1;
          M.dest = -1;
        } else if(M.opcode == 0 && M.func == 32) {
          instr += "\tADD\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          M.src1 = M.rs;
          M.src2 = M.rt;
          M.dest = M.rd;
        } else if(M.opcode == 0 && M.func == 13) {
          instr += "\tBREAK";
          seenBreak = true;
          firstMem = address + 4;
          M.src1 = M.src2 = M.dest = -1;
        } else if(M.opcode == 0 && M.func == 8) {
          instr += "\tJR\tR" + to_string(M.rs);
          M.src1 = M.rs;
          M.src2 = -1;
          M.dest = -1;
        } else {
          cerr << "Unknown opcode: " << M.opcode << endl;
          exit(0);
        }
        
        M.instr = instr;
        outline += instr;
        outdis << outline << endl;
      } else {
        instr = M.binstrNoSpace + "\t" + to_string(address) + " " + to_string(M.I);
        M.instr = instr;
        outdis << instr << endl;
      }
      
      MEM[address] = M;
      lastMem = address;
      address += 4;
    }
  }
  outdis.close();

  // Run simulation
  Simulator sim(MEM, firstMem, lastMem, outsim);
  sim.run();
  outsim.close();

  return 0;
}
