//Project 3 Tomasulo
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <bitset>
#include <map>

using namespace std;

struct instruction {
	string opcode;
	int rd;
	int rs1;
	int rs2;
	int imm12;
};

struct contentRS {
	string operand;
	string rs1;
	string rs2;
};

struct ALU {
	int cycle;
	int RS;
	bool empty; // 0 代表空的，1 代表有東西
};

bool RSempty[5] = {};

vector< instruction > ISA;

vector< int > RF(6);
vector< string > RAT(6);
vector< contentRS > RS(5);
ALU bufferADD;
ALU bufferMUL;

int currentCycle = 1; // 目前在第幾個 cycle
int ADDSUB, MUL, DIV; // 分別在 ALU 執行的 cycle time
bool changedCycle; // 是否為有變化的cycle

void loadTest()
{
	fstream test("test.txt", ios::in);
	for (int i = 0; test.peek() != EOF; i++)
	{
		string input, operation;
		stringstream ss;
		string temp1, temp2, temp3; // 用來暫存 rd, rs1, rs2
		instruction init;
		getline(test, input);
		ISA.push_back(init);
		ss << input;
		ss >> operation;

		getline(ss, temp1, ',');
		getline(ss, temp2, ',');
		getline(ss, temp3);
		ss.str("");
		ss.clear();

		temp1 = temp1.substr(2, temp1.size() - 2);
		temp2 = temp2.substr(2, temp2.size() - 2);

		ISA[i].opcode = operation;
		ISA[i].rd = stoi(temp1);
		ISA[i].rs1 = stoi(temp2);

		if (operation == "ADD" || operation == "SUB" || operation == "MUL" || operation == "DIV")
		{
			temp3 = temp3.substr(2, temp3.size() - 2);
			ISA[i].rs2 = stoi(temp3);
		}

		else if (operation == "ADDI")
		{
			temp3 = temp3.substr(1, temp3.size() - 1);
			ISA[i].imm12 = stoi(temp3);
		}
	}
}

void printCycleStatus()
{
	cout << "Cycle : " << currentCycle << endl << endl;

	cout << "  ____RF____" << endl;
	for (int i = 1; i < RF.size(); ++i)
		cout << setw(3) << "F" << i << " | " << setw(3) << RF[i] << " |" << endl;
	cout << "  ----------" << endl << endl;

	cout << "  ____RAT____" << endl;
	for (int i = 1; i < RAT.size(); ++i)
		cout << setw(3) << "F" << i << " | " << setw(4) << RAT[i] << " |" << endl;
	cout << "  ----------" << endl << endl;

	cout << " ____________RS____________" << endl;
	for (int i = 0; i < 3; ++i)
	{
		cout << setw(3) << "RS" << i + 1 << " | " << setw(4) << RS[i].operand << " | " << setw(4) << RS[i].rs1 << " | " << setw(4) << RS[i].rs2 << " | " << endl;

		if (i == 2)
		{
			cout << " --------------------------" << endl;
			cout << " BUFFER: ";
			if (!bufferADD.empty)
				cout << "empty" << endl << endl;
			else
				cout << "(RS" << bufferADD.RS << ") " << RS[bufferADD.RS - 1].rs1 << " " << RS[bufferADD.RS - 1].operand << " " << RS[bufferADD.RS - 1].rs2 << endl << endl;
		}
	}

	cout << " __________________________" << endl;

	for (int i = 3; i < 5; ++i)
	{
		cout << setw(3) << "RS" << i + 1 << " | " << setw(4) << RS[i].operand << " | " << setw(4) << RS[i].rs1 << " | " << setw(4) << RS[i].rs2 << " | " << endl;

		if (i == 4)
		{
			cout << " --------------------------" << endl;
			cout << " BUFFER: ";
			if (!bufferMUL.empty)
				cout << "empty" << endl << endl;
			else
				cout << "(RS" << bufferMUL.RS << ") " << RS[bufferMUL.RS - 1].rs1 << " " << RS[bufferMUL.RS - 1].operand << " " << RS[bufferMUL.RS - 1].rs2 << endl << endl;
		}
	}
}

