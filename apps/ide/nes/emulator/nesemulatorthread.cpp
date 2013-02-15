//    NESICIDE - an IDE for the 8-bit NES.
//    Copyright (C) 2009  Christopher S. Pow

//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <QSettings>

#include <cdockwidgetregistry.h>

#include "nesemulatorthread.h"

#include "dbg_cnes.h"
#include "dbg_cnesrom.h"

#include "ccc65interface.h"

#include "cobjectregistry.h"
#include "breakpointwatcherthread.h"
#include "main.h"

#undef main
#include <SDL.h>

#include "cnesicideproject.h"

QSemaphore nesBreakpointSemaphore(0);
QSemaphore nesAudioSemaphore(0);

SDL_AudioSpec sdlAudioSpec;

// Hook function endpoints.
static void breakpointHook ( void )
{
   SDL_PauseAudio(1);

   // Tell the world.
   NESEmulatorThread* emulator = dynamic_cast<NESEmulatorThread*>(CObjectRegistry::getObject("Emulator"));
   emulator->_breakpointHook();

   // Put my thread to sleep.
   nesBreakpointSemaphore.acquire();
   SDL_PauseAudio(0);
}

void NESEmulatorThread::_breakpointHook()
{
   emit breakpoint();
}

static void audioHook ( void )
{
   nesAudioSemaphore.acquire();
}

extern "C" void SDL_GetMoreData(void* /*userdata*/, uint8_t* stream, int32_t len)
{
#if 0
   LARGE_INTEGER t;
   static LARGE_INTEGER to;
   LARGE_INTEGER f;
   QueryPerformanceFrequency(&f);
   QueryPerformanceCounter(&t);
   QString str;
   str.sprintf("Smp: %d, Freq: %d, Ctr: %d, Diff %d, Per %lf", len>>1, f.LowPart, t.LowPart,t.LowPart-to.LowPart,(float)(t.LowPart-to.LowPart)/(float)f.LowPart);
   to = t;
   qDebug(str.toLatin1().constData());
#endif
   if ( nesGetAudioSamplesAvailable() >= (len>>1) )
   {
      memcpy(stream,nesGetAudioSamples(len>>1),len);
   }
   else
   {
//      qDebug("UNDERRUN");
   }
   nesAudioSemaphore.release();
}

NESEmulatorThread::NESEmulatorThread(QObject*)
{
   m_joy [ CONTROLLER1 ] = 0;
   m_joy [ CONTROLLER2 ] = 0;
   m_isRunning = false;
   m_isPaused = false;
   m_showOnPause = false;
   m_pauseAfterFrames = -1;
   m_isStarting = false;
   m_isTerminating = false;
   m_isResetting = false;
   m_debugFrame = 0;
   m_pCartridge = NULL;

   // Enable callbacks from the external emulator library.
   nesSetBreakpointHook(breakpointHook);
   nesSetAudioHook(audioHook);

   SDL_Init ( SDL_INIT_AUDIO );

   sdlAudioSpec.callback = SDL_GetMoreData;
   sdlAudioSpec.userdata = NULL;
   sdlAudioSpec.channels = 1;
   sdlAudioSpec.format = AUDIO_S16SYS;
   sdlAudioSpec.freq = SDL_SAMPLE_RATE;

   // Set up audio sample rate for video mode...
   sdlAudioSpec.samples = APU_SAMPLES;

   SDL_OpenAudio ( &sdlAudioSpec, NULL );

   nesClearAudioSamplesAvailable();

   BreakpointWatcherThread* breakpointWatcher = dynamic_cast<BreakpointWatcherThread*>(CObjectRegistry::getObject("Breakpoint Watcher"));
   QObject::connect(this,SIGNAL(breakpoint()),breakpointWatcher,SLOT(breakpoint()));

   SDL_PauseAudio ( 0 );
}

NESEmulatorThread::~NESEmulatorThread()
{
}

void NESEmulatorThread::kill()
{
   // Force hard-reset of the machine...
   nesEnableBreakpoints(false);

   m_isStarting = false;
   m_isRunning = false;
   m_isPaused = false;
   m_showOnPause = false;
   m_isTerminating = true;

   SDL_PauseAudio ( 1 );

   SDL_CloseAudio ();

   SDL_Quit();

   start();

   while ( !isFinished() )
   {
      nesBreakpointSemaphore.release();
      nesAudioSemaphore.release();
   }
}

