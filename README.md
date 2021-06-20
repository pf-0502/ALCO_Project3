# ALCO_Project3
*Tomasulo*
## 專案說明
#### 給定一個assembly code，輸出所有**有變化**cycle  

*Input*：一段assembly code，使用者可指定ADD/SUB、MUL、DIV在ALU中需執行幾個cycle  

*Output*：輸出所有**有變化**cycle，包括目前cycle、RF、RAT、RS以及BUFFER狀態  
## 程式流程
1. 讀assembly code(.txt檔)進來  

2. 輸入ADD/SUB、MUL、DIV在ALU中需執行的cycle數  

3. 對每個instruction做Issue、Dispatch及Write Result  

4. Issue或Dispatch可與Write Result在同個cycle執行，但Issue與Dispatch不能在同個cycle執行  

5. 每個cycle執行完後currentCycle均加1  

6. 直到所有instruction均執行並寫入結束  

## 程式碼解釋
`struct instruction` 包含 **opcode** 、 **rd** 、 **rs1** 、 **rs2** 和 **immediate**  

`struct contentRS` 包含 **operand** 、 **rs1** 和 **rs2**  

`struct ALU` 包含 **cycle** 、 **RS** 、 **empty**  

`bool RSempty[5] = {}` 設一個空間存放RS的狀態是否為空  

`vector< instruction > ISA` 存放即將被執行的instruction  

`vector< int > RF(6)` 存放Register File，並將其設為6個空間(第1個不使用)  

`vector< string > RAT(6)` 存放Register Allocation Table，並將其設為6個空間(第1個不使用)  

`vector< contentRS > RS(5)` Reservation Station  

`ALU bufferADD` 在加減法器裡的ALU  

`ALU bufferMUL` 在乘除法器裡的ALU  

`int currentCycle = 1` 目前在第幾個 cycle  

`int ADDSUB, MUL, DIV` 分別在 ALU 執行的 cycle time  

`bool changedCycle` 是否為有變化的cycle  

***
```c++
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
```
  將檔案讀進來並分別放到`ISA`內  
***
  ```c++
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
  ```
  輸出該cycle RF、RAT、RS以及BUFFER的狀態  
***
  ```c++
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
  ```
  將instruction從`ISA`Issue進來，並將`ISA`裡的值放入到`RS`
***
  ```c++
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
  ```
  若所屬ALU中的`BUFFER`為空且該`RS`內的**rs1**和**rs2**均為有值的狀態下，Dispatch到`BUFFER`裡並進行運算
***
```c++
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
```
將運算完後的結果寫回RF、RAT以及RS

## Sample Input
    ADDI F1, F2, 1
    SUB F1, F3, F4
    DIV F1, F2, F3
    MUL F2, F3, F4
    ADD F2, F4, F2
    ADDI F4, F1, 2
    MUL F5, F5, F5
    ADD F1, F4, F4
## Sample Output
