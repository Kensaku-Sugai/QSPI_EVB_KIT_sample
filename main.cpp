#include "mbed.h"
#include "QSPI_FLASH.h"
#include <string.h>

Serial pc(USBTX, USBRX);
QSPI_FLASH qspiflash(PC_0, PC_1, PC_2, PC_3, PC_4, PC_5);

uint8_t databuf[PAGE_SIZE_QSPI_NAND]={};  // Program data buffer

void dump(uint8_t *dt, uint8_t nor_nand) {
    uint16_t ca = 0;
    uint32_t end = 0;
	
    if(nor_nand==0) {
        end = 16;
    } else if(nor_nand==1) {
        end = (PAGE_SIZE_QSPI_NAND)/16;
    }

    for(uint16_t i=0; i<end; i++) {
        pc.printf("%04X: ", ca);
        for(uint16_t j=0; j<16; j++) {
            pc.printf("%02X ", dt[i*16+j]);
        }
        pc.printf("\n");
        ca = ca + 16;
    }
}

// Block Erase
void block_erase(uint32_t addr, uint8_t nor_nand, uint8_t flag_ADS) {
    if(nor_nand==0) {
        pc.printf("==================Sector Erase Start==================\n");
        qspiflash.QSPI_WriteEnable();
        qspiflash.QSPI_NOR_Sector_Erase(addr,flag_ADS);
        pc.printf("Sector Erase (20h) : done\n");
    } else if(nor_nand==1) {
        pc.printf("==================Block Erase Start==================\n");
        qspiflash.QSPI_WriteEnable();
        qspiflash.QSPI_NAND_BlockErase(addr);
        pc.printf("Block Erase (D8h) : done\n");
    }
}

// Read data
void read_data(uint32_t addr, uint8_t nor_nand, uint8_t flag_ADS, uint8_t read_mode) {
    uint16_t num;                   // Number of read data

    for(num=0; num<PAGE_SIZE_QSPI_NAND; ++num){
        databuf[num]=0;
    }

    pc.printf("===================Page Read Start===================\n");
    if(nor_nand==0) {
        num=qspiflash.QSPI_NOR_QuadFastRead(addr,databuf, flag_ADS);
        pc.printf("Read (EBh) : num = %d\n", num);
        dump(databuf, nor_nand);
    } else if(nor_nand==1) {
        num = qspiflash.QSPI_NAND_PageDataRead(addr);
        if(num) {
            pc.printf("Page Data Read (13h) : done\n");
        }
        num = qspiflash.QSPI_NAND_QuadReadData(0x00, databuf, read_mode);
        pc.printf("Read Data (EBh) : num = %d\n", num);
        dump(databuf, nor_nand);
    }
}

// Program data
void program_data(uint32_t addr, uint8_t mode, uint8_t nor_nand, uint8_t flag_ADS) {
    uint16_t num;                    // Number of read data

    pc.printf("=================Page Program Start==================\n");

    if(nor_nand==0) {
        if(mode==0) {    // Increment program mode
            for(uint16_t i=0; i<PAGE_SIZE_QSPI_NOR; i++) {
                databuf[i] = i;
            }
        } else if(mode==1) {    // AAh 55h program mode
            for(uint16_t i=0; i<PAGE_SIZE_QSPI_NOR; i++) {
                if(i%2==0) {
                    databuf[i] = 0xAA;
                } else {
                    databuf[i] = 0x55;
                }
            }
        } else if(mode==2) {    // FFh 00h program mode
            for(uint16_t i=0; i<PAGE_SIZE_QSPI_NOR; i++) {
                if(i%2==0) {
                    databuf[i] = 0xFF;
                } else {
                    databuf[i] = 0x00;
                }
            }
        } else if(mode==3) {    // 00h program mode
            for(uint16_t i=0; i<PAGE_SIZE_QSPI_NOR; i++) {
                databuf[i] = 0x00;
            }
        }
        qspiflash.QSPI_NOR_QuadInputPageProgram(addr, databuf, flag_ADS);
        while(qspiflash.QSPI_NOR_IsBusy());
        pc.printf("Page Program (32h) : \n");
    } else if(nor_nand==1) {
        if(mode==0) {    // Increment program mode
            for(uint16_t i=0; i<PAGE_SIZE_QSPI_NAND; i++) {
                databuf[i] = i;
            }
        } else if(mode==1) {    // AAh 55h program mode
            for(uint16_t i=0; i<PAGE_SIZE_QSPI_NAND; i++) {
                if(i%2==0) {
                    databuf[i] = 0xAA;
                } else {
                    databuf[i] = 0x55;
                }
            }
        } else if(mode==2) {    // FFh 00h program mode
            for(uint16_t i=0; i<PAGE_SIZE_QSPI_NAND; i++) {
                if(i%2==0) {
                    databuf[i] = 0xFF;
                } else {
                    databuf[i] = 0x00;
                }
            }
        } else if(mode==3) {    // 00h program mode
            for(uint16_t i=0; i<PAGE_SIZE_QSPI_NAND; i++) {
                databuf[i] = 0x00;
            }
        }

        qspiflash.QSPI_WriteEnable();
        num = qspiflash.QSPI_NAND_QuadProgramDataLoad(0x00, databuf);
        if(num) {
            pc.printf("Program Data Load (32h) : num = %d\n", num);
        }

        qspiflash.QSPI_WriteEnable();
        num = qspiflash.QSPI_NAND_ProgramExecute(addr);
        if(num) {
            pc.printf("Program Execute (10h) : done\n");
        }
    }
}

