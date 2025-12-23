SAME54_Project/
├── SAME54_Project.X/                 ← MPLAB X project
│   ├── Makefile
│   └── nbproject/
│       ├── configurations.xml
│       └── project.xml
│
├── common/                           ← Core system + board support
│   ├── board.c                      ← LED, switch, board helpers
│   ├── board.h
│   ├── cpu.c                        ← clock / SystemConfigPerformance
│   ├── cpu.h
│   ├── delay.c                      ← async delay helpers
│   ├── delay.h
│   ├── systick.c                    ← SysTick + millis()
│   ├── systick.h
│   └── user_row.c                   ← user row / NVM helpers
│
├── drivers/                          ← Peripheral drivers
│   ├── uart.c                       ← SERCOM2 polling / basic UART
│   ├── uart.h
│   ├── uart_dma.c                   ← SERCOM2 + DMAC TX (logging)
│   └── uart_dma.h
│
├── main.c                            ← primary application entry
├── main_test.c                       ← test / experimentation entry
├── main_uart_dma.c                  ← DMA UART demo app
│
├── architecture_design_note.md       ← design & mental model notes
└── package.log                      ← build / packaging log