void NESEmulatorThread::adjustAudio(int32_t bufferDepth)
{
   SDL_AudioSpec obtained;

   SDL_PauseAudio ( 1 );

   SDL_CloseAudio ();

   sdlAudioSpec.callback = SDL_GetMoreData;
   sdlAudioSpec.userdata = NULL;
   sdlAudioSpec.channels = 1;
   sdlAudioSpec.format = AUDIO_S16SYS;
   sdlAudioSpec.freq = SDL_SAMPLE_RATE;

   // Set up audio sample rate for video mode...
   sdlAudioSpec.samples = bufferDepth;

   SDL_OpenAudio ( &sdlAudioSpec, NULL );

   SDL_PauseAudio ( 0 );

   nesClearAudioSamplesAvailable();
}

void NESEmulatorThread::breakpointsChanged()
{
   // unused
}

void NESEmulatorThread::primeEmulator()
{
   if ( (nesicideProject) &&
        (nesicideProject->getCartridge()) )
   {
      m_pCartridge = nesicideProject->getCartridge();

      nesClearOpcodeMasks();
   }
}

void NESEmulatorThread::softResetEmulator()
{
   // Force hard-reset of the machine...
   nesEnableBreakpoints(false);

   m_isResetting = true;
   m_isSoftReset = true;
   m_isStarting = false;
   m_isRunning = false;
   m_isPaused = true;
   m_showOnPause = false;

   // If during the last run we were stopped at a breakpoint, clear it...
   if ( !(nesBreakpointSemaphore.available()) )
   {
      nesBreakpointSemaphore.release();
   }
   start();
}

void NESEmulatorThread::resetEmulator()
{
   // Force hard-reset of the machine...
   nesEnableBreakpoints(false);

   m_isResetting = true;
   m_isSoftReset = false;
   m_isStarting = false;
   m_isRunning = false;
   m_isPaused = true;
   m_showOnPause = false;

   // If during the last run we were stopped at a breakpoint, clear it...
   if ( !(nesBreakpointSemaphore.available()) )
   {
      nesBreakpointSemaphore.release();
   }
   start();
}

void NESEmulatorThread::startEmulation ()
{
   m_isStarting = true;

   // If during the last run we were stopped at a breakpoint, clear it...
   if ( !(nesBreakpointSemaphore.available()) )
   {
      nesBreakpointSemaphore.release();
   }
   start();
}

void NESEmulatorThread::stepCPUEmulation ()
{
   uint32_t endAddr;
   uint32_t addr;
   uint32_t absAddr;

   // Check if we have an end address to stop at from a debug information file.
   // If we do, it'll be the valid end of a C statement or an assembly instruction.
   addr = nesGetCPURegister(CPU_PC);
   absAddr = nesGetAbsoluteAddressFromAddress(addr);
   endAddr = CCC65Interface::getEndAddressFromAbsoluteAddress(addr,absAddr);

   if ( endAddr != 0xFFFFFFFF )
   {
      nesSetGotoAddress(endAddr);

      m_isStarting = true;
      m_isPaused = false;

      // If during the last run we were stopped at a breakpoint, clear it...
      if ( !(nesBreakpointSemaphore.available()) )
      {
         nesBreakpointSemaphore.release();
      }
      start();
   }
   else
   {
      // If during the last run we were stopped at a breakpoint, clear it...
      // But ensure we come right back...
      nesStepCpu();

      m_isStarting = true;
      m_isPaused = false;

      if ( !(nesBreakpointSemaphore.available()) )
      {
         nesBreakpointSemaphore.release();
      }
      start();
   }
}

