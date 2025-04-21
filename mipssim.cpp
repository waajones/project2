#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <string>
#include <bitset>
#include <fstream>
#include <unordered_map>

using namespace std;

int main(int argc, char *argv[]){

    struct memItem {
        int I{0}, opcode{0}, rs{0}, rt{0}, rd{0}, shamt{0}, func{0}, imm{0}, jtarget{0}, boffset{0}; 
        int src1{0}, src2{0}, dest{-2};
        unsigned int UI{0};
        string binstr; 
        string binstrNoSpace;
        string instr, outline;
        memItem() = default;
        };

    unordered_map<int, memItem> MEM;
    int firstMem=0, lastMem=0;
    char buffer[4];
    int I;
    char * iPtr;
    iPtr = (char*)(void*) &I;
    int FD = open(argv[2], O_RDONLY);
    string outdisstr = string(argv[4] ) + "_dis.txt";
    ofstream outdis( outdisstr );
    int address = 96;
    int amt = 4;
    bool seenBreak = false;
    while( amt != 0 )
    {
        amt = read(FD, buffer, 4);
        if(amt == 4){
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
            if(! seenBreak ) {  
                outline = M.binstr + "\t";
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
              } else
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

    //sim start

    string outsimstr = string(argv[4]) + "_pipeline.txt";

    struct processor {
        bool seenBreak = false;
        unordered_map<int, memItem> MEM;
        int firstMem;
        int lastMem;
        int PC = 96;
        int R[32] = {0};
        int cycle = 1;
        int fetch;
        memItem preIssue[4], preALU[2], preMem[2], postALU, postMem;
        ofstream outsim;

        processor(unordered_map<int, memItem> mem, int _firstMem, int _lastMem, const string &filename): MEM(mem), firstMem(_firstMem), lastMem(_lastMem), outsim(filename) {}

            void IF() {
                static bool waitingOnBranch = false;
                static bool branchTaken = false;
                fetch = PC;

                // Check if we should branch
                if (waitingOnBranch) {
                    // Only evaluate branch when all prior instructions have completed
                    if (preIssue[0].instr == "" && preIssue[1].instr == "" && 
                        preIssue[2].instr == "" && preIssue[3].instr == "" &&
                        preMem[0].instr == "" && preMem[1].instr == "" &&
                        postMem.instr == "") {
                        
                        if (R[postMem.src1] < 0) {  // BLTZ
                            PC = PC + postMem.boffset;
                            branchTaken = true;
                        } else {
                            PC = PC + 4;
                        }
                        waitingOnBranch = false;
                        postMem = memItem();
                    }
                    return;
                }

                // If we have already branched
                if (branchTaken) return;

                for (int i = 0; i < 2; i++) {
                    if (preIssue[3].instr != "") break;
                    if (MEM.find(PC) == MEM.end()) break;
        
                    memItem I = MEM[PC];
                    if (I.opcode == 0 && I.func == 13) {
                        seenBreak = true;
                        break;
                    }
        
                    if (I.opcode == 1) { // BLTZ
                        waitingOnBranch = true;
                        postMem = I;
                        break;
                    }
        
                    int j = 0;
                    for (; j < 4; j++) {
                        if (preIssue[j].instr == "") break;
                    }
                    if (j == 4) break;
                    
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
                
                if (preIssue[i].opcode == 35 || preIssue[i].opcode == 43) { // LW or SW
                    // Check for hazards
                    if (XBW(preIssue[i].src1, i - 1)) break;
                    if (XBW(preIssue[i].src2, i - 1)) break;
                    if (XBW(preIssue[i].dest, i - 1)) break;
                    
                    // Find empty preMem slot
                    int slot = -1;
                    if (preMem[0].instr == "") slot = 0;
                    else if (preMem[1].instr == "") slot = 1;
                    if (slot == -1) break;  
                    
                    // Mov
                    preMem[slot] = preIssue[i];
                    
                    // Shift up
                    for (int j = i; j < 3; j++) {
                        preIssue[j] = preIssue[j + 1];
                    }
                    preIssue[3] = memItem();
                    i--; // Recheck current position
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
                if (I.opcode == 35) { // LW
                    postMem = I;
                    int addr = R[I.rs] + I.imm;
                    postMem.I = MEM[addr].I;
                } else if (I.opcode == 43) { // SW
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
            string C("--------------------\nCycle:");
            C += to_string(cycle) + "\tPC:" + to_string(fetch);
            C += "\n\nPre-Issue Buffer:";
            for (int i = 0; i < 4; i++) {
                C += "\n\tEntry " + to_string(i) + ":\t";
                if (preIssue[i].instr != "") C += "[" + preIssue[i].instr.substr(1) + "]";
            }
            C += "\nPre_ALU Queue:";
            for (int i = 0; i < 2; i++) {
                C += "\n\tEntry " + to_string(i) + ":\t";
                if (preALU[i].instr != "") C += "[" + preALU[i].instr.substr(1) + "]";
            }
            C += "\nPost_ALU Queue:\n\tEntry 0:\t";
            if (postALU.instr != "") C += "[" + postALU.instr.substr(1) + "]";
            C += "\nPre_MEM Queue:";
            for (int i = 0; i < 2; i++) {
                C += "\n\tEntry " + to_string(i) + ":\t";
                if (preMem[i].instr != "") C += "[" + preMem[i].instr.substr(1) + "]";
            }
            C += "\nPost_MEM Queue:\n\tEntry 0:\t";
            if (postMem.instr != "") C += "[" + postMem.instr.substr(1) + "]";

            C += "\n\nRegisters";
            for (int i = 0; i < 32; i++) {
                if (i % 8 == 0) {
                    C += "\nR";
                    string num = to_string(i);
                    if (i < 10) num.insert(0, "0");
                    C += num + ":";
                }
                C += "\t" + to_string(R[i]);
            }

            C += "\n\nData";
            for (int i = firstMem; i <= lastMem; i += 4) {
                if ((i - firstMem) % 32 == 0) {
                    C += "\n" + to_string(i) + ":";
                }
                C += "\t" + to_string(MEM[i].I);
            }
            C += "\n";
            outsim << C << endl;
            cout << C << endl;
        }
    };


    processor P(MEM, firstMem, lastMem,outsimstr);
      seenBreak = false;
      while(true){

        if(P.cycle >15) exit(0);

        P.WB();
        P.MEM1();
        P.ALU();
        P.ISSUE();
        P.IF();
        P.PRINT();
        if( seenBreak ) break;
        P.cycle++;
      }

    return 0;
}