void Issue()
{
	if (!ISA.empty())
	{
		if (ISA[0].opcode == "ADD" || ISA[0].opcode == "SUB" || ISA[0].opcode == "ADDI")
		{
			for (int i = 0; i < 3; i++) // ADD's ALU
			{
				if (!RSempty[i]) // RS[0] ~ RS[2] 有空間
				{
					if (ISA[0].opcode == "ADD" || ISA[0].opcode == "ADDI") // operand 為 +
						RS[i].operand = "+";
					else                                                   // operand 為 -
						RS[i].operand = "-";

					if (RAT[ISA[0].rs1] == "")                             // RAT裡沒東西
						RS[i].rs1 = to_string(RF[ISA[0].rs1]);
					else                                                   // RAT有東西
						RS[i].rs1 = RAT[ISA[0].rs1];

					if (ISA[0].opcode == "ADDI")                           // I-type imm 的值直接放到 rs2 裡
						RS[i].rs2 = to_string(ISA[0].imm12);
					else if (RAT[ISA[0].rs2] == "")                        // RAT裡沒東西
						RS[i].rs2 = to_string(RF[ISA[0].rs2]);
					else                                                   // RAT有東西
						RS[i].rs2 = RAT[ISA[0].rs2];

					RAT[ISA[0].rd] = "RS" + to_string(i + 1);              // 更新 RAT 的值

					RSempty[i] = 1;
					changedCycle = 1;
					ISA.erase(ISA.begin()); // 把 Issue 進來的 instruction 刪除
					break;
				}
			}
		}
		else
		{
			for (int i = 3; i < 5; i++) // MUL's ALU
			{
				if (!RSempty[i]) // RS[3] ~ RS[4] 有空間
				{
					if (ISA[0].opcode == "MUL")                            // operand 為 *
						RS[i].operand = "*";
					else                                                   // operand 為 /
						RS[i].operand = "/";

					if (RAT[ISA[0].rs1] == "")                             // RAT裡沒東西
						RS[i].rs1 = to_string(RF[ISA[0].rs1]);
					else                                                   // RAT有東西
						RS[i].rs1 = RAT[ISA[0].rs1];

					if (RAT[ISA[0].rs2] == "")                             // RAT裡沒東西
						RS[i].rs2 = to_string(RF[ISA[0].rs2]);
					else                                                   // RAT有東西
						RS[i].rs2 = RAT[ISA[0].rs2];

					RAT[ISA[0].rd] = "RS" + to_string(i + 1);              // 更新 RAT 的值

					RSempty[i] = 1;
					changedCycle = 1;
					ISA.erase(ISA.begin()); // 把 Issue 進來的 instruction 刪除
					break;
				}
			}
		}
	}
}

void Dispatch()
{
	if (!bufferADD.empty)
	{
		for (int i = 0; i < 3; i++)
		{
			if (RSempty[i])
			{
				if (RS[i].rs1[0] != 'R' && RS[i].rs2[0] != 'R')
				{
					bufferADD.RS = i + 1;
					bufferADD.empty = 1;
					bufferADD.cycle = currentCycle + ADDSUB;

					changedCycle = 1;
				}
			}
		}
	}
	if (!bufferMUL.empty)
	{
		for (int i = 3; i < 5; i++)
		{
			if (RSempty[i])
			{
				if (RS[i].rs1[0] != 'R' && RS[i].rs2[0] != 'R')
				{
					if (RS[i].operand == "*")
					{
						bufferMUL.RS = i + 1;
						bufferMUL.empty = 1;
						bufferMUL.cycle = currentCycle + MUL;
					}
					else
					{
						bufferMUL.RS = i + 1;
						bufferMUL.empty = 1;
						bufferMUL.cycle = currentCycle + DIV;
					}

					changedCycle = 1;
				}
			}
		}
	}
}

void WriteResult(ALU& buffer)
{
	int result;
	if (currentCycle == buffer.cycle)
	{
		// 先計算 buffer 裡的值
		if (RS[buffer.RS - 1].operand == "+")
			result = stoi(RS[buffer.RS - 1].rs1) + stoi(RS[buffer.RS - 1].rs2);
		else if (RS[buffer.RS - 1].operand == "-")
			result = stoi(RS[buffer.RS - 1].rs1) - stoi(RS[buffer.RS - 1].rs2);
		else if (RS[buffer.RS - 1].operand == "*")
			result = stoi(RS[buffer.RS - 1].rs1) * stoi(RS[buffer.RS - 1].rs2);
		else
			result = stoi(RS[buffer.RS - 1].rs1) / stoi(RS[buffer.RS - 1].rs2);
		buffer.empty = 0;

		// 更新 RAT 和 RF 的值
		for (int i = 1; i <= 5; i++)
			if (RAT[i] == "RS" + to_string(buffer.RS))
			{
				RAT[i] = "";
				RF[i] = result;
			}

		// Capture
		for (int i = 0; i < 5; i++)
		{
			if (RS[i].rs1 == "RS" + to_string(buffer.RS))
				RS[i].rs1 = to_string(result);
			if (RS[i].rs2 == "RS" + to_string(buffer.RS))
				RS[i].rs2 = to_string(result);
		}

		// free RS
		RS[buffer.RS - 1].operand = "";
		RS[buffer.RS - 1].rs1 = "";
		RS[buffer.RS - 1].rs2 = "";
		RSempty[buffer.RS - 1] = 0;

		changedCycle = 1;
	}
}

int main() {
	loadTest();
	cout << "Please input cycle time of ADD/SUB, MUL, DIV respectivity." << endl;
	cin >> ADDSUB >> MUL >> DIV;
	
	for (int i = 1; i <= 5; i++) // 設定 RF 裡的值
		RF[i] = (i - 1) * 2;

	while (true)
	{
		if (ISA.empty()) //判斷 ISA 及 RS 是否皆為空
		{
			int i;
			for (i = 0; i < 5; i++)
				if (RSempty[i] != 0)
					break;
			if (i == 5)
				break;
		}

		changedCycle = 0;

		WriteResult(bufferADD);
		WriteResult(bufferMUL);
		Dispatch();
		Issue();

		if (changedCycle)
			printCycleStatus();

		currentCycle++;
	}
}