void NESEmulatorThread::stepOverCPUEmulation ()
{
   uint32_t endAddr;
   uint32_t addr;
   uint32_t absAddr;
   uint32_t instAbsAddr;
   bool    isInstr;
   uint8_t instr;

   // Check if we have an end address to stop at from a debug information file.
   // If we do, it'll be the valid end of a C statement or an assembly instruction.
   addr = nesGetCPURegister(CPU_PC);
   absAddr = nesGetAbsoluteAddressFromAddress(addr);
   endAddr = CCC65Interface::getEndAddressFromAbsoluteAddress(addr,absAddr);

   if ( endAddr != 0xFFFFFFFF )
   {
      // If the line has enough of an assembly-stream associated with it...
      if ( endAddr-addr >= 2 )
      {
         // Check if last instruction on line is JSR...
         // This is fairly typical of if conditions with function calls on the same line.
         instr = nesGetPRGROMData(endAddr-2);
         instAbsAddr = nesGetAbsoluteAddressFromAddress(endAddr-2);
         isInstr = CCC65Interface::isAbsoluteAddressAnOpcode(instAbsAddr);
         if ( !isInstr )
         {
            instr = nesGetPRGROMData(addr);
         }
      }
      else
      {
         instr = nesGetPRGROMData(addr);
      }
   }
   else
   {
      // Check if the current instruction is a JSR...
      instr = nesGetPRGROMData(addr);

      // Assume the instruction is JSR for loop below.
      endAddr = addr+2;
   }

   // If the current instruction is a JSR we need to tell the emulator to
   // go to the PC one past the JSR to 'step over' the JSR.
   if ( instr == JSR_ABSOLUTE )
   {
      // Go to next opcode point in ROM.
      // This *should* be where the JSR will vector back to on RTS.
      nesSetGotoAddress(endAddr+1);

      m_isStarting = true;
      m_isPaused = false;

      // If during the last run we were stopped at a breakpoint, clear it...
      if ( !(nesBreakpointSemaphore.available()) )
      {
         nesBreakpointSemaphore.release();
      }
      start();
   }
   else
   {
      stepCPUEmulation();
   }
}

void NESEmulatorThread::stepOutCPUEmulation ()
{
//CPTODO: FINISH THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   m_isStarting = true;
   m_isPaused = false;

   // If during the last run we were stopped at a breakpoint, clear it...
   if ( !(nesBreakpointSemaphore.available()) )
   {
      nesBreakpointSemaphore.release();
   }
   start();
}

void NESEmulatorThread::stepPPUEmulation ()
{
   // If during the last run we were stopped at a breakpoint, clear it...
   // But ensure we come right back...
   nesStepPpu();

   m_isStarting = true;
   m_isPaused = false;

   if ( !(nesBreakpointSemaphore.available()) )
   {
      nesBreakpointSemaphore.release();
   }
   start();
}

void NESEmulatorThread::advanceFrame ()
{
   // If during the last run we were stopped at a breakpoint, clear it...
   // But ensure we come right back...
   nesStepPpuFrame();

   m_isStarting = true;
   m_isPaused = false;

   if ( !(nesBreakpointSemaphore.available()) )
   {
      nesBreakpointSemaphore.release();
   }
   start();
}

void NESEmulatorThread::pauseEmulation (bool show)
{
   m_isStarting = false;
   m_isRunning = false;
   m_isPaused = true;
   m_showOnPause = show;
   start();
}

void NESEmulatorThread::loadCartridge()
{
   QFile saveState;
   QString errors;
   int32_t b;
   int32_t a;

   // Clear emulator's cartridge ROMs...
   nesUnloadROM();

   if ( m_pCartridge->getPrgRomBanks()->getPrgRomBanks().count() )
   {
      // Load cartridge PRG-ROM banks into emulator...
      for ( b = 0; b < m_pCartridge->getPrgRomBanks()->getPrgRomBanks().count(); b++ )
      {
         nesLoadPRGROMBank ( b, (uint8_t*)m_pCartridge->getPrgRomBanks()->getPrgRomBanks().at(b)->getBankData() );

         // Update opcode masks to show proper disassembly...
         for ( a = 0; a < MEM_8KB; a++ )
         {
            if ( CCC65Interface::isAbsoluteAddressAnOpcode((b*MEM_8KB)+a) )
            {
               nesSetOpcodeMask((b*MEM_8KB)+a,1);
            }
            else
            {
               nesSetOpcodeMask((b*MEM_8KB)+a,0);
            }
         }
      }

      // Load cartridge CHR-ROM banks into emulator...
      for ( b = 0; b < m_pCartridge->getChrRomBanks()->getChrRomBanks().count(); b++ )
      {
         nesLoadCHRROMBank ( b, (uint8_t*)m_pCartridge->getChrRomBanks()->getChrRomBanks().at(b)->getBankData() );
      }

      // Perform any necessary fixup from the ROM loading...
      nesLoadROM();

      // Set up PPU with iNES header information...
      if ( m_pCartridge->getMirrorMode() == HorizontalMirroring )
      {
         nesSetHorizontalMirroring();
      }
      else if ( m_pCartridge->getMirrorMode() == VerticalMirroring )
      {
         nesSetVerticalMirroring();
      }
      if ( m_pCartridge->getFourScreen() )
      {
         nesSetFourScreen();
      }

      // Initialize NES...
      nesResetInitial(m_pCartridge->getMapperNumber());

      if ( !nesicideProject->getProjectCartridgeSaveStateName().isEmpty() )
      {
         saveState.setFileName(nesicideProject->getProjectCartridgeSaveStateName());
         if ( saveState.open(QIODevice::ReadOnly) )
         {
            deserializeContent(saveState);
         }
      }

      // Trigger inspector updates...
      nesDisassemble();
      emit updateDebuggers();

      // Trigger UI updates...
      emit machineReady();
   }
}

