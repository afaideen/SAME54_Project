#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-same54_xplained_pro.mk)" "nbproject/Makefile-local-same54_xplained_pro.mk"
include nbproject/Makefile-local-same54_xplained_pro.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=same54_xplained_pro
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/SAME54_Project.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/SAME54_Project.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS

else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../src/common/board.c ../src/common/cpu.c ../src/common/delay.c ../src/common/systick.c ../src/drivers/qspi/qspi_flash.c ../src/drivers/qspi/qspi_hw.c ../src/drivers/rtcc.c ../src/drivers/uart.c ../src/drivers/uart_dma.c ../src/main.c ../src/drivers/qspi/sst26/sst26.c ../src/drivers/qspi/n25q/n25q256a.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/394045403/board.o ${OBJECTDIR}/_ext/394045403/cpu.o ${OBJECTDIR}/_ext/394045403/delay.o ${OBJECTDIR}/_ext/394045403/systick.o ${OBJECTDIR}/_ext/1151356775/qspi_flash.o ${OBJECTDIR}/_ext/1151356775/qspi_hw.o ${OBJECTDIR}/_ext/1639450193/rtcc.o ${OBJECTDIR}/_ext/1639450193/uart.o ${OBJECTDIR}/_ext/1639450193/uart_dma.o ${OBJECTDIR}/_ext/1360937237/main.o ${OBJECTDIR}/_ext/1254920606/sst26.o ${OBJECTDIR}/_ext/456336618/n25q256a.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/394045403/board.o.d ${OBJECTDIR}/_ext/394045403/cpu.o.d ${OBJECTDIR}/_ext/394045403/delay.o.d ${OBJECTDIR}/_ext/394045403/systick.o.d ${OBJECTDIR}/_ext/1151356775/qspi_flash.o.d ${OBJECTDIR}/_ext/1151356775/qspi_hw.o.d ${OBJECTDIR}/_ext/1639450193/rtcc.o.d ${OBJECTDIR}/_ext/1639450193/uart.o.d ${OBJECTDIR}/_ext/1639450193/uart_dma.o.d ${OBJECTDIR}/_ext/1360937237/main.o.d ${OBJECTDIR}/_ext/1254920606/sst26.o.d ${OBJECTDIR}/_ext/456336618/n25q256a.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/394045403/board.o ${OBJECTDIR}/_ext/394045403/cpu.o ${OBJECTDIR}/_ext/394045403/delay.o ${OBJECTDIR}/_ext/394045403/systick.o ${OBJECTDIR}/_ext/1151356775/qspi_flash.o ${OBJECTDIR}/_ext/1151356775/qspi_hw.o ${OBJECTDIR}/_ext/1639450193/rtcc.o ${OBJECTDIR}/_ext/1639450193/uart.o ${OBJECTDIR}/_ext/1639450193/uart_dma.o ${OBJECTDIR}/_ext/1360937237/main.o ${OBJECTDIR}/_ext/1254920606/sst26.o ${OBJECTDIR}/_ext/456336618/n25q256a.o

# Source Files
SOURCEFILES=../src/common/board.c ../src/common/cpu.c ../src/common/delay.c ../src/common/systick.c ../src/drivers/qspi/qspi_flash.c ../src/drivers/qspi/qspi_hw.c ../src/drivers/rtcc.c ../src/drivers/uart.c ../src/drivers/uart_dma.c ../src/main.c ../src/drivers/qspi/sst26/sst26.c ../src/drivers/qspi/n25q/n25q256a.c

