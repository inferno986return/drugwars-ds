/*---------------------------------------------------------------------------------
$Id: template.c,v 1.2 2005/09/07 20:06:06 wntrmute Exp $

Simple ARM7 stub (sends RTC, TSC, and X/Y data to the ARM 9)

$Log: template.c,v $
Revision 1.2  2005/09/07 20:06:06  wntrmute
updated for latest libnds changes

Revision 1.8  2005/08/03 05:13:16  wntrmute
corrected sound code


---------------------------------------------------------------------------------*/
#include <nds.h>

#include <nds/bios.h>
#include <nds/arm7/touch.h>
#include <nds/arm7/clock.h>

//#include "transfer.h" //for doublec's fifo impl// 
//#include "ndsload.h" //for exit to Darkain MultiNDS Loader
#include "Sound7.h"

//---------------------------------------------------------------------------------
void startSound(int sampleRate, const void* data, u32 bytes, u8 channel, u8 vol,  u8 pan, u8 format) {
 //---------------------------------------------------------------------------------
 SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
 SCHANNEL_SOURCE(channel) = (u32)data;
 SCHANNEL_LENGTH(channel) = bytes >> 2 ;
 SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_FORMAT_8BIT:SOUND_FORMAT_16BIT);
}


//---------------------------------------------------------------------------------
s32 getFreeSoundChannel() {
 //---------------------------------------------------------------------------------
 int i;
 for (i=0; i<16; i++) {
  if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
 }
 return -1;
}

//---------------------------------------------------------------------------------
void VblankHandler(void) {
 //---------------------------------------------------------------------------------
 
   if (REG_IF & IRQ_VBLANK) { 
	 uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0, batt=0, aux=0;
	 int t1=0, t2=0;
	 uint32 temp=0;
	 uint8 ct[sizeof(IPC->curtime)];
	 u32 i;
	  
	 // Read the touch screen
	 
	 but = REG_KEYXY;
	 
	 if (!(but & (1<<6))) {
	  
	  touchPosition tempPos = touchReadXY();
	  
	  x = tempPos.x;
	  y = tempPos.y;
	  xpx = tempPos.px;
	  ypx = tempPos.py;
	 }
	 
	 z1 = touchRead(TSC_MEASURE_Z1);
	 z2 = touchRead(TSC_MEASURE_Z2);
	 
	 
	 batt = touchRead(TSC_MEASURE_BATTERY);
	 aux  = touchRead(TSC_MEASURE_AUX);
	 
	 // Read the time
	 rtcGetTime((uint8 *)ct);
	 BCDToInteger((uint8 *)&(ct[1]), 7);
	 
	 // Read the temperature
	 temp = touchReadTemperature(&t1, &t2);
	 
	 // Update the IPC struct
	 IPC->buttons  = but;
	 IPC->touchX   = x;
	 IPC->touchY   = y;
	 IPC->touchXpx  = xpx;
	 IPC->touchYpx  = ypx;
	 IPC->touchZ1  = z1;
	 IPC->touchZ2  = z2;
	 IPC->battery  = batt;
	 IPC->aux   = aux;
	 
	 for(i=0; i<sizeof(ct); i++) {
	  IPC->curtime[i] = ct[i];
	 }
	 
	 IPC->temperature = temp;
	 IPC->tdiode1 = t1;
	 IPC->tdiode2 = t2;
	 
	 //ready to exit to loader if needed (darkain loader)
	 //if (LOADNDS->PATH != 0) {
	 // LOADNDS->ARM7FUNC(LOADNDS->PATH);
	 //}
	 
	 
	 //ready to exit to loader if needed (gbamp loader)
	 //if (*((vu32*)0x027FFE24) == (u32)0x027FFE04) { // Check for ARM9 reset
	 // REG_IME = IME_DISABLE; // Disable interrupts
	 // REG_IF = REG_IF; // Acknowledge interrupt
	 // *((vu32*)0x027FFE34) = (u32)0x08000000; // Bootloader start address
	 // swiSoftReset(); // Jump to boot loader
	 //}
	  
	 SndVblIrq();	// DekuTree64's version :)
	 
	 //sound code  :-)
	 TransferSound * snd = IPC->soundData;
	 IPC->soundData = 0;
	 
	 if (0 != snd) {
	  
	  for (i=0; i<snd->count; i++) {
    	startSound(snd->data[i].rate, snd->data[i].data, snd->data[i].len, 15, snd->data[i].vol, snd->data[i].pan, snd->data[i].format);
	  }
	 }
	}
	  
  if (REG_IF & IRQ_TIMER0) {
		// DekuTree64's MOD player update
    SndTimerIrq();
  }  
  
  REG_IF = REG_IF;
 
 //process fifo commands (thanks to DoubleC)
 //CommandProcessCommands();
    //arm7_fifo->cnt  = REG_IPC_FIFO_CR;
}

//---------------------------------------------------------------------------------
int main(int argc, char ** argv) {
//---------------------------------------------------------------------------------
 
 // Reset the clock if needed
 rtcReset();
 
 //set up the loader exit code
 //LOADNDS->PATH = 0;
 
  REG_IME = 0;
  IRQ_HANDLER = &VblankHandler;
  REG_IE = IRQ_VBLANK;
  REG_IF = ~0;
  DISP_SR = DISP_VBLANK_IRQ;
  REG_IME = 1;
 
 SndInit7 (); 
 
 // Keep the ARM7 out of main RAM
 while (1) swiWaitForVBlank();
}