void NESEmulatorThread::run ()
{
   QWidget* emulatorWidget = CDockWidgetRegistry::getWidget("Emulator");
   int scaleX;
   int scaleY;
   int scale;
   int emuX;
   int emuY;
   int32_t samplesAvailable;
   int32_t debuggerUpdateRate = EnvironmentSettingsDialog::debuggerUpdateRate();

   // Special case for 1Hz debugger update to match system mode.
   if ( debuggerUpdateRate == -1 )
   {
      if ( nesGetSystemMode() == MODE_NTSC )
      {
         debuggerUpdateRate = 60;
      }
      else
      {
         debuggerUpdateRate = 50;
      }
   }

   while ( m_isStarting || m_isRunning || m_isResetting || m_isPaused )
   {
      // Allow thread exit...
      if ( m_isTerminating )
      {
         break;
      }

      // Allow thread to keep going...
      if ( m_isStarting )
      {
         m_isStarting = false;
         m_isRunning = true;
         m_isPaused = false;

         // Re-enable breakpoints that were previously enabled...
         nesEnableBreakpoints(true);

         // Trigger UI updates...
         emit emulatorStarted();
      }

      // Properly coordinate NES reset with emulator...
      if ( m_isResetting )
      {
         // Reset emulated I/O devices...
         m_joy [ CONTROLLER1 ] = 0;
         m_joy [ CONTROLLER2 ] = 0;

         // Reset NES...
         nesReset(m_isSoftReset);

         // Re-enable breakpoints that were previously enabled...
         nesEnableBreakpoints(true);

         // Allow cartridge to be loaded...
         if ( m_pCartridge )
         {
            // Reload the cartridge image...
            // This internally causes a NES reset.
            loadCartridge();
            m_pCartridge = NULL;
         }

         // Trigger inspector updates...
         nesDisassemble();
         emit updateDebuggers();

         // Trigger UI updates...
         emit emulatorReset();

         // Don't *keep* resetting...
         m_isResetting = false;
      }

      // Pause?
      if ( m_isPaused || (m_pauseAfterFrames == 0) )
      {
         // Trigger inspector updates...
         nesDisassemble();
         emit updateDebuggers();

         // Trigger UI updates...
         emit emulatorPaused(m_showOnPause);

         m_isPaused = false;
         m_isRunning = false;
         if ( m_pauseAfterFrames == 0 )
         {
            m_pauseAfterFrames = -1;

            emit emulatorPausedAfter();
         }

         nesBreak();
      }

      // Run the NES...
      if ( m_isRunning )
      {
         // Re-enable breakpoints that were previously enabled...
         nesEnableBreakpoints(true);

         // Make sure breakpoint semaphore is on the precipice...
         nesBreakpointSemaphore.tryAcquire();

         // Run emulator for one frame...
         if ( emulatorWidget )
         {
            // Figure out where in the window the actual emulator display is.
            scaleX = emulatorWidget->geometry().width()/256;
            scaleY = emulatorWidget->geometry().height()/240;
            scale = (scaleX<scaleY)?scaleX:scaleY;
            if ( scale == 0 ) scale = 1;
            emuX = (emulatorWidget->geometry().x()+(emulatorWidget->geometry().width()/2))-(128*scale);
            if ( emuX < 0 ) emuX = 0;
            emuY = (emulatorWidget->geometry().y()+(emulatorWidget->geometry().height()/2))-(120*scale);
            if ( emuY < 0 ) emuY = 0;

            // Note, only need to check CCW for Vaus since both CCW and CW rotation are mouse
            // controlled if one of them is.
            if ( (EmulatorPrefsDialog::getControllerType(CONTROLLER1) == IO_Zapper) ||
                 ((EmulatorPrefsDialog::getControllerType(CONTROLLER1) == IO_Vaus) &&
                 (EmulatorPrefsDialog::getControllerMouseMap(CONTROLLER1,IO_Vaus_CCW))) )
            {
               nesSetControllerScreenPosition(CONTROLLER1,
                                              QCursor::pos().x(),
                                              QCursor::pos().y(),
                                              emuX,
                                              emuY,
                                              emuX+(256*scale),
                                              emuY+(240*scale));
            }
            if ( (EmulatorPrefsDialog::getControllerType(CONTROLLER2) == IO_Zapper) ||
                 ((EmulatorPrefsDialog::getControllerType(CONTROLLER2) == IO_Vaus) &&
                 (EmulatorPrefsDialog::getControllerMouseMap(CONTROLLER2,IO_Vaus_CCW))) )
            {
               nesSetControllerScreenPosition(CONTROLLER2,
                                              QCursor::pos().x(),
                                              QCursor::pos().y(),
                                              emuX,
                                              emuY,
                                              emuX+(256*scale),
                                              emuY+(240*scale));
            }
         }
         nesRun(m_joy);

         if ( m_pauseAfterFrames != -1 )
         {
            m_pauseAfterFrames--;
         }

         emit emulatedFrame();

         if ( m_debugFrame )
         {
            m_debugFrame--;
         }
         if ( (!m_debugFrame) && (debuggerUpdateRate) )
         {
            m_debugFrame = debuggerUpdateRate;
            if ( nesIsDebuggable() )
            {
               emit updateDebuggers();
            }
         }
      }
   }

   return;
}

