這是一份為你的專案量身打造的 **專業級 README.md**。

這份文件不僅僅是說明書，它是你的**技術行銷文件**。它強調了你如何解決 **硬體時序 (Timing)**、**軟硬整合 (Co-design)** 以及 **自動化驗證 (Automation)** 的難題。

請將以下內容複製到你的 `README.md` 檔案中：

---

# RISC-V Single-Cycle SoC on Zynq-7000 with Automated HIL Verification

這是一個基於 **RISC-V RV32I 指令集** 的單週期 (Single-Cycle) 處理器 SoC 專案，實作於 **Xilinx Zynq-7000 FPGA (Cora Z7S)** 平台上。

本專案不僅包含硬體設計，更整合了 **Python 自動化測試套件** 與 **C 語言韌體**，建構了一套完整的 **HIL (Hardware-in-the-Loop)** 硬體迴路驗證系統，解決了傳統 FPGA 開發中測試繁瑣與觀察不易的問題。

---

## 🌟 Key Features (核心功能)

* **RISC-V Core Design:**
* 實作 RV32I 基礎指令集 (Arithmetic, Logic, Memory, Branch/Jump)。
* 自定義 **AXI4-Lite Slave Interface**，讓 RISC-V 核心能作為 IP 掛載於 Zynq PS 端。


* **System Integration (SoC):**
* **PS-PL Co-design:** 利用 Zynq PS (Cortex-A9) 作為控制器，透過 AXI Bus 控制 PL 端的 RISC-V 核心。
* **Stable IO Protocol:** 在 C 韌體層解決了高頻寫入下的訊號遺漏問題 (Ghost Instruction Issue)，確保指令寫入的時序穩定性。


* **Automated Verification Suite:**
* 開發 Python 自動化腳本，支援 **批量測試 (Batch Testing)**。
* 支援 Hex 檔案動態載入，無需重新燒錄 Bitstream 即可更換測試程式。
* 包含完整的測試案例庫 (Add, Sub, Branch, Fibonacci, Multiplication)。



---

## 🏗️ System Architecture (系統架構)

系統資料流如下：
`PC (Python Script) <--> UART <--> Zynq PS (C Firmware) <--> AXI4-Lite <--> RISC-V IP (PL)`

1. **PC 端 (Host):** Python 腳本解析 Hex 機器碼，透過 UART 發送指令與控制訊號。
2. **PS 端 (Controller):** 運行於 Cortex-A9 的 C 程式接收 UART 封包，並透過 `Xil_Out32` 驅動 AXI GPIO，產生精確的 **Write Enable** 脈衝。
3. **PL 端 (Target):** RISC-V Core 接收指令並寫入 Instruction Memory，執行後將結果存回 Data Memory。
4. **驗證:** Python 讀回 Data Memory 的值，並與黃金模型 (Golden Reference) 進行自動比對。

---

## 📂 Repository Structure (檔案結構)

```text
RISCV_Zynq_SoC/
├── hw/                     # 硬體設計 (Hardware Source)
│   ├── src/
│   │   ├── ALU_Decoder.v   # 已修復 SUB 指令解碼錯誤 (Bug Fix)
│   │   ├── system_wrapper.v
│   │   └── ... (其他 Verilog 模組)
│   ├── bd/                 # Block Design Tcl 腳本
│   └── system_wrapper.xsa  # 硬體描述檔 (Hardware Handoff)
│
├── sw/                     # 嵌入式軟體 (Embedded Software)
│   └── src/
│       ├── helloworld.c    # UART Monitor Firmware (含時序控制邏輯)
│       └── lscript.ld      # Linker Script
│
└── tests/                  # 自動化驗證套件 (Automation Suite)
    ├── test_riscv.py       # Python 主測試腳本
    └── test_suite/         # 測試案例庫
        ├── hex/            # RISC-V 機器碼 (.hex)
        └── expected/       # 預期結果 (.ans)

```

---

## 🚀 Getting Started (如何執行)

### 1. Hardware Setup (Vivado)

1. 開啟 Vivado，載入專案或利用 Source 檔案重建專案。
2. 確認 Block Design 包含 Zynq Processing System 與自定義 RISC-V IP。
3. Generate Bitstream 並 Export Hardware (`.xsa`)。

### 2. Firmware Setup (Vitis)

1. 在 Vitis 中建立 Platform Project (基於 `.xsa`)。
2. 建立 Application Project (`riscv_automation_monitor`)。
3. 將 `sw/src/helloworld.c` 與 `lscript.ld` 複製到專案的 `src` 資料夾中。
4. Build Project 並執行 **Run As -> Launch Hardware** (燒錄 FPGA 並執行 C 程式)。

### 3. Run Verification (Python)

確保 FPGA 已啟動且 UART 訊號線已連接電腦。

```bash
# 安裝相依套件
pip install pyserial

# 進入測試資料夾
cd tests

# 執行自動化測試
python test_riscv.py

```

---

## 📊 Test Cases & Results (測試成果)

本專案通過了以下關鍵迴歸測試 (Regression Tests)：

| Test Case | Description | Focus Area | Status |
| --- | --- | --- | --- |
| **01_add** | Basic Addition | ALU ADD operation, Register Write | ✅ PASS |
| **02_sub** | Subtraction | ALU SUB decoding (Fixed Bug), R-Type Logic | ✅ PASS |
| **03_branch** | Branch if Equal | BEQ Logic, PC Control, Zero Flag | ✅ PASS |
| **04_fibonacci** | Fibonacci Sequence | RAW Dependency, Loop Logic | ✅ PASS |
| **05_mult** | Software Multiplication | Complex algorithm, Memory Store | ✅ PASS |

**執行截圖：**

```text
[1/5] Running test: 01_add_test
  [PASS] Result: 30 ✓

[2/5] Running test: 02_sub_test
  [PASS] Result: 35 ✓
...
Result: ALL TESTS PASSED! ✓✓✓

```

---

## 🛠️ Future Work (未來展望)

* **Pipelining:** 將單週期架構升級為五級管線 (5-Stage Pipeline)，並處理 Data/Control Hazard。
* **DMA Integration:** 引入 AXI DMA 以加速大數據量的指令載入 (Instruction Loading)。
* **Compliance Testing:** 執行官方 RISC-V Architectural Compliance Test Suite。

---

## 👤 Author

**Pei-Sheng Ke**

* Master of Science in Electrical & Computer Engineering, Ohio State University
* Focus: Digital IC Design, FPGA Verification, Computer Architecture

---

*Last Updated: Jan 2026*
