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
    int I = 0, opcode = 0, rs = 0, rt = 0, rd = 0, shamt = 0, func = 0, imm = 0, jtarget = 0, boffset = 0;
    int src1 = 0, src2 = 0, dest = -2;
    unsigned int UI = 0;
    string binstr = "", binstrNoSpace = "", instr = "", outline = "";
  };

  unordered_map<int, memItem> MEM;
  int firstMem=0, lastMem=0;
  char buffer[4];
  int I;
  char * iPtr;
  iPtr = (char*)(void*) &I;
  int FD = open(argv[2], O_RDONLY);
  string outdisstr = string(argv[4]) + "_dis.txt";
  ofstream outdis(outdisstr);
  int address = 96;
  int amt = 4;
  bool seenBreak = false;

  while (amt != 0) {
    amt = read(FD, buffer, 4);
    if (amt == 4) {
      iPtr[0] = buffer[3];
      iPtr[1] = buffer[2];
      iPtr[2] = buffer[1];
      iPtr[3] = buffer[0];
      memItem M;
      M.I= I;
      M.UI = static_cast<unsigned int>(I);
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
      if (!seenBreak) {
        outline = M.binstr + "\t";
        if (M.I == 0) {
          instr += "\tNOP";
          outline += to_string(address) + instr;
          M.src1 = 0; M.src2 = 0; M.dest = 0;
        } else if (M.opcode == 8) {
          instr += "\tADDI\tR" + to_string(M.rt) + ", R" + to_string(M.rs) + ", #" + to_string(M.imm);
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rs; M.dest = M.rt;
        } else if (M.opcode == 43) {
          instr += "\tSW\tR" + to_string(M.rt) + ", " + to_string(M.imm) + "(R" + to_string(M.rs) + ")";
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rt; M.dest = -1;
        } else if (M.opcode == 35) {
          instr += "\tLW\tR" + to_string(M.rt) + ", " + to_string(M.imm) + "(R" + to_string(M.rs) + ")";
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rs; M.dest = M.rt;
        } else if (M.opcode == 1) {
          instr += "\tBLTZ\tR" + to_string(M.rs) + ", #" + to_string(M.boffset);
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rs; M.dest = -1;
        } else if (M.opcode == 0 && M.func == 0) {
          instr += "\tSLL\tR" + to_string(M.rd) + ", R" + to_string(M.rt) + ", #" + to_string(M.shamt);
          outline += to_string(address) + instr;
          M.src1 = M.rt; M.src2 = M.rt; M.dest = M.rd;
        } else if (M.opcode == 0 && M.func == 2) {
          instr += "\tSRL\tR" + to_string(M.rd) + ", R" + to_string(M.rt) + ", #" + to_string(M.shamt);
          outline += to_string(address) + instr;
          M.src1 = M.rt; M.src2 = M.rt; M.dest = M.rd;
        } else if (M.opcode == 0 && M.func == 34) {
          instr += "\tSUB\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rt; M.dest = M.rd;
        } else if (M.opcode == 28 && M.func == 2) {
          instr += "\tMUL\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rt; M.dest = M.rd;
        } else if (M.opcode == 0 && M.func == 10) {
          instr += "\tMOVZ\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rt; M.dest = M.rd;
        } else if (M.opcode == 2) {
          instr += "\tJ\t#" + to_string(M.jtarget);
          outline += to_string(address) + instr;
          M.src1 = -1; M.src2 = -1; M.dest = 0;
        } else if (M.opcode == 0 && M.func == 32) {
          instr += "\tADD\tR" + to_string(M.rd) + ", R" + to_string(M.rs) + ", R" + to_string(M.rt);
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rt; M.dest = M.rd;
        } else if (M.opcode == 0 && M.func == 13) {
          instr += "\tBREAK\t";
          outline += to_string(address) + instr;
          seenBreak = true;
          firstMem = address + 4;
          M.src1 = -1; M.src2 = -1; M.dest = 0;
        } else if (M.opcode == 0 && M.func == 8) {
          instr += "\tJR\tR" + to_string(M.rs);
          outline += to_string(address) + instr;
          M.src1 = M.rs; M.src2 = M.rs; M.dest = 0;
        } else {
          cerr << "Unknown opcode: " << M.opcode << endl;
          exit(0);
        }
        M.outline = outline;
      } else {
        instr = M.binstrNoSpace + "\t" + to_string(address) + " " + to_string(M.I);
        M.outline = instr;
      }
      M.instr = instr;
      MEM[address] = M;
      outdis << M.outline << endl;
      lastMem = address;
      address += 4;
    }
  }
  outdis.close();

  string outsimstr = string(argv[4]) + "_sim.txt";
  ofstream outsim(outsimstr);

  struct processor {
    unordered_map<int, memItem> MEM;
    int PC=96, R[32]={0}, cycle=1, fetch, firstMem, lastMem;
    memItem preIssue[4], preALU[2], preMem[2], postALU, postMem;

    processor(unordered_map<int, memItem> MEM, int _firstMem, int _lastMem):
      MEM(MEM), firstMem(_firstMem), lastMem(_lastMem) {}

    void IF() {
      fetch = PC;
      for (int i = 0; i < 2; i++) {
        if (preIssue[3].instr != "") break;
        memItem I = MEM[PC];
        int j = 0;
        for (; j < 4; j++)
          if (preIssue[j].instr == "") break;
        preIssue[j] = I;
        PC += 4;
      }
    }

    bool XBW(int r, int start) {
      for (int i = start; i >= 0; i--)
        if (preIssue[i].dest == r) return true;
      if (preMem[1].instr != "" && preMem[1].dest == r) return true;
      if (preALU[0].dest == r) return true;
      if (postALU.dest == r) return true;
      if (postMem.dest == r) return true;
      return false;
    }

    void ISSUE() {
      for (int i = 0; i < 4; i++) {
        if (preIssue[i].instr == "") continue;
        if (XBW(preIssue[i].src1, i-1)) continue;
        if (XBW(preIssue[i].src2, i-1)) continue;
        if (XBW(preIssue[i].dest, i-1)) continue;
        if (preIssue[i].opcode == 35 || preIssue[i].opcode == 43) {
          if (preMem[0].instr == "") {
            preMem[0] = preIssue[i];
            preIssue[i] = memItem();
          } else if (preMem[1].instr == "") {
            preMem[1] = preIssue[i];
            preIssue[i] = memItem();
          }
        }
      }
    }

    void ALU() {
      if (preALU[0].instr != "") {
        memItem I = preALU[0];
        if (I.opcode == 8) postALU = I, postALU.I = R[I.rs] + I.imm;
        else if (I.opcode == 0 && I.func == 32) postALU = I, postALU.I = R[I.rs] + R[I.rt];
        else if (I.opcode == 0 && I.func == 34) postALU = I, postALU.I = R[I.rs] - R[I.rt];
        else if (I.opcode == 28 && I.func == 2) postALU = I, postALU.I = R[I.rs] * R[I.rt];
        else if (I.opcode == 0 && I.func == 10) { postALU = I; if (R[I.rt] == 0) postALU.I = R[I.rs]; else postALU.dest = -1; }
        else if (I.opcode == 0 && I.func == 0) postALU = I, postALU.I = R[I.rt] << I.shamt;
        else if (I.opcode == 0 && I.func == 2) postALU = I, postALU.I = R[I.rt] >> I.shamt;
        preALU[0] = memItem();
      }
    }

    void MEM1() {
      if (preMem[0].instr != "") {
        memItem I = preMem[0];
        if (I.opcode == 35) {
          postMem = I;
          int addr = R[I.rs] + I.imm;
          postMem.I = MEM[addr].I;
        } else if (I.opcode == 43) {
          int addr = R[I.rs] + I.imm;
          MEM[addr].I = R[I.rt];
          postMem = memItem();
        }
        preMem[0] = preMem[1];
        preMem[1] = memItem();
      }
    }

    void WB() {
      if (postALU.dest >= 0) R[postALU.dest] = postALU.I;
      if (postMem.dest >= 0) R[postMem.dest] = postMem.I;
      postALU = memItem();
      postMem = memItem();
    }

    void PRINT() {
      string C("--------------------\ncycle:");
      C += to_string(cycle) + "\t" + to_string(fetch);
      C += "\n\nPre-Issue Buffer:";
      for (int i = 0; i < 4; i++) C += "\n\tEntry " + to_string(i) + ":\t" + preIssue[i].instr;
      C += "\nPre_ALU Queue:";
      for (int i = 0; i < 2; i++) C += "\n\tEntry " + to_string(i) + ":\t" + preALU[i].instr;
      C += "\nPost_ALU Queue:\n\tEntry 0:\t" + postALU.instr;
      C += "\nPre_MEM Queue:";
      for (int i = 0; i < 2; i++) C += "\n\tEntry " + to_string(i) + ":\t" + preMem[i].instr;
      C += "\nPost_MEM Queue:\n\tEntry 0:\t" + postMem.instr;
      C += "\n\nRegisters:";
      for (int i = 0; i < 32; i++) {
        if (i % 8 == 0) {
          C += "\nr";
          string num = to_string(i);
          if (i < 10) num.insert(0,"0");
          C += num + ":";
        }
        C += "\t" + to_string(R[i]);
      }
      C += "\n\ndata:";
      for (int i = firstMem; i <= lastMem; i += 4) {
        if ((i - firstMem) % 32 == 0) C += "\n" + to_string(i) + ":";
        C += "\t" + to_string(MEM[i].I);
      }
      cout << C << endl;
    }
  };
processor P(MEM, firstMem, lastMem);
while (true) {
  if (P.MEM[P.PC].opcode == 0 && P.MEM[P.PC].func == 13) break;
  P.WB();
  P.MEM1();
  P.ALU();
  P.ISSUE();
  P.IF();
  P.PRINT();
  P.cycle++;
 }
P.PRINT();
return 0;
}