bool NESEmulatorThread::serialize(QDomDocument& doc, QDomNode& node)
{
   QString cartMem;
   QString ppuMem;
   char byte[3];
   int  idx;

   // Save state.
   QDomElement saveElement = addElement ( doc, node, "save" );
   // Serialize the CPU state.
   QDomElement cpuElement = addElement( doc, saveElement, "cpu" );

   // Serialize the CPU registers.
   QDomElement cpuRegsElement = addElement(doc,cpuElement,"registers");

   cpuRegsElement.setAttribute("pc",nesGetCPURegister(CPU_PC));
   cpuRegsElement.setAttribute("sp",nesGetCPURegister(CPU_SP));
   cpuRegsElement.setAttribute("a",nesGetCPURegister(CPU_A));
   cpuRegsElement.setAttribute("x",nesGetCPURegister(CPU_X));
   cpuRegsElement.setAttribute("y",nesGetCPURegister(CPU_Y));
   cpuRegsElement.setAttribute("f",nesGetCPURegister(CPU_F));

   // Serialize the CPU memory.
   QDomElement cpuMemElement = addElement(doc,cpuElement,"memory");
   QDomCDATASection cpuMemDataSect;
   QString cpuMem;

   for ( idx = 0; idx < MEM_2KB; idx++ )
   {
      sprintf(byte,"%02X",nesGetCPUMemory(idx));
      cpuMem += byte;
   }
   cpuMemDataSect = doc.createCDATASection(cpuMem);
   cpuMemElement.appendChild(cpuMemDataSect);

   // Serialize the PPU state.
   QDomElement ppuElement = addElement( doc, saveElement, "ppu" );

   // Serialize the PPU registers.
   QDomElement ppuRegsElement = addElement(doc,ppuElement,"registers");

   QDomCDATASection ppuRegsDataSect;
   QString ppuRegs;

   for ( idx = 0; idx < MEM_8B; idx++ )
   {
      sprintf(byte,"%02X",nesGetPPURegister(0x2000+idx));
      ppuRegs += byte;
   }
   ppuRegsDataSect = doc.createCDATASection(ppuRegs);
   ppuRegsElement.appendChild(ppuRegsDataSect);

   // Serialize the PPU memory.
   QDomElement ppuMemElement = addElement(doc,ppuElement,"memory");
   QDomCDATASection ppuMemDataSect;

   ppuMem.clear();
   for ( idx = 0; idx < MEM_8KB; idx++ )
   {
      sprintf(byte,"%02X",nesGetPPUMemory(0x2000+idx));
      ppuMem += byte;
   }
   ppuMemDataSect = doc.createCDATASection(ppuMem);
   ppuMemElement.appendChild(ppuMemDataSect);

   // Serialize the PPU OAM memory.
   QDomElement ppuOamMemElement = addElement(doc,ppuElement,"oam");
   QDomCDATASection ppuOamMemDataSect;

   ppuMem.clear();
   for ( idx = 0; idx < MEM_256B; idx++ )
   {
      sprintf(byte,"%02X",nesGetPPUOAM(idx));
      ppuMem += byte;
   }
   ppuOamMemDataSect = doc.createCDATASection(ppuMem);
   ppuOamMemElement.appendChild(ppuOamMemDataSect);

   // Serialize the APU state.
   QDomElement apuElement = addElement( doc, saveElement, "apu" );

   // Serialize the APU registers.
   QDomElement apuRegsElement = addElement(doc,apuElement,"registers");

   QDomCDATASection apuRegsDataSect;
   QString apuRegs;

   for ( idx = 0; idx < MEM_32B; idx++ )
   {
      sprintf(byte,"%02X",nesGetAPURegister(0x4000+idx));
      apuRegs += byte;
   }
   apuRegsDataSect = doc.createCDATASection(apuRegs);
   apuRegsElement.appendChild(apuRegsDataSect);

   // Serialize the Cartridge state.
   QDomElement cartElement = addElement( doc, saveElement, "cartridge" );

   // Serialize the Cartridge SRAM memory.
   QDomElement cartSramMemElement = addElement( doc, cartElement, "sram" );
   QDomCDATASection cartSramMemDataSect;

   cartMem.clear();
   for ( idx = 0; idx < MEM_64KB; idx++ )
   {
      sprintf(byte,"%02X",nesGetSRAMDataPhysical(idx));
      cartMem += byte;
   }
   cartSramMemDataSect = doc.createCDATASection(cartMem);
   cartSramMemElement.appendChild(cartSramMemDataSect);

   // Serialize the Cartridge EXRAM memory.
   QDomElement cartExramMemElement = addElement( doc, cartElement, "exram" );
   QDomCDATASection cartExramMemDataSect;

   cartMem.clear();
   for ( idx = 0; idx < MEM_1KB; idx++ )
   {
      sprintf(byte,"%02X",nesGetEXRAMData(0x5C00+idx));
      cartMem += byte;
   }
   cartExramMemDataSect = doc.createCDATASection(cartMem);
   cartExramMemElement.appendChild(cartExramMemDataSect);

   return true;
}