# Pack Options 
PACK_COMMON_OPTIONS=-I "${CMSIS_DIR}/CMSIS/Core/Include"



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-same54_xplained_pro.mk dist/${CND_CONF}/${IMAGE_TYPE}/SAME54_Project.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=ATSAME54P20A
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/394045403/board.o: ../src/common/board.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/board.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/board.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/394045403/board.o.d" -o ${OBJECTDIR}/_ext/394045403/board.o ../src/common/board.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/board.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/cpu.o: ../src/common/cpu.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/cpu.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/cpu.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/394045403/cpu.o.d" -o ${OBJECTDIR}/_ext/394045403/cpu.o ../src/common/cpu.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/cpu.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/delay.o: ../src/common/delay.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/delay.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/delay.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/394045403/delay.o.d" -o ${OBJECTDIR}/_ext/394045403/delay.o ../src/common/delay.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/delay.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/systick.o: ../src/common/systick.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/systick.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/systick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/394045403/systick.o.d" -o ${OBJECTDIR}/_ext/394045403/systick.o ../src/common/systick.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/systick.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1151356775/qspi_flash.o: ../src/drivers/qspi/qspi_flash.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1151356775" 
	@${RM} ${OBJECTDIR}/_ext/1151356775/qspi_flash.o.d 
	@${RM} ${OBJECTDIR}/_ext/1151356775/qspi_flash.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1151356775/qspi_flash.o.d" -o ${OBJECTDIR}/_ext/1151356775/qspi_flash.o ../src/drivers/qspi/qspi_flash.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1151356775/qspi_flash.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1151356775/qspi_hw.o: ../src/drivers/qspi/qspi_hw.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1151356775" 
	@${RM} ${OBJECTDIR}/_ext/1151356775/qspi_hw.o.d 
	@${RM} ${OBJECTDIR}/_ext/1151356775/qspi_hw.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1151356775/qspi_hw.o.d" -o ${OBJECTDIR}/_ext/1151356775/qspi_hw.o ../src/drivers/qspi/qspi_hw.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1151356775/qspi_hw.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1639450193/rtcc.o: ../src/drivers/rtcc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1639450193" 
	@${RM} ${OBJECTDIR}/_ext/1639450193/rtcc.o.d 
	@${RM} ${OBJECTDIR}/_ext/1639450193/rtcc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1639450193/rtcc.o.d" -o ${OBJECTDIR}/_ext/1639450193/rtcc.o ../src/drivers/rtcc.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1639450193/rtcc.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1639450193/uart.o: ../src/drivers/uart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1639450193" 
	@${RM} ${OBJECTDIR}/_ext/1639450193/uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1639450193/uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1639450193/uart.o.d" -o ${OBJECTDIR}/_ext/1639450193/uart.o ../src/drivers/uart.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1639450193/uart.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1639450193/uart_dma.o: ../src/drivers/uart_dma.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1639450193" 
	@${RM} ${OBJECTDIR}/_ext/1639450193/uart_dma.o.d 
	@${RM} ${OBJECTDIR}/_ext/1639450193/uart_dma.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1639450193/uart_dma.o.d" -o ${OBJECTDIR}/_ext/1639450193/uart_dma.o ../src/drivers/uart_dma.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1639450193/uart_dma.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d" -o ${OBJECTDIR}/_ext/1360937237/main.o ../src/main.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/main.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1254920606/sst26.o: ../src/drivers/qspi/sst26/sst26.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1254920606" 
	@${RM} ${OBJECTDIR}/_ext/1254920606/sst26.o.d 
	@${RM} ${OBJECTDIR}/_ext/1254920606/sst26.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1254920606/sst26.o.d" -o ${OBJECTDIR}/_ext/1254920606/sst26.o ../src/drivers/qspi/sst26/sst26.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1254920606/sst26.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/456336618/n25q256a.o: ../src/drivers/qspi/n25q/n25q256a.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/456336618" 
	@${RM} ${OBJECTDIR}/_ext/456336618/n25q256a.o.d 
	@${RM} ${OBJECTDIR}/_ext/456336618/n25q256a.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG   -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/456336618/n25q256a.o.d" -o ${OBJECTDIR}/_ext/456336618/n25q256a.o ../src/drivers/qspi/n25q/n25q256a.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/456336618/n25q256a.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