// Detect Initial Bad Block Marker
void detect_ibbm(void) {
    uint32_t addr=0;
    uint32_t bb_num=0;

    for(uint16_t i=0; i<MAX_BLOCKS_QSPI_NAND; i++) {
        qspiflash.QSPI_NAND_PageDataRead(addr);
        qspiflash.QSPI_NAND_ReadData2(0, databuf);
        if(databuf[0]!=0xFF) {
            pc.printf("Block %d = Initial Bad Block val=%02x adr=%04x\n", i, databuf[0], addr);
            bb_num = bb_num + 1;
        }
        addr = addr + 64;
    }
    pc.printf("Total Number of Initial Bad Blocks = %d\n", bb_num);
}

int main() {
    uint8_t menu;
    uint32_t address;
    uint8_t temp[80];     // Temporary data buffer
    uint8_t id[5];        // Temporary data buffer
    uint8_t status[3];    // Temporary data buffer
    uint8_t flag_ADS=0;   // 0: 3-Byte Address Mode  1: 4-Byte Address Mode
    uint8_t stack=0;      // 0: Single Chip Package  1: Multi Chip Package
    uint8_t nor_nand=0;   // 0: NOR  1: NAND
    uint8_t read_mode=0;  // 0: Continuous Read Mode 1: Buffer Read Mode

    while(1) {
        pc.printf("\n");
        pc.printf("=====================================================\n");
        pc.printf("=         mbed Serial Flash Sample Program          =\n");
        pc.printf("=         Winbond Electronics Corporation           =\n");
        pc.printf("=====================================================\n");

        // Get Manufacture and Device ID
        qspiflash.QSPI_NOR_ReadJEDECID(id);
        if(id[1]==0x40) {
            qspiflash.QSPI_STACK_Die_Select_NAND();
            qspiflash.QSPI_NAND_ReadJEDECID(id);
            if(id[1]==0xAB) {  //stacked NOR
                pc.printf("SpiStack(NOR) was detected.\n");
                stack=1;
                nor_nand=0;
                qspiflash.QSPI_STACK_Die_Select_NOR();
                qspiflash.QSPI_NOR_DeviceReset();
                qspiflash.QSPI_NOR_ReadJEDECID(id);
                status[0]=qspiflash.QSPI_NOR_ReadStatusReg1();
                status[1]=qspiflash.QSPI_NOR_ReadStatusReg2();
                status[2]=qspiflash.QSPI_NOR_ReadStatusReg3();
            } else {           //non-stacked NOR
                pc.printf("Serial NOR was detected.\n");
                stack=0;
                nor_nand=0;
                qspiflash.QSPI_STACK_Die_Select_NOR();
                qspiflash.QSPI_NOR_DeviceReset();
                qspiflash.QSPI_NOR_ReadJEDECID(id);
                status[0]=qspiflash.QSPI_NOR_ReadStatusReg1();
                status[1]=qspiflash.QSPI_NOR_ReadStatusReg2();
                status[2]=qspiflash.QSPI_NOR_ReadStatusReg3();
            }
        } else {
            qspiflash.QSPI_NAND_ReadJEDECID(id);
            if(id[1]==0xAA) {  //non-stacked NAND
                pc.printf("Serial NAND was detected.\n");
                stack=0;
                nor_nand=1;
                status[0]=qspiflash.QSPI_NAND_ReadStatusReg1();
                status[1]=qspiflash.QSPI_NAND_ReadStatusReg2();
                status[2]=qspiflash.QSPI_NAND_ReadStatusReg3();
            } else {
                if(id[1]==0xAB) {  //stacked NAND
            	    pc.printf("SpiStack(NAND) was detected.\n");
                    stack=1;
                    nor_nand=1;
                    status[0]=qspiflash.QSPI_NAND_ReadStatusReg1();
                    status[1]=qspiflash.QSPI_NAND_ReadStatusReg2();
                    status[2]=qspiflash.QSPI_NAND_ReadStatusReg3();
                } else {
                    pc.printf("Unknown Flash Memory was detected.\n");
                }
            }
        }

        pc.printf("Vendor ID : ");
        pc.printf("%02X", id[0]);
        pc.printf("\n");
        pc.printf("Device ID : ");
        for(uint8_t i=1; i<3; i++) {
            pc.printf("%02X", id[i]);
            pc.printf(" ");
        }
        pc.printf("\n");

        if(nor_nand==0) {
            pc.printf("=====================================================\n");
            pc.printf("Serial NOR mode\n");
            pc.printf("=====================================================\n");
            pc.printf("Menu :\n");
            if(stack==1) { 
                pc.printf("  0. Die Select to Serial NAND\n");
            }
            pc.printf("  1. Read\n");
            pc.printf("  2. Sector Erase\n");
            pc.printf("  3. Program Increment Data\n");
            pc.printf("  4. Program AAh 55h\n");
            pc.printf("  5. Program FFh 00h\n");
            pc.printf("  6. Program 00h\n");
            pc.printf("  7. Read Status Register\n");
            pc.printf("Please input menu number: ");
            pc.scanf("%d", &menu);

            switch(menu) {
                case 0:
                    if(stack==1) {
                        pc.printf(">Die Select to Serial NAND\n");
                        qspiflash.QSPI_STACK_Die_Select_NAND();
                        nor_nand=1;
                        pc.printf(" Stack : Serial NAND selected\n");
                    } else {
                        pc.printf("Invalid menu number\n");
                    }
                    break;
                case 1:
                    pc.printf(">Read\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    read_data(address, nor_nand, flag_ADS, read_mode);
                    break;
                case 2:
                    pc.printf(">Sector Erase\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    block_erase(address, nor_nand, flag_ADS);
                    break;
                case 3:
                    pc.printf(">Program Increment Data\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    program_data(address, 0, nor_nand, flag_ADS);
                    break;
                case 4:
                    pc.printf(">Program AAh 55h\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    program_data(address, 1, nor_nand, flag_ADS);
                    break;
                case 5:
                    pc.printf(">Program FFh 00h\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    program_data(address, 2, nor_nand, flag_ADS);
                    break;
                case 6:
                    pc.printf(">Program 00h\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    program_data(address, 3, nor_nand, flag_ADS);
                    break;
                case 7:
                    status[0]=qspiflash.QSPI_NOR_ReadStatusReg1();
                    status[1]=qspiflash.QSPI_NOR_ReadStatusReg2();
                    status[2]=qspiflash.QSPI_NOR_ReadStatusReg3();
                    pc.printf("Status : ");
                    for(uint8_t i=0; i<3; i++) {
                        pc.printf("%02X", status[i]);
                        pc.printf(" ");
                    }
                    pc.printf("\n");
                    break;
            }
        } else if(nor_nand==1) {
            // Set Block Protect Bits to 0
            qspiflash.QSPI_NAND_WriteStatusReg1(0x00);

            // Get ECC-E and BUF of Status Register-2
            if(status[1] & 0x10) {
                pc.printf("ECC-E  : 1\n");
            } else {
                pc.printf("ECC-E  : 0\n");
            }
            read_mode=status[1] & 0x08;
            if(read_mode) {
                pc.printf("BUF    : 1\n");
            } else {
                pc.printf("BUF    : 0\n");
            }

            // Get P-FAIL, E-FAIL and WEL of Status Register-3
            if(status[2] & 0x08) {
                pc.printf("P-FAIL : 1\n");
            } else {
                pc.printf("P-FAIL : 0\n");
            }
            if(status[2] & 0x04) {
                pc.printf("E-FAIL : 1\n");
            } else {
                pc.printf("E-FAIL : 0\n");
            }

            pc.printf("=====================================================\n");
            pc.printf("Serial NAND mode \n");
            pc.printf("=====================================================\n");
            pc.printf("Menu :\n");
            if(stack==1){
                pc.printf("  0. Die Select to Serial NOR\n");
            }
            pc.printf("  1. Read\n");
            pc.printf("  2. Block Erase\n");
            pc.printf("  3. Program Increment Data\n");
            pc.printf("  4. Program AAh 55h\n");
            pc.printf("  5. Program FFh 00h\n");
            pc.printf("  6. Program 00h\n");
            pc.printf("  7. Detect Initial Bad Block Marker\n");
            pc.printf("       -> Please execute \"7\" before the first Erase/Program.\n");
            pc.printf("  8. Read BBM LUT\n");
            pc.printf("  9. Read Status Register\n");
            pc.printf("Please input menu number: ");
            pc.scanf("%d", &menu);

            switch(menu) {
                case 0:
                    if(stack==1) {
                        pc.printf(">Die Select to Serial NOR\n");
                        qspiflash.QSPI_STACK_Die_Select_NOR();
                        nor_nand=0;
                        pc.printf(" Stack : Serial NOR selected\n");
                    } else {
                        pc.printf("Invalid menu number\n");
                    }
                    break;
                case 1:
                    pc.printf(">Read\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    read_data(address, nor_nand, flag_ADS, read_mode);
                    break;
                case 2:
                    pc.printf(">Block Erase\n");
                    pc.printf(">Please input address(Page): ");
                    pc.scanf("%x", &address);
                    block_erase(address, nor_nand, flag_ADS);
                    break;
                case 3:
                    pc.printf(">Program Increment Data\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    program_data(address, 0, nor_nand, flag_ADS);
                    break;
                case 4:
                    pc.printf(">Program AAh 55h\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    program_data(address, 1, nor_nand, flag_ADS);
                    break;
                case 5:
                    pc.printf(">Program FFh 00h\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    program_data(address, 2, nor_nand, flag_ADS);
                    break;
                case 6:
                    pc.printf(">Program 00h\n");
                    pc.printf(">Please input address: ");
                    pc.scanf("%x", &address);
                    program_data(address, 3, nor_nand, flag_ADS);
                    break;
                case 7:
                    pc.printf(">Detect Bad Block Marker\n");
                    detect_ibbm();
                    break;
                case 8:
                    qspiflash.QSPI_NAND_ReadBBMLUT(temp);
                    for(uint8_t i=0;i<20;++i){
                          pc.printf("LBA[%02d] = %02X %02X  PBA[%02d] = %02X %02X \n", i, temp[i*4+0],temp[i*4+1],i,temp[i*4+2],temp[i*4+3]);
                  }
                    break;
                case 9:
                    status[0]=qspiflash.QSPI_NAND_ReadStatusReg1();
                    status[1]=qspiflash.QSPI_NAND_ReadStatusReg2();
                    status[2]=qspiflash.QSPI_NAND_ReadStatusReg3();
                    pc.printf("Status : ");
                    for(uint8_t i=0; i<3; i++) {
                        pc.printf("%02X", status[i]);
                        pc.printf(" ");
                    }
                    pc.printf("\n");
                    break;
               default:
                    pc.printf("Invalid menu number\n");
                    break;
            }
        }
    }
}