bool NESEmulatorThread::serializeContent(QFile& fileOut)
{
   QByteArray bytes;
   int idx;

   for ( idx = 0; idx < MEM_64KB; idx++ )
   {
      bytes += nesGetSRAMDataPhysical(idx);
   }

   fileOut.write(bytes);
}

bool NESEmulatorThread::deserialize(QDomDocument& doc, QDomNode& /*node*/, QString& /*errors*/)
{
   // Loop through the child elements and process the ones we find
   QDomElement saveStateElement = doc.documentElement();
   QDomNode child = saveStateElement.firstChild();
   QDomNode childsChild;
   QDomNode cdataNode;
   QDomElement childsElement;
   QDomCDATASection cdataSection;
   QString cdataString;
   int idx;
   char byte;

   do
   {
#if 0
// CPTODO: For now we save lots of crap in serialize but only
//         restore the SRAM content on deserialize.  Having a
//         cycle-perfect "pick up where i left off" option just
//         isn't very feasible and not really necessary with the
//         IDE.  The standalone emulator may do it differently.
      if (child.nodeName() == "cpu")
      {
         childsChild = child.firstChild();
         do
         {
            if ( childsChild.nodeName() == "registers" )
            {
               childsElement = childsChild.toElement();
               nesSetCPUProgramCounter(childsElement.attribute("pc").toInt());
               nesSetCPUStackPointer(childsElement.attribute("sp").toInt());
               nesSetCPUAccumulator(childsElement.attribute("a").toInt());
               nesSetCPUIndexX(childsElement.attribute("x").toInt());
               nesSetCPUIndexY(childsElement.attribute("y").toInt());
               nesSetCPUFlags(childsElement.attribute("f").toInt());
            }
            else if ( childsChild.nodeName() == "memory" )
            {
               cdataNode = childsChild.firstChild();
               cdataSection = cdataNode.toCDATASection();
               cdataString = cdataSection.data();
               for ( idx = 0; idx < MEM_2KB; idx++ )
               {
                  byte = cdataString.left(2).toInt(0,16);
                  cdataString = cdataString.right(cdataString.length()-2);
                  nesSetCPUMemory(idx,byte);
               }
            }
         }
         while (!(childsChild = childsChild.nextSibling()).isNull());
      }
      else if (child.nodeName() == "ppu")
      {
         childsChild = child.firstChild();
         do
         {
            if ( childsChild.nodeName() == "registers" )
            {
               cdataNode = childsChild.firstChild();
               cdataSection = cdataNode.toCDATASection();
               cdataString = cdataSection.data();
               for ( idx = 0; idx < MEM_8B; idx++ )
               {
                  byte = cdataString.left(2).toInt(0,16);
                  cdataString = cdataString.right(cdataString.length()-2);
                  nesSetPPURegister(0x2000+idx,byte);
               }
            }
            else if ( childsChild.nodeName() == "memory" )
            {
               cdataNode = childsChild.firstChild();
               cdataSection = cdataNode.toCDATASection();
               cdataString = cdataSection.data();
               for ( idx = 0; idx < MEM_8KB; idx++ )
               {
                  byte = cdataString.left(2).toInt(0,16);
                  cdataString = cdataString.right(cdataString.length()-2);
                  nesSetPPUMemory(0x2000+idx,byte);
               }
            }
            else if ( childsChild.nodeName() == "oam" )
            {
               cdataNode = childsChild.firstChild();
               cdataSection = cdataNode.toCDATASection();
               cdataString = cdataSection.data();
               for ( idx = 0; idx < MEM_256B; idx++ )
               {
                  byte = cdataString.left(2).toInt(0,16);
                  cdataString = cdataString.right(cdataString.length()-2);
                  nesSetPPUOAM(idx&3,idx>>2,byte);
               }
            }
         }
         while (!(childsChild = childsChild.nextSibling()).isNull());
      }
      else if (child.nodeName() == "apu")
      {
      }
      else if (child.nodeName() == "cartridge")
      {
      }
#endif
      if (child.nodeName() == "cartridge")
      {
         childsChild = child.firstChild();
         do
         {
            if ( childsChild.nodeName() == "sram" )
            {
               cdataNode = childsChild.firstChild();
               cdataSection = cdataNode.toCDATASection();
               cdataString = cdataSection.data();
               for ( idx = 0; idx < MEM_64KB; idx++ )
               {
                  byte = cdataString.left(2).toInt(0,16);
                  cdataString = cdataString.right(cdataString.length()-2);
                  nesLoadSRAMDataPhysical(idx,byte);
               }
            }
         }
         while (!(childsChild = childsChild.nextSibling()).isNull());
      }
   }
   while (!(child = child.nextSibling()).isNull());

   emit updateDebuggers();

   return true;
}

bool NESEmulatorThread::deserializeContent(QFile& fileIn)
{
   QByteArray bytes;
   int idx;
   char zero = 0;

   bytes = fileIn.readAll();

   if ( bytes.count() != MEM_64KB )
   {
      if ( bytes.count() < MEM_64KB )
      {
         for ( idx = bytes.count(); idx < MEM_64KB; idx++ )
         {
            bytes.append(zero);
         }
      }

      QString str;
      str = "The save file found for this ROM:\n\n";
      str += fileIn.fileName();
      str += "\n\nis not 64KB in size and may not be\n";
      str += "valid.  Game save data may be lost.";
//      QMessageBox::warning(0,"Save file corrupted?",str);
      bytes.truncate(MEM_64KB);
   }

   for ( idx = 0; idx < bytes.count(); idx++ )
   {
      nesSetSRAMDataPhysical(idx,bytes.at(idx));
   }
}