else
${OBJECTDIR}/_ext/394045403/board.o: ../src/common/board.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/board.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/board.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/394045403/board.o.d" -o ${OBJECTDIR}/_ext/394045403/board.o ../src/common/board.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/board.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/cpu.o: ../src/common/cpu.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/cpu.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/cpu.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/394045403/cpu.o.d" -o ${OBJECTDIR}/_ext/394045403/cpu.o ../src/common/cpu.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/cpu.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/delay.o: ../src/common/delay.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/delay.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/delay.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/394045403/delay.o.d" -o ${OBJECTDIR}/_ext/394045403/delay.o ../src/common/delay.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/delay.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/394045403/systick.o: ../src/common/systick.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/394045403" 
	@${RM} ${OBJECTDIR}/_ext/394045403/systick.o.d 
	@${RM} ${OBJECTDIR}/_ext/394045403/systick.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/394045403/systick.o.d" -o ${OBJECTDIR}/_ext/394045403/systick.o ../src/common/systick.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/394045403/systick.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1151356775/qspi_flash.o: ../src/drivers/qspi/qspi_flash.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1151356775" 
	@${RM} ${OBJECTDIR}/_ext/1151356775/qspi_flash.o.d 
	@${RM} ${OBJECTDIR}/_ext/1151356775/qspi_flash.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1151356775/qspi_flash.o.d" -o ${OBJECTDIR}/_ext/1151356775/qspi_flash.o ../src/drivers/qspi/qspi_flash.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1151356775/qspi_flash.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1151356775/qspi_hw.o: ../src/drivers/qspi/qspi_hw.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1151356775" 
	@${RM} ${OBJECTDIR}/_ext/1151356775/qspi_hw.o.d 
	@${RM} ${OBJECTDIR}/_ext/1151356775/qspi_hw.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1151356775/qspi_hw.o.d" -o ${OBJECTDIR}/_ext/1151356775/qspi_hw.o ../src/drivers/qspi/qspi_hw.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1151356775/qspi_hw.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1639450193/rtcc.o: ../src/drivers/rtcc.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1639450193" 
	@${RM} ${OBJECTDIR}/_ext/1639450193/rtcc.o.d 
	@${RM} ${OBJECTDIR}/_ext/1639450193/rtcc.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1639450193/rtcc.o.d" -o ${OBJECTDIR}/_ext/1639450193/rtcc.o ../src/drivers/rtcc.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1639450193/rtcc.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1639450193/uart.o: ../src/drivers/uart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1639450193" 
	@${RM} ${OBJECTDIR}/_ext/1639450193/uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1639450193/uart.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1639450193/uart.o.d" -o ${OBJECTDIR}/_ext/1639450193/uart.o ../src/drivers/uart.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1639450193/uart.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1639450193/uart_dma.o: ../src/drivers/uart_dma.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1639450193" 
	@${RM} ${OBJECTDIR}/_ext/1639450193/uart_dma.o.d 
	@${RM} ${OBJECTDIR}/_ext/1639450193/uart_dma.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1639450193/uart_dma.o.d" -o ${OBJECTDIR}/_ext/1639450193/uart_dma.o ../src/drivers/uart_dma.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1639450193/uart_dma.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1360937237/main.o: ../src/main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1360937237" 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360937237/main.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1360937237/main.o.d" -o ${OBJECTDIR}/_ext/1360937237/main.o ../src/main.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1360937237/main.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/1254920606/sst26.o: ../src/drivers/qspi/sst26/sst26.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1254920606" 
	@${RM} ${OBJECTDIR}/_ext/1254920606/sst26.o.d 
	@${RM} ${OBJECTDIR}/_ext/1254920606/sst26.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/1254920606/sst26.o.d" -o ${OBJECTDIR}/_ext/1254920606/sst26.o ../src/drivers/qspi/sst26/sst26.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1254920606/sst26.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/_ext/456336618/n25q256a.o: ../src/drivers/qspi/n25q/n25q256a.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/456336618" 
	@${RM} ${OBJECTDIR}/_ext/456336618/n25q256a.o.d 
	@${RM} ${OBJECTDIR}/_ext/456336618/n25q256a.o 
	${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -I"C:/Microchip/xc32/v4.50/pic32c/include/proc/SAME54" -MMD -MF "${OBJECTDIR}/_ext/456336618/n25q256a.o.d" -o ${OBJECTDIR}/_ext/456336618/n25q256a.o ../src/drivers/qspi/n25q/n25q256a.c    -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -mdfp="${DFP_DIR}" ${PACK_COMMON_OPTIONS} 
	@${FIXDEPS} "${OBJECTDIR}/_ext/456336618/n25q256a.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/SAME54_Project.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE) -g   -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/SAME54_Project.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D=__DEBUG_D,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -mdfp="${DFP_DIR}"
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/SAME54_Project.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/SAME54_Project.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_same54_xplained_pro=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml -mdfp="${DFP_DIR}"
	${MP_CC_DIR}\\xc32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/SAME54_Project.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/same54_xplained_pro
	${RM} -r dist/same54_xplained_pro

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
