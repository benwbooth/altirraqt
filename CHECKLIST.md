# Altirra Qt File Checklist

Reference tree: `/home/ben/src/altirra-reference`

Strategy: literal file-by-file transliteration from Win32-specific APIs to cross-platform Qt equivalents, keeping file ordering and module boundaries as close to upstream as practical.


## src/ATAppBase/h
- [ ] `src/ATAppBase/h/stdafx.h`

## src/ATAppBase/source
- [ ] ~~`src/ATAppBase/source/crthooks.cpp`~~ — Win32 CRT hooks, skip (Qt/libc handles init)
- [ ] ~~`src/ATAppBase/source/exceptionfilter.cpp`~~ — Win32 SEH filter, skip
- [x] `src/ATAppBase/source/stdafx.cpp`

## src/ATAudio/h/at/ataudio
- [ ] `src/ATAudio/h/at/ataudio/audiosamplebuffer.h`

## src/ATAudio/h
- [ ] `src/ATAudio/h/stdafx.h`

## src/ATAudio/source
- [ ] `src/ATAudio/source/audioconvolutionplayer.cpp`
- [ ] `src/ATAudio/source/audiofilters.cpp`
- [ ] `src/ATAudio/source/audiooutput.cpp`
- [ ] `src/ATAudio/source/audiooutwasapi.cpp`
- [ ] `src/ATAudio/source/audiooutwaveout.cpp`
- [ ] `src/ATAudio/source/audiooutxa2.cpp`
- [ ] `src/ATAudio/source/audiosamplebuffer.cpp`
- [ ] `src/ATAudio/source/audiosampleplayer.cpp`
- [ ] `src/ATAudio/source/audiosamplepool.cpp`
- [ ] `src/ATAudio/source/pokey.cpp`
- [ ] `src/ATAudio/source/pokeyrenderer.cpp`
- [ ] `src/ATAudio/source/pokeysavestate.cpp`
- [ ] `src/ATAudio/source/pokeytables.cpp`
- [ ] `src/ATAudio/source/stdafx.cpp`

## src/ATCPU/h
- [ ] `src/ATCPU/h/co65802.inl`
- [ ] `src/ATCPU/h/co6809.inl`
- [ ] `src/ATCPU/h/coz80.inl`
- [ ] `src/ATCPU/h/stdafx.h`

## src/ATCPU/source
- [ ] `src/ATCPU/source/co6502.cpp`
- [ ] `src/ATCPU/source/co6502machine.cpp`
- [ ] `src/ATCPU/source/co65802.cpp`
- [ ] `src/ATCPU/source/co6809.cpp`
- [ ] `src/ATCPU/source/co8048.cpp`
- [ ] `src/ATCPU/source/co8051.cpp`
- [ ] `src/ATCPU/source/coz80.cpp`
- [ ] `src/ATCPU/source/decode6502.cpp`
- [ ] `src/ATCPU/source/decode65816.cpp`
- [ ] `src/ATCPU/source/decode6809.cpp`
- [ ] `src/ATCPU/source/decodez80.cpp`
- [ ] `src/ATCPU/source/memorymap.cpp`
- [ ] `src/ATCPU/source/stdafx.cpp`

## src/ATCompiler/h
- [ ] `src/ATCompiler/h/stdafx.h`
- [ ] `src/ATCompiler/h/utils.h`

## src/ATCompiler/source
- [ ] `src/ATCompiler/source/lzpack.cpp`
- [ ] `src/ATCompiler/source/lzunpack.cpp`
- [ ] `src/ATCompiler/source/main.cpp`
- [ ] `src/ATCompiler/source/makeexports.cpp`
- [ ] `src/ATCompiler/source/makereloc.cpp`
- [ ] `src/ATCompiler/source/makereloc2.cpp`
- [ ] `src/ATCompiler/source/makereloc3.cpp`
- [ ] `src/ATCompiler/source/makereloc4.cpp`
- [ ] `src/ATCompiler/source/mkfsdos2.cpp`
- [ ] `src/ATCompiler/source/stdafx.cpp`
- [ ] `src/ATCompiler/source/utils.cpp`

## src/ATCore/h/at/atcore/internal
- [ ] `src/ATCore/h/at/atcore/internal/checksum.h`
- [ ] `src/ATCore/h/at/atcore/internal/timerserviceimpl_win32.h`

## src/ATCore/h
- [ ] `src/ATCore/h/stdafx.h`

## src/ATCore/source
- [x] `src/ATCore/source/address.cpp`
- [x] `src/ATCore/source/asyncdispatcherimpl.cpp`
- [x] `src/ATCore/source/atascii.cpp`
- [x] `src/ATCore/source/bussignal.cpp`
- [x] `src/ATCore/source/checksum.cpp`
- [x] `src/ATCore/source/checksum_arm64.cpp`
- [x] `src/ATCore/source/checksum_intrin.cpp`
- [x] `src/ATCore/source/configvar.cpp`
- [x] `src/ATCore/source/consoleoutput.cpp`
- [x] `src/ATCore/source/crc.cpp`
- [x] `src/ATCore/source/decmath.cpp`
- [x] `src/ATCore/source/deferredevent.cpp`
- [x] `src/ATCore/source/deviceimpl.cpp`
- [x] `src/ATCore/source/deviceparentimpl.cpp`
- [x] `src/ATCore/source/devicesioimpl.cpp`
- [x] `src/ATCore/source/devicestorageimpl.cpp`
- [x] `src/ATCore/source/enumparse.cpp`
- [x] `src/ATCore/source/fft.cpp`
- [x] `src/ATCore/source/fft_avx2.cpp`
- [x] `src/ATCore/source/fft_neon.cpp`
- [x] `src/ATCore/source/fft_scalar.cpp`
- [x] `src/ATCore/source/fft_sse2.cpp`
- [x] `src/ATCore/source/logging.cpp`
- [x] `src/ATCore/source/md5.cpp`
- [x] `src/ATCore/source/memoryutils.cpp`
- [x] `src/ATCore/source/notifylist.cpp`
- [x] `src/ATCore/source/profile.cpp`
- [x] `src/ATCore/source/progress.cpp`
- [x] `src/ATCore/source/propertyset.cpp`
- [x] `src/ATCore/source/randomization.cpp`
- [x] `src/ATCore/source/savestate.cpp`
- [x] `src/ATCore/source/scheduler.cpp`
- [x] `src/ATCore/source/serialization.cpp`
- [x] `src/ATCore/source/sioutils.cpp`
- [x] `src/ATCore/source/snapshotimpl.cpp`
- [x] `src/ATCore/source/stdafx.cpp`
- [ ] ~~`src/ATCore/source/timerserviceimpl_win32.cpp`~~ — Win32-only, POSIX replacement pending
- [x] `src/ATCore/source/vfs.cpp`

## src/ATDebugger/h/at/atdebugger/internal
- [ ] `src/ATDebugger/h/at/atdebugger/internal/debugdevice.h`
- [ ] `src/ATDebugger/h/at/atdebugger/internal/symstore.h`

## src/ATDebugger/h
- [ ] `src/ATDebugger/h/stdafx.h`

## src/ATDebugger/source
- [ ] `src/ATDebugger/source/breakpointsimpl.cpp`
- [ ] `src/ATDebugger/source/debugdevice.cpp`
- [ ] `src/ATDebugger/source/defsymbols.cpp`
- [ ] `src/ATDebugger/source/historytree.cpp`
- [ ] `src/ATDebugger/source/historytreebuilder.cpp`
- [ ] `src/ATDebugger/source/stdafx.cpp`
- [ ] `src/ATDebugger/source/symbolio.cpp`
- [ ] `src/ATDebugger/source/symbols.cpp`
- [ ] `src/ATDebugger/source/target.cpp`

## src/ATDevices/autogen
- [ ] `src/ATDevices/autogen/exeloader-0700.inl`
- [ ] `src/ATDevices/autogen/exeloader-hispeed-0700.inl`
- [ ] `src/ATDevices/autogen/exeloader-hispeed.inl`
- [ ] `src/ATDevices/autogen/exeloader-nobasic.inl`
- [ ] `src/ATDevices/autogen/exeloader.inl`

## src/ATDevices/h/at/atdevices
- [ ] `src/ATDevices/h/at/atdevices/blockdevwritefilter.h`
- [ ] `src/ATDevices/h/at/atdevices/computereyes.h`
- [ ] `src/ATDevices/h/at/atdevices/corvus.h`
- [ ] `src/ATDevices/h/at/atdevices/dongle.h`
- [ ] `src/ATDevices/h/at/atdevices/parfilewriter.h`
- [ ] `src/ATDevices/h/at/atdevices/supersalt.h`
- [ ] `src/ATDevices/h/at/atdevices/videogenerator.h`

## src/ATDevices/h
- [ ] `src/ATDevices/h/diskdrive.h`
- [ ] `src/ATDevices/h/exeloader.h`
- [ ] `src/ATDevices/h/stdafx.h`

## src/ATDevices/source
- [ ] `src/ATDevices/source/blockdevwritefilter.cpp`
- [ ] `src/ATDevices/source/computereyes.cpp`
- [ ] `src/ATDevices/source/corvus.cpp`
- [ ] `src/ATDevices/source/devices.cpp`
- [ ] `src/ATDevices/source/diskdrive.cpp`
- [ ] `src/ATDevices/source/dongle.cpp`
- [ ] `src/ATDevices/source/exeloader.cpp`
- [ ] `src/ATDevices/source/loopback.cpp`
- [ ] `src/ATDevices/source/modemsound.cpp`
- [ ] `src/ATDevices/source/netserial.cpp`
- [ ] `src/ATDevices/source/paralleltoserial.cpp`
- [ ] `src/ATDevices/source/parfilewriter.cpp`
- [ ] `src/ATDevices/source/serialsplitter.cpp`
- [ ] `src/ATDevices/source/sioclock.cpp`
- [ ] `src/ATDevices/source/stdafx.cpp`
- [ ] `src/ATDevices/source/supersalt.cpp`
- [ ] `src/ATDevices/source/testdevicesiohs.cpp`
- [ ] `src/ATDevices/source/testdevicesiopoll.cpp`
- [ ] `src/ATDevices/source/videogenerator.cpp`

## src/ATEmulation/h
- [ ] `src/ATEmulation/h/stdafx.h`

## src/ATEmulation/source
- [ ] `src/ATEmulation/source/acia.cpp`
- [ ] `src/ATEmulation/source/acia6850.cpp`
- [ ] `src/ATEmulation/source/crtc.cpp`
- [ ] `src/ATEmulation/source/ctc.cpp`
- [ ] `src/ATEmulation/source/diskutils.cpp`
- [ ] `src/ATEmulation/source/eeprom.cpp`
- [ ] `src/ATEmulation/source/flash.cpp`
- [ ] `src/ATEmulation/source/phone.cpp`
- [ ] `src/ATEmulation/source/riot.cpp`
- [ ] `src/ATEmulation/source/rtcds1305.cpp`
- [ ] `src/ATEmulation/source/rtcmcp7951x.cpp`
- [ ] `src/ATEmulation/source/rtcv3021.cpp`
- [ ] `src/ATEmulation/source/scsi.cpp`
- [ ] `src/ATEmulation/source/stdafx.cpp`
- [ ] `src/ATEmulation/source/via.cpp`

## src/ATHelpBuilder
- [ ] `src/ATHelpBuilder/AssemblyInfo.cpp`

## src/ATHelpBuilder/h
- [ ] `src/ATHelpBuilder/h/stdafx.h`

## src/ATHelpBuilder/source
- [ ] `src/ATHelpBuilder/source/ATHelpBuilder.cpp`
- [ ] `src/ATHelpBuilder/source/stdafx.cpp`

## src/ATIO/autogen
- [ ] `src/ATIO/autogen/bootsecdos2.inl`

## src/ATIO/h/at/atio
- [ ] `src/ATIO/h/at/atio/audioreaderflac.h`
- [ ] `src/ATIO/h/at/atio/audioreaderwav.h`
- [ ] `src/ATIO/h/at/atio/audioutils.h`
- [ ] `src/ATIO/h/at/atio/cassetteaudiofilters.h`
- [ ] `src/ATIO/h/at/atio/tapepiecetable.h`
- [ ] `src/ATIO/h/at/atio/vorbisbitreader.h`
- [ ] `src/ATIO/h/at/atio/vorbisdecoder.h`
- [ ] `src/ATIO/h/at/atio/vorbismisc.h`

## src/ATIO/h
- [ ] `src/ATIO/h/stdafx.h`

## src/ATIO/source
- [ ] `src/ATIO/source/atfs.cpp`
- [ ] `src/ATIO/source/audioreader.cpp`
- [ ] `src/ATIO/source/audioreaderflac.cpp`
- [ ] `src/ATIO/source/audioreaderflac_arm64.cpp`
- [ ] `src/ATIO/source/audioreaderflac_x86.cpp`
- [ ] `src/ATIO/source/audioreadervorbis.cpp`
- [ ] `src/ATIO/source/audioreaderwav.cpp`
- [ ] `src/ATIO/source/audioutils.cpp`
- [ ] `src/ATIO/source/blobimage.cpp`
- [ ] `src/ATIO/source/cartridgeimage.cpp`
- [ ] `src/ATIO/source/cartridgetypes.cpp`
- [ ] `src/ATIO/source/cassetteaudiofilters.cpp`
- [ ] `src/ATIO/source/cassetteblock.cpp`
- [ ] `src/ATIO/source/cassettedecoder.cpp`
- [ ] `src/ATIO/source/cassetteimage.cpp`
- [ ] `src/ATIO/source/diskfs.cpp`
- [ ] `src/ATIO/source/diskfsarc.cpp`
- [ ] `src/ATIO/source/diskfscpm.cpp`
- [ ] `src/ATIO/source/diskfsdos2.cpp`
- [ ] `src/ATIO/source/diskfsdos3.cpp`
- [ ] `src/ATIO/source/diskfssdx2.cpp`
- [ ] `src/ATIO/source/diskfssdx2util.cpp`
- [ ] `src/ATIO/source/diskfsutil.cpp`
- [ ] `src/ATIO/source/diskimage.cpp`
- [ ] `src/ATIO/source/image.cpp`
- [ ] `src/ATIO/source/partitiondiskview.cpp`
- [ ] `src/ATIO/source/partitiontable.cpp`
- [ ] `src/ATIO/source/programimage.cpp`
- [ ] `src/ATIO/source/savestate.cpp`
- [ ] `src/ATIO/source/stdafx.cpp`
- [ ] `src/ATIO/source/tapepiecetable.cpp`
- [ ] `src/ATIO/source/vorbisbitreader.cpp`
- [ ] `src/ATIO/source/vorbisdecoder.cpp`
- [ ] `src/ATIO/source/vorbismisc.cpp`
- [ ] `src/ATIO/source/wav.cpp`

## src/ATNativeUI/h
- [ ] `src/ATNativeUI/h/stdafx.h`

## src/ATNativeUI/res
- [ ] `src/ATNativeUI/res/atnativeui.rc`
- [ ] `src/ATNativeUI/res/resource.h`

## src/ATNativeUI/source
- [ ] `src/ATNativeUI/source/acceleditdialog.cpp`
- [ ] `src/ATNativeUI/source/accessibility_win32.cpp`
- [ ] `src/ATNativeUI/source/canvas_win32.cpp`
- [ ] `src/ATNativeUI/source/controlstyles.cpp`
- [ ] `src/ATNativeUI/source/debug_win32.cpp`
- [ ] `src/ATNativeUI/source/dialog.cpp`
- [ ] `src/ATNativeUI/source/dragdrop.cpp`
- [ ] `src/ATNativeUI/source/genericdialog.cpp`
- [ ] `src/ATNativeUI/source/hotkeyexcontrol.cpp`
- [ ] `src/ATNativeUI/source/hotkeyscandialog.cpp`
- [ ] `src/ATNativeUI/source/messagedispatcher.cpp`
- [ ] `src/ATNativeUI/source/messageloop.cpp`
- [ ] `src/ATNativeUI/source/nativewindowproxy.cpp`
- [ ] `src/ATNativeUI/source/progress.cpp`
- [ ] `src/ATNativeUI/source/stdafx.cpp`
- [ ] `src/ATNativeUI/source/theme.cpp`
- [ ] `src/ATNativeUI/source/uiframe.cpp`
- [ ] `src/ATNativeUI/source/uinativewindow.cpp`
- [ ] `src/ATNativeUI/source/uipane.cpp`
- [ ] `src/ATNativeUI/source/uipanedialog.cpp`
- [ ] `src/ATNativeUI/source/uiproxies.cpp`
- [ ] `src/ATNativeUI/source/utils.cpp`

## src/ATNetwork/h/at/atnetwork/internal
- [ ] `src/ATNetwork/h/at/atnetwork/internal/dhcpd.h`

## src/ATNetwork/h
- [ ] `src/ATNetwork/h/ipstack.h`
- [ ] `src/ATNetwork/h/stdafx.h`
- [ ] `src/ATNetwork/h/tcpstack.h`
- [ ] `src/ATNetwork/h/udpstack.h`

## src/ATNetwork/source
- [ ] `src/ATNetwork/source/dhcpd.cpp`
- [ ] `src/ATNetwork/source/ethernetbus.cpp`
- [ ] `src/ATNetwork/source/ethernetframe.cpp`
- [ ] `src/ATNetwork/source/gatewayserver.cpp`
- [ ] `src/ATNetwork/source/ipstack.cpp`
- [ ] `src/ATNetwork/source/socket.cpp`
- [ ] `src/ATNetwork/source/stdafx.cpp`
- [ ] `src/ATNetwork/source/tcp.cpp`
- [ ] `src/ATNetwork/source/tcpstack.cpp`
- [ ] `src/ATNetwork/source/udp.cpp`
- [ ] `src/ATNetwork/source/udpstack.cpp`

## src/ATNetworkSockets/h/at/atnetworksockets/internal
- [ ] `src/ATNetworkSockets/h/at/atnetworksockets/internal/lookupworker.h`
- [ ] `src/ATNetworkSockets/h/at/atnetworksockets/internal/socketutils.h`
- [ ] `src/ATNetworkSockets/h/at/atnetworksockets/internal/socketworker.h`
- [ ] `src/ATNetworkSockets/h/at/atnetworksockets/internal/vxlantunnel.h`
- [ ] `src/ATNetworkSockets/h/at/atnetworksockets/internal/worker.h`

## src/ATNetworkSockets/h
- [ ] `src/ATNetworkSockets/h/stdafx.h`

## src/ATNetworkSockets/source
- [ ] `src/ATNetworkSockets/source/lookupworker.cpp`
- [ ] `src/ATNetworkSockets/source/nativesockets.cpp`
- [ ] `src/ATNetworkSockets/source/socketutils_win32.cpp`
- [ ] `src/ATNetworkSockets/source/socketworker.cpp`
- [ ] `src/ATNetworkSockets/source/stdafx.cpp`
- [ ] `src/ATNetworkSockets/source/vxlantunnel.cpp`
- [ ] `src/ATNetworkSockets/source/worker.cpp`

## src/ATTest/h
- [ ] `src/ATTest/h/blob.h`
- [ ] `src/ATTest/h/stdafx.h`
- [ ] `src/ATTest/h/test.h`

## src/ATTest/source
- [ ] `src/ATTest/source/TestCoProc_6502.cpp`
- [ ] `src/ATTest/source/TestCore_Checksum.cpp`
- [ ] `src/ATTest/source/TestCore_FFT.cpp`
- [ ] `src/ATTest/source/TestCore_MD5.cpp`
- [ ] `src/ATTest/source/TestCore_VFS.cpp`
- [ ] `src/ATTest/source/TestDebugger_HistoryTree.cpp`
- [ ] `src/ATTest/source/TestDebugger_SymbolIO.cpp`
- [ ] `src/ATTest/source/TestEmu_PCLink.cpp`
- [ ] `src/ATTest/source/TestEmu_PokeyPots.cpp`
- [ ] `src/ATTest/source/TestEmu_PokeyTimers.cpp`
- [ ] `src/ATTest/source/TestIO_DiskImage.cpp`
- [ ] `src/ATTest/source/TestIO_FLAC.cpp`
- [ ] `src/ATTest/source/TestIO_TapeWrite.cpp`
- [ ] `src/ATTest/source/TestIO_VirtFAT32.cpp`
- [ ] `src/ATTest/source/TestIO_Vorbis.cpp`
- [ ] `src/ATTest/source/TestKasumi_Pixmap.cpp`
- [ ] `src/ATTest/source/TestKasumi_Resampler.cpp`
- [ ] `src/ATTest/source/TestKasumi_Uberblit.cpp`
- [ ] `src/ATTest/source/TestMisc_TTF.cpp`
- [ ] `src/ATTest/source/TestNet_NativeDatagramLiveTest.cpp`
- [ ] `src/ATTest/source/TestNet_NativeSockets.cpp`
- [ ] `src/ATTest/source/TestSystem_CRC.cpp`
- [ ] `src/ATTest/source/TestSystem_Exception.cpp`
- [ ] `src/ATTest/source/TestSystem_FastVector.cpp`
- [ ] `src/ATTest/source/TestSystem_Filesys.cpp`
- [ ] `src/ATTest/source/TestSystem_Function.cpp`
- [ ] `src/ATTest/source/TestSystem_HashMap.cpp`
- [ ] `src/ATTest/source/TestSystem_HashSet.cpp`
- [ ] `src/ATTest/source/TestSystem_Int128.cpp`
- [ ] `src/ATTest/source/TestSystem_Math.cpp`
- [ ] `src/ATTest/source/TestSystem_VecMath.cpp`
- [ ] `src/ATTest/source/TestSystem_Vector.cpp`
- [ ] `src/ATTest/source/TestSystem_Zip.cpp`
- [ ] `src/ATTest/source/TestTrace_CPU.cpp`
- [ ] `src/ATTest/source/TestTrace_IO.cpp`
- [ ] `src/ATTest/source/TestUI_TextDOM.cpp`
- [ ] `src/ATTest/source/blob.cpp`
- [ ] `src/ATTest/source/main.cpp`
- [ ] `src/ATTest/source/stdafx.cpp`
- [ ] `src/ATTest/source/utils.cpp`

## src/ATUI/h
- [ ] `src/ATUI/h/stdafx.h`

## src/ATUI/source
- [ ] `src/ATUI/source/stdafx.cpp`
- [ ] `src/ATUI/source/uiaccessibility.cpp`
- [ ] `src/ATUI/source/uianchor.cpp`
- [ ] `src/ATUI/source/uicommandmanager.cpp`
- [ ] `src/ATUI/source/uicontainer.cpp`
- [ ] `src/ATUI/source/uidrawingutils.cpp`
- [ ] `src/ATUI/source/uimanager.cpp`
- [ ] `src/ATUI/source/uimenulist.cpp`
- [ ] `src/ATUI/source/uiwidget.cpp`

## src/ATUIControls/h
- [ ] `src/ATUIControls/h/stdafx.h`

## src/ATUIControls/source
- [ ] `src/ATUIControls/source/stdafx.cpp`
- [ ] `src/ATUIControls/source/uibutton.cpp`
- [ ] `src/ATUIControls/source/uiimage.cpp`
- [ ] `src/ATUIControls/source/uilabel.cpp`
- [ ] `src/ATUIControls/source/uilistview.cpp`
- [ ] `src/ATUIControls/source/uislider.cpp`
- [ ] `src/ATUIControls/source/uitextedit.cpp`

## src/ATVM/h
- [ ] `src/ATVM/h/stdafx.h`

## src/ATVM/source
- [ ] `src/ATVM/source/compiler.cpp`
- [ ] `src/ATVM/source/stdafx.cpp`
- [ ] `src/ATVM/source/vm.cpp`

## src/Altirra/autobuild_default
- [ ] `src/Altirra/autobuild_default/atversionrc.h`
- [ ] `src/Altirra/autobuild_default/version.h`

## src/Altirra/autogen
- [ ] `src/Altirra/autogen/pbidisk.inl`
- [ ] `src/Altirra/autogen/playsap-b.inl`
- [ ] `src/Altirra/autogen/playsap-c.inl`
- [ ] `src/Altirra/autogen/playsap-d-ntsc.inl`
- [ ] `src/Altirra/autogen/playsap-d-pal.inl`
- [ ] `src/Altirra/autogen/playsap-r.inl`
- [ ] `src/Altirra/autogen/playvgm.inl`
- [ ] `src/Altirra/autogen/xep80_font_internal.inl`
- [ ] `src/Altirra/autogen/xep80_font_intl.inl`
- [ ] `src/Altirra/autogen/xep80_font_normal.inl`

## src/Altirra/h
- [ ] `src/Altirra/h/1025full.h`
- [ ] `src/Altirra/h/1029full.h`
- [ ] `src/Altirra/h/1030full.h`
- [ ] `src/Altirra/h/1090.h`
- [ ] `src/Altirra/h/1400xl.h`
- [ ] `src/Altirra/h/1450xld.h`
- [ ] `src/Altirra/h/820full.h`
- [ ] `src/Altirra/h/850full.h`
- [ ] `src/Altirra/h/antic.h`
- [ ] `src/Altirra/h/antic_sse2.inl`
- [ ] `src/Altirra/h/artifacting.h`
- [ ] `src/Altirra/h/artifacting_filters.h`
- [ ] `src/Altirra/h/asyncdownloader.h`
- [ ] `src/Altirra/h/audiomonitor.h`
- [ ] `src/Altirra/h/audiorawsource.h`
- [ ] `src/Altirra/h/audiosampleplayer.h`
- [ ] `src/Altirra/h/audiowriter.h`
- [ ] `src/Altirra/h/autosavemanager.h`
- [ ] `src/Altirra/h/aviwriter.h`
- [ ] `src/Altirra/h/bit3.h`
- [ ] `src/Altirra/h/bkptmanager.h`
- [ ] `src/Altirra/h/blackbox.h`
- [ ] `src/Altirra/h/blackboxfloppy.h`
- [ ] `src/Altirra/h/blockdevdiskadapter.h`
- [ ] `src/Altirra/h/blockdevvirtfat32.h`
- [ ] `src/Altirra/h/browser.h`
- [ ] `src/Altirra/h/callback.h`
- [ ] `src/Altirra/h/cartridge.h`
- [ ] `src/Altirra/h/cartridgeport.h`
- [ ] `src/Altirra/h/cassette.h`
- [ ] `src/Altirra/h/cheatengine.h`
- [ ] `src/Altirra/h/cmdhelpers.h`
- [ ] `src/Altirra/h/common_png.h`
- [ ] `src/Altirra/h/compatdb.h`
- [ ] `src/Altirra/h/compatedb.h`
- [ ] `src/Altirra/h/compatengine.h`
- [ ] `src/Altirra/h/console.h`
- [ ] `src/Altirra/h/constants.h`
- [ ] `src/Altirra/h/covox.h`
- [ ] `src/Altirra/h/cpu.h`
- [ ] `src/Altirra/h/cpuheatmap.h`
- [ ] `src/Altirra/h/cpuhookmanager.h`
- [ ] `src/Altirra/h/cpumachine.inl`
- [ ] `src/Altirra/h/cpumemory.h`
- [ ] `src/Altirra/h/cpustates.h`
- [ ] `src/Altirra/h/cputracer.h`
- [ ] `src/Altirra/h/cs8900a.h`
- [ ] `src/Altirra/h/customdevice.h`
- [ ] `src/Altirra/h/customdevice_win32.h`
- [ ] `src/Altirra/h/customdevicevmtypes.h`
- [ ] `src/Altirra/h/debugdisplay.h`
- [ ] `src/Altirra/h/debugger.h`
- [ ] `src/Altirra/h/debuggerexp.h`
- [ ] `src/Altirra/h/debuggerlog.h`
- [ ] `src/Altirra/h/debuggersettings.h`
- [ ] `src/Altirra/h/debugtarget.h`
- [ ] `src/Altirra/h/decmath.h`
- [ ] `src/Altirra/h/decode_png.h`
- [ ] `src/Altirra/h/devicemanager.h`
- [ ] `src/Altirra/h/devxcmdcopypaste.h`
- [ ] `src/Altirra/h/directorywatcher.h`
- [ ] `src/Altirra/h/disasm.h`
- [ ] `src/Altirra/h/disk.h`
- [ ] `src/Altirra/h/diskdrive815.h`
- [ ] `src/Altirra/h/diskdriveamdc.h`
- [ ] `src/Altirra/h/diskdriveatr8000.h`
- [ ] `src/Altirra/h/diskdrivefull.h`
- [ ] `src/Altirra/h/diskdrivefullbase.h`
- [ ] `src/Altirra/h/diskdriveindusgt.h`
- [ ] `src/Altirra/h/diskdrivepercom.h`
- [ ] `src/Altirra/h/diskdrivespeedyxf.h`
- [ ] `src/Altirra/h/diskdrivexf551.h`
- [ ] `src/Altirra/h/diskinterface.h`
- [ ] `src/Altirra/h/diskprofile.h`
- [ ] `src/Altirra/h/disktrace.h`
- [ ] `src/Altirra/h/diskvirtimagebase.h`
- [ ] `src/Altirra/h/dragoncart.h`
- [ ] `src/Altirra/h/encode_png.h`
- [ ] `src/Altirra/h/errordecode.h`
- [ ] `src/Altirra/h/fdc.h`
- [ ] `src/Altirra/h/firmwaredetect.h`
- [ ] `src/Altirra/h/firmwaremanager.h`
- [ ] `src/Altirra/h/gtia.h`
- [ ] `src/Altirra/h/gtia_neon.inl`
- [ ] `src/Altirra/h/gtia_sse2_intrin.inl`
- [ ] `src/Altirra/h/gtiarenderer.h`
- [ ] `src/Altirra/h/gtiarenderer_neon.inl`
- [ ] `src/Altirra/h/gtiarenderer_ssse3_intrin.inl`
- [ ] `src/Altirra/h/gtiatables.h`
- [ ] `src/Altirra/h/hlebasicloader.h`
- [ ] `src/Altirra/h/hleciohook.h`
- [ ] `src/Altirra/h/hlefastboothook.h`
- [ ] `src/Altirra/h/hlefpaccelerator.h`
- [ ] `src/Altirra/h/hleprogramloader.h`
- [ ] `src/Altirra/h/hleutils.h`
- [ ] `src/Altirra/h/hostdevice.h`
- [ ] `src/Altirra/h/hostdeviceutils.h`
- [ ] `src/Altirra/h/ide.h`
- [ ] `src/Altirra/h/idephysdisk.h`
- [ ] `src/Altirra/h/iderawimage.h`
- [ ] `src/Altirra/h/idevhdimage.h`
- [ ] `src/Altirra/h/inputcontroller.h`
- [ ] `src/Altirra/h/inputdefs.h`
- [ ] `src/Altirra/h/inputmanager.h`
- [ ] `src/Altirra/h/inputmap.h`
- [ ] `src/Altirra/h/irqcontroller.h`
- [ ] `src/Altirra/h/joystick.h`
- [ ] `src/Altirra/h/kerneldb.h`
- [ ] `src/Altirra/h/kmkjzide.h`
- [ ] `src/Altirra/h/mediamanager.h`
- [ ] `src/Altirra/h/memorymanager.h`
- [ ] `src/Altirra/h/mio.h`
- [ ] `src/Altirra/h/mmu.h`
- [ ] `src/Altirra/h/modem.h`
- [ ] `src/Altirra/h/modemtcp.h`
- [ ] `src/Altirra/h/multiplexer.h`
- [ ] `src/Altirra/h/myide.h`
- [ ] `src/Altirra/h/options.h`
- [ ] `src/Altirra/h/oshelper.h`
- [ ] `src/Altirra/h/ostracing.h`
- [ ] `src/Altirra/h/palettegenerator.h`
- [ ] `src/Altirra/h/palettesolver.h`
- [ ] `src/Altirra/h/pbi.h`
- [ ] `src/Altirra/h/pbidisk.h`
- [ ] `src/Altirra/h/pclink.h`
- [ ] `src/Altirra/h/pia.h`
- [ ] `src/Altirra/h/pokeysavecompat.h`
- [ ] `src/Altirra/h/pokeytrace.h`
- [ ] `src/Altirra/h/portmanager.h`
- [ ] `src/Altirra/h/printer.h`
- [ ] `src/Altirra/h/printer1020.h`
- [ ] `src/Altirra/h/printer1020font.h`
- [ ] `src/Altirra/h/printerbase.h`
- [ ] `src/Altirra/h/printerfont.h`
- [ ] `src/Altirra/h/printeroutput.h`
- [ ] `src/Altirra/h/printerttfencoder.h`
- [ ] `src/Altirra/h/printertypes.h`
- [ ] `src/Altirra/h/profiler.h`
- [ ] `src/Altirra/h/profilerui.h`
- [ ] `src/Altirra/h/rapidus.h`
- [ ] `src/Altirra/h/rs232.h`
- [ ] `src/Altirra/h/rtime8.h`
- [ ] `src/Altirra/h/sapconverter.h`
- [ ] `src/Altirra/h/sapwriter.h`
- [ ] `src/Altirra/h/savestate.h`
- [ ] `src/Altirra/h/savestateio.h`
- [ ] `src/Altirra/h/savestatetypes.h`
- [ ] `src/Altirra/h/scsidisk.h`
- [ ] `src/Altirra/h/sdrive.h`
- [ ] `src/Altirra/h/settings.h`
- [ ] `src/Altirra/h/side.h`
- [ ] `src/Altirra/h/side3.h`
- [ ] `src/Altirra/h/simeventmanager.h`
- [ ] `src/Altirra/h/simulator.h`
- [ ] `src/Altirra/h/sio2sd.h`
- [ ] `src/Altirra/h/siomanager.h`
- [ ] `src/Altirra/h/slightsid.h`
- [ ] `src/Altirra/h/soundboard.h`
- [ ] `src/Altirra/h/startuplogger.h`
- [ ] `src/Altirra/h/stdafx.h`
- [ ] `src/Altirra/h/textdom.h`
- [ ] `src/Altirra/h/texteditor.h`
- [ ] `src/Altirra/h/thepill.h`
- [ ] `src/Altirra/h/trace.h`
- [ ] `src/Altirra/h/tracecpu.h`
- [ ] `src/Altirra/h/tracefileencoding.h`
- [ ] `src/Altirra/h/tracefileformat.h`
- [ ] `src/Altirra/h/traceio.h`
- [ ] `src/Altirra/h/tracenative.h`
- [ ] `src/Altirra/h/tracetape.h`
- [ ] `src/Altirra/h/tracevideo.h`
- [ ] `src/Altirra/h/uiaccessors.h`
- [ ] `src/Altirra/h/uicalibrationscreen.h`
- [ ] `src/Altirra/h/uicaptionupdater.h`
- [ ] `src/Altirra/h/uiclipboard.h`
- [ ] `src/Altirra/h/uicommondialogs.h`
- [ ] `src/Altirra/h/uicompat.h`
- [ ] `src/Altirra/h/uiconfgeneric.h`
- [ ] `src/Altirra/h/uiconfirm.h`
- [ ] `src/Altirra/h/uiconfmodem.h`
- [ ] `src/Altirra/h/uidbgbreakpoints.h`
- [ ] `src/Altirra/h/uidbgcallstack.h`
- [ ] `src/Altirra/h/uidbgconsole.h`
- [ ] `src/Altirra/h/uidbgdebugdisplay.h`
- [ ] `src/Altirra/h/uidbgdisasm.h`
- [ ] `src/Altirra/h/uidbghistory.h`
- [ ] `src/Altirra/h/uidbgmemory.h`
- [ ] `src/Altirra/h/uidbgpane.h`
- [ ] `src/Altirra/h/uidbgprinteroutput.h`
- [ ] `src/Altirra/h/uidbgregisters.h`
- [ ] `src/Altirra/h/uidbgsource.h`
- [ ] `src/Altirra/h/uidbgtargets.h`
- [ ] `src/Altirra/h/uidbgwatch.h`
- [ ] `src/Altirra/h/uidevices.h`
- [ ] `src/Altirra/h/uidiskexplorer_win32.h`
- [ ] `src/Altirra/h/uidisplay.h`
- [ ] `src/Altirra/h/uidisplayaccessibility.h`
- [ ] `src/Altirra/h/uidisplayaccessibility_win32.h`
- [ ] `src/Altirra/h/uidisplaytool.h`
- [ ] `src/Altirra/h/uidragdrop.h`
- [ ] `src/Altirra/h/uienhancedtext.h`
- [ ] `src/Altirra/h/uifilebrowser.h`
- [ ] `src/Altirra/h/uifilefilters.h`
- [ ] `src/Altirra/h/uifirmwaremenu.h`
- [ ] `src/Altirra/h/uifrontend.h`
- [ ] `src/Altirra/h/uifullscreenmode.h`
- [ ] `src/Altirra/h/uihistoryview.h`
- [ ] `src/Altirra/h/uiinstance.h`
- [ ] `src/Altirra/h/uikeyboard.h`
- [ ] `src/Altirra/h/uimenu.h`
- [ ] `src/Altirra/h/uimessagebox.h`
- [ ] `src/Altirra/h/uimrulist.h`
- [ ] `src/Altirra/h/uionscreenkeyboard.h`
- [ ] `src/Altirra/h/uipageddialog.h`
- [ ] `src/Altirra/h/uiportmenus.h`
- [ ] `src/Altirra/h/uiprofiler.h`
- [ ] `src/Altirra/h/uiprogress.h`
- [ ] `src/Altirra/h/uiqueue.h`
- [ ] `src/Altirra/h/uirender.h`
- [ ] `src/Altirra/h/uisavestate.h`
- [ ] `src/Altirra/h/uisettingswindow.h`
- [ ] `src/Altirra/h/uitapeeditor.h`
- [ ] `src/Altirra/h/uitypes.h`
- [ ] `src/Altirra/h/uivideodisplaywindow.h`
- [ ] `src/Altirra/h/ultimate1mb.h`
- [ ] `src/Altirra/h/updatecheck.h`
- [ ] `src/Altirra/h/updatefeed.h`
- [ ] `src/Altirra/h/vbxe.h`
- [ ] `src/Altirra/h/vbxestate.h`
- [ ] `src/Altirra/h/verifier.h`
- [ ] `src/Altirra/h/veronica.h`
- [ ] `src/Altirra/h/versioninfo.h`
- [ ] `src/Altirra/h/vgmplayer.h`
- [ ] `src/Altirra/h/vgmwriter.h`
- [ ] `src/Altirra/h/videomanager.h`
- [ ] `src/Altirra/h/videostillimage.h`
- [ ] `src/Altirra/h/videowriter.h`
- [ ] `src/Altirra/h/virtualscreen.h`
- [ ] `src/Altirra/h/warpos.h`
- [ ] `src/Altirra/h/xelcf.h`
- [ ] `src/Altirra/h/xep80.h`

## src/Altirra/res
- [ ] `src/Altirra/res/Altirra.rc`
- [ ] `src/Altirra/res/AltirraExtIcons.rc`
- [ ] `src/Altirra/res/AltirraVersion.rc`
- [ ] `src/Altirra/res/resource.h`

## src/Altirra/source
- [ ] `src/Altirra/source/1025full.cpp`
- [ ] `src/Altirra/source/1029full.cpp`
- [ ] `src/Altirra/source/1030.cpp`
- [ ] `src/Altirra/source/1030full.cpp`
- [ ] `src/Altirra/source/1090.cpp`
- [ ] `src/Altirra/source/1400xl.cpp`
- [ ] `src/Altirra/source/1450xld.cpp`
- [ ] `src/Altirra/source/820full.cpp`
- [ ] `src/Altirra/source/850full.cpp`
- [ ] `src/Altirra/source/about.cpp`
- [ ] `src/Altirra/source/allocator.cpp`
- [ ] `src/Altirra/source/antic.cpp`
- [ ] `src/Altirra/source/artifacting.cpp`
- [ ] `src/Altirra/source/artifacting_filters.cpp`
- [ ] `src/Altirra/source/artifacting_neon.cpp`
- [ ] `src/Altirra/source/artifacting_ntsc_neon.cpp`
- [ ] `src/Altirra/source/artifacting_ntsc_sse2.cpp`
- [ ] `src/Altirra/source/artifacting_ntsc_sse2_intrin.cpp`
- [ ] `src/Altirra/source/artifacting_pal_neon.cpp`
- [ ] `src/Altirra/source/artifacting_pal_scalar.cpp`
- [ ] `src/Altirra/source/artifacting_pal_sse2.cpp`
- [ ] `src/Altirra/source/artifacting_pal_sse2_intrin.cpp`
- [ ] `src/Altirra/source/artifacting_sse2_intrin.cpp`
- [ ] `src/Altirra/source/assembler.cpp`
- [ ] `src/Altirra/source/asyncdownloader_win32.cpp`
- [ ] `src/Altirra/source/audiomonitor.cpp`
- [ ] `src/Altirra/source/audiorawsource.cpp`
- [ ] `src/Altirra/source/audiosampleplayer.cpp`
- [ ] `src/Altirra/source/audiostocksamples.cpp`
- [ ] `src/Altirra/source/audiowriter.cpp`
- [ ] `src/Altirra/source/autosavemanager.cpp`
- [ ] `src/Altirra/source/aviwriter.cpp`
- [ ] `src/Altirra/source/bit3.cpp`
- [ ] `src/Altirra/source/bkptmanager.cpp`
- [ ] `src/Altirra/source/blackbox.cpp`
- [ ] `src/Altirra/source/blackboxfloppy.cpp`
- [ ] `src/Altirra/source/blockdevdiskadapter.cpp`
- [ ] `src/Altirra/source/blockdevvirtfat32.cpp`
- [ ] `src/Altirra/source/browser.cpp`
- [ ] `src/Altirra/source/cartdetect.cpp`
- [ ] `src/Altirra/source/cartridge.cpp`
- [ ] `src/Altirra/source/cartridgeport.cpp`
- [ ] `src/Altirra/source/cassette.cpp`
- [ ] `src/Altirra/source/cheatengine.cpp`
- [ ] `src/Altirra/source/cmdaudio.cpp`
- [ ] `src/Altirra/source/cmdcart.cpp`
- [ ] `src/Altirra/source/cmdcassette.cpp`
- [ ] `src/Altirra/source/cmdcpu.cpp`
- [ ] `src/Altirra/source/cmddebug.cpp`
- [ ] `src/Altirra/source/cmdinput.cpp`
- [ ] `src/Altirra/source/cmdoptions.cpp`
- [ ] `src/Altirra/source/cmds.cpp`
- [ ] `src/Altirra/source/cmdsystem.cpp`
- [ ] `src/Altirra/source/cmdtools.cpp`
- [ ] `src/Altirra/source/cmdview.cpp`
- [ ] `src/Altirra/source/cmdwindow.cpp`
- [ ] `src/Altirra/source/common_png.cpp`
- [ ] `src/Altirra/source/compatdb.cpp`
- [ ] `src/Altirra/source/compatedb.cpp`
- [ ] `src/Altirra/source/compatengine.cpp`
- [ ] `src/Altirra/source/console.cpp`
- [ ] `src/Altirra/source/constants.cpp`
- [ ] `src/Altirra/source/covox.cpp`
- [ ] `src/Altirra/source/cpu.cpp`
- [ ] `src/Altirra/source/cpu6502.cpp`
- [ ] `src/Altirra/source/cpu6502ill.cpp`
- [ ] `src/Altirra/source/cpu65c02.cpp`
- [ ] `src/Altirra/source/cpu65c816.cpp`
- [ ] `src/Altirra/source/cpuheatmap.cpp`
- [ ] `src/Altirra/source/cpuhookmanager.cpp`
- [ ] `src/Altirra/source/cputracer.cpp`
- [ ] `src/Altirra/source/cs8900a.cpp`
- [ ] `src/Altirra/source/customdevice.cpp`
- [ ] `src/Altirra/source/customdevice_win32.cpp`
- [ ] `src/Altirra/source/customdevicevmtypes.cpp`
- [ ] `src/Altirra/source/debugdisplay.cpp`
- [ ] `src/Altirra/source/debugger.cpp`
- [ ] `src/Altirra/source/debuggerautotest.cpp`
- [ ] `src/Altirra/source/debuggerexp.cpp`
- [ ] `src/Altirra/source/debuggerlog.cpp`
- [ ] `src/Altirra/source/debuggersettings.cpp`
- [ ] `src/Altirra/source/debugtarget.cpp`
- [ ] `src/Altirra/source/decmath.cpp`
- [ ] `src/Altirra/source/decode_png.cpp`
- [ ] `src/Altirra/source/devicemanager.cpp`
- [ ] `src/Altirra/source/devices.cpp`
- [ ] `src/Altirra/source/devxcmdcopypaste.cpp`
- [ ] `src/Altirra/source/devxcmdexploredisk.cpp`
- [ ] `src/Altirra/source/devxcmdmountvhd.cpp`
- [ ] `src/Altirra/source/devxcmdmountvhd_win32.cpp`
- [ ] `src/Altirra/source/devxcmdrescandynamicdisk.cpp`
- [ ] `src/Altirra/source/directorywatcher.cpp`
- [ ] `src/Altirra/source/disasm.cpp`
- [ ] `src/Altirra/source/disk.cpp`
- [ ] `src/Altirra/source/diskdrive815.cpp`
- [ ] `src/Altirra/source/diskdriveamdc.cpp`
- [ ] `src/Altirra/source/diskdriveatr8000.cpp`
- [ ] `src/Altirra/source/diskdrivefull.cpp`
- [ ] `src/Altirra/source/diskdrivefullbase.cpp`
- [ ] `src/Altirra/source/diskdriveindusgt.cpp`
- [ ] `src/Altirra/source/diskdrivepercom.cpp`
- [ ] `src/Altirra/source/diskdrivespeedyxf.cpp`
- [ ] `src/Altirra/source/diskdrivexf551.cpp`
- [ ] `src/Altirra/source/diskinterface.cpp`
- [ ] `src/Altirra/source/diskprofile.cpp`
- [ ] `src/Altirra/source/disktrace.cpp`
- [ ] `src/Altirra/source/diskvirtimage.cpp`
- [ ] `src/Altirra/source/diskvirtimagebase.cpp`
- [ ] `src/Altirra/source/diskvirtimagesdfs.cpp`
- [ ] `src/Altirra/source/dragoncart.cpp`
- [ ] `src/Altirra/source/encode_png.cpp`
- [ ] `src/Altirra/source/errordecode.cpp`
- [ ] `src/Altirra/source/fdc.cpp`
- [ ] `src/Altirra/source/firmwaredetect.cpp`
- [ ] `src/Altirra/source/firmwaremanager.cpp`
- [ ] `src/Altirra/source/gtia.cpp`
- [ ] `src/Altirra/source/gtiarenderer.cpp`
- [ ] `src/Altirra/source/gtiatables.cpp`
- [ ] `src/Altirra/source/hlebasicloader.cpp`
- [ ] `src/Altirra/source/hleciohook.cpp`
- [ ] `src/Altirra/source/hlefastboothook.cpp`
- [ ] `src/Altirra/source/hlefpaccelerator.cpp`
- [ ] `src/Altirra/source/hleprogramloader.cpp`
- [ ] `src/Altirra/source/hleutils.cpp`
- [ ] `src/Altirra/source/hostdevice.cpp`
- [ ] `src/Altirra/source/hostdeviceutils.cpp`
- [ ] `src/Altirra/source/ide.cpp`
- [ ] `src/Altirra/source/idediskdevices.cpp`
- [ ] `src/Altirra/source/idephysdisk.cpp`
- [ ] `src/Altirra/source/iderawimage.cpp`
- [ ] `src/Altirra/source/idevhdimage.cpp`
- [ ] `src/Altirra/source/inputcontroller.cpp`
- [ ] `src/Altirra/source/inputdefs.cpp`
- [ ] `src/Altirra/source/inputmanager.cpp`
- [ ] `src/Altirra/source/inputmap.cpp`
- [ ] `src/Altirra/source/irqcontroller.cpp`
- [ ] `src/Altirra/source/joystick.cpp`
- [ ] `src/Altirra/source/kmkjzide.cpp`
- [ ] `src/Altirra/source/leakdetector.cpp`
- [ ] `src/Altirra/source/main.cpp`
- [ ] `src/Altirra/source/memorymanager.cpp`
- [ ] `src/Altirra/source/midimate.cpp`
- [ ] `src/Altirra/source/mio.cpp`
- [ ] `src/Altirra/source/misccvars.cpp`
- [ ] `src/Altirra/source/mmu.cpp`
- [ ] `src/Altirra/source/modem.cpp`
- [ ] `src/Altirra/source/modemtcp.cpp`
- [ ] `src/Altirra/source/mpp1000e.cpp`
- [ ] `src/Altirra/source/multiplexer.cpp`
- [ ] `src/Altirra/source/myide.cpp`
- [ ] `src/Altirra/source/options.cpp`
- [ ] `src/Altirra/source/oshelper.cpp`
- [ ] `src/Altirra/source/ostracing.cpp`
- [ ] `src/Altirra/source/palettegenerator.cpp`
- [ ] `src/Altirra/source/palettesolver.cpp`
- [ ] `src/Altirra/source/pbi.cpp`
- [ ] `src/Altirra/source/pbidisk.cpp`
- [ ] `src/Altirra/source/pclink.cpp`
- [ ] `src/Altirra/source/pclink_win32.cpp`
- [ ] `src/Altirra/source/pia.cpp`
- [ ] `src/Altirra/source/pipeserial_win32.cpp`
- [ ] `src/Altirra/source/pocketmodem.cpp`
- [ ] `src/Altirra/source/pokeysavecompat.cpp`
- [ ] `src/Altirra/source/pokeytrace.cpp`
- [ ] `src/Altirra/source/portmanager.cpp`
- [ ] `src/Altirra/source/printer.cpp`
- [ ] `src/Altirra/source/printer1020.cpp`
- [ ] `src/Altirra/source/printer1020font.cpp`
- [ ] `src/Altirra/source/printerbase.cpp`
- [ ] `src/Altirra/source/printerfont.cpp`
- [ ] `src/Altirra/source/printeroutput.cpp`
- [ ] `src/Altirra/source/printerttfencoder.cpp`
- [ ] `src/Altirra/source/profiler.cpp`
- [ ] `src/Altirra/source/profilerui.cpp`
- [ ] `src/Altirra/source/rapidus.cpp`
- [ ] `src/Altirra/source/rs232.cpp`
- [ ] `src/Altirra/source/rtime8.cpp`
- [ ] `src/Altirra/source/rverter.cpp`
- [ ] `src/Altirra/source/sapconverter.cpp`
- [ ] `src/Altirra/source/sapwriter.cpp`
- [ ] `src/Altirra/source/savestate.cpp`
- [ ] `src/Altirra/source/savestateio.cpp`
- [ ] `src/Altirra/source/savestatetypes.cpp`
- [ ] `src/Altirra/source/scsidisk.cpp`
- [ ] `src/Altirra/source/sdrive.cpp`
- [ ] `src/Altirra/source/settings.cpp`
- [ ] `src/Altirra/source/side.cpp`
- [ ] `src/Altirra/source/side3.cpp`
- [ ] `src/Altirra/source/simeventmanager.cpp`
- [ ] `src/Altirra/source/simulator.cpp`
- [ ] `src/Altirra/source/sio2sd.cpp`
- [ ] `src/Altirra/source/siomanager.cpp`
- [ ] `src/Altirra/source/slightsid.cpp`
- [ ] `src/Altirra/source/soundboard.cpp`
- [ ] `src/Altirra/source/startuplogger.cpp`
- [ ] `src/Altirra/source/stdafx.cpp`
- [ ] `src/Altirra/source/sx212.cpp`
- [ ] `src/Altirra/source/textdom.cpp`
- [ ] `src/Altirra/source/texteditor.cpp`
- [ ] `src/Altirra/source/thepill.cpp`
- [ ] `src/Altirra/source/trace.cpp`
- [ ] `src/Altirra/source/tracecpu.cpp`
- [ ] `src/Altirra/source/tracefileencoding.cpp`
- [ ] `src/Altirra/source/tracefileformat.cpp`
- [ ] `src/Altirra/source/traceimporta800.cpp`
- [ ] `src/Altirra/source/traceio.cpp`
- [ ] `src/Altirra/source/tracenative.cpp`
- [ ] `src/Altirra/source/tracetape.cpp`
- [ ] `src/Altirra/source/tracevideo.cpp`
- [ ] `src/Altirra/source/ui.cpp`
- [ ] `src/Altirra/source/uiabout.cpp`
- [ ] `src/Altirra/source/uiaccessors.cpp`
- [ ] `src/Altirra/source/uiadvancedconfiguration.cpp`
- [ ] `src/Altirra/source/uiaudiooptions.cpp`
- [ ] `src/Altirra/source/uicalibrationscreen.cpp`
- [ ] `src/Altirra/source/uicaptionupdater.cpp`
- [ ] `src/Altirra/source/uicartmapper.cpp`
- [ ] `src/Altirra/source/uicheater.cpp`
- [ ] `src/Altirra/source/uiclipboard.cpp`
- [ ] `src/Altirra/source/uicolors.cpp`
- [ ] `src/Altirra/source/uicommondialogs.cpp`
- [ ] `src/Altirra/source/uicompat.cpp`
- [ ] `src/Altirra/source/uicompatdb.cpp`
- [ ] `src/Altirra/source/uiconf850.cpp`
- [ ] `src/Altirra/source/uiconfblackbox.cpp`
- [ ] `src/Altirra/source/uiconfdev1020.cpp`
- [ ] `src/Altirra/source/uiconfdev1030.cpp`
- [ ] `src/Altirra/source/uiconfdev1400xl.cpp`
- [ ] `src/Altirra/source/uiconfdev815.cpp`
- [ ] `src/Altirra/source/uiconfdevamdc.cpp`
- [ ] `src/Altirra/source/uiconfdevatr8000.cpp`
- [ ] `src/Altirra/source/uiconfdevblackboxfloppy.cpp`
- [ ] `src/Altirra/source/uiconfdevcomputereyes.cpp`
- [ ] `src/Altirra/source/uiconfdevcorvus.cpp`
- [ ] `src/Altirra/source/uiconfdevcovox.cpp`
- [ ] `src/Altirra/source/uiconfdevcustom.cpp`
- [ ] `src/Altirra/source/uiconfdevdongle.cpp`
- [ ] `src/Altirra/source/uiconfdevhappy810.cpp`
- [ ] `src/Altirra/source/uiconfdevhdvirtfat32.cpp`
- [ ] `src/Altirra/source/uiconfdevkmkjzide.cpp`
- [ ] `src/Altirra/source/uiconfdevkmkjzide2.cpp`
- [ ] `src/Altirra/source/uiconfdevmultiplexer.cpp`
- [ ] `src/Altirra/source/uiconfdevnetserial.cpp`
- [ ] `src/Altirra/source/uiconfdevparfilewriter.cpp`
- [ ] `src/Altirra/source/uiconfdevpercom.cpp`
- [ ] `src/Altirra/source/uiconfdevpipeserial.cpp`
- [ ] `src/Altirra/source/uiconfdevpocketmodem.cpp`
- [ ] `src/Altirra/source/uiconfdevprinter.cpp`
- [ ] `src/Altirra/source/uiconfdevside3.cpp`
- [ ] `src/Altirra/source/uiconfdevsx212.cpp`
- [ ] `src/Altirra/source/uiconfdevvbxe.cpp`
- [ ] `src/Altirra/source/uiconfdevvideostillimage.cpp`
- [ ] `src/Altirra/source/uiconfdevxep80.cpp`
- [ ] `src/Altirra/source/uiconfdiskdrivefull.cpp`
- [ ] `src/Altirra/source/uiconfgeneric.cpp`
- [ ] `src/Altirra/source/uiconfharddisk.cpp`
- [ ] `src/Altirra/source/uiconfiguresystem.cpp`
- [ ] `src/Altirra/source/uiconfirm.cpp`
- [ ] `src/Altirra/source/uiconfmodem.cpp`
- [ ] `src/Altirra/source/uiconfmyide2.cpp`
- [ ] `src/Altirra/source/uiconfregister.cpp`
- [ ] `src/Altirra/source/uiconfsoundboard.cpp`
- [ ] `src/Altirra/source/uiconfveronica.cpp`
- [ ] `src/Altirra/source/uidbgbreakpoints.cpp`
- [ ] `src/Altirra/source/uidbgcallstack.cpp`
- [ ] `src/Altirra/source/uidbgconsole.cpp`
- [ ] `src/Altirra/source/uidbgdebugdisplay.cpp`
- [ ] `src/Altirra/source/uidbgdisasm.cpp`
- [ ] `src/Altirra/source/uidbghelp.cpp`
- [ ] `src/Altirra/source/uidbghistory.cpp`
- [ ] `src/Altirra/source/uidbgmemory.cpp`
- [ ] `src/Altirra/source/uidbgpane.cpp`
- [ ] `src/Altirra/source/uidbgprinteroutput.cpp`
- [ ] `src/Altirra/source/uidbgregisters.cpp`
- [ ] `src/Altirra/source/uidbgsource.cpp`
- [ ] `src/Altirra/source/uidbgtargets.cpp`
- [ ] `src/Altirra/source/uidbgwatch.cpp`
- [ ] `src/Altirra/source/uidebugfont.cpp`
- [ ] `src/Altirra/source/uidevices.cpp`
- [ ] `src/Altirra/source/uidisk.cpp`
- [ ] `src/Altirra/source/uidiskexplorer.cpp`
- [ ] `src/Altirra/source/uidiskexplorer_win32.cpp`
- [ ] `src/Altirra/source/uidisplay.cpp`
- [ ] `src/Altirra/source/uidisplayaccessibility.cpp`
- [ ] `src/Altirra/source/uidisplayaccessibility_win32.cpp`
- [ ] `src/Altirra/source/uidisplaytool.cpp`
- [ ] `src/Altirra/source/uidragdrop.cpp`
- [ ] `src/Altirra/source/uidragoncart.cpp`
- [ ] `src/Altirra/source/uiemuerror.cpp`
- [ ] `src/Altirra/source/uienhancedtext.cpp`
- [ ] `src/Altirra/source/uiexportroms.cpp`
- [ ] `src/Altirra/source/uifileassoc.cpp`
- [ ] `src/Altirra/source/uifilebrowser.cpp`
- [ ] `src/Altirra/source/uifilefilters.cpp`
- [ ] `src/Altirra/source/uifirmware.cpp`
- [ ] `src/Altirra/source/uifirmwaremenu.cpp`
- [ ] `src/Altirra/source/uifirmwarescan.cpp`
- [ ] `src/Altirra/source/uifrontend.cpp`
- [ ] `src/Altirra/source/uifullscreenmode.cpp`
- [ ] `src/Altirra/source/uihistoryview.cpp`
- [ ] `src/Altirra/source/uihostdevice.cpp`
- [ ] `src/Altirra/source/uiinput.cpp`
- [ ] `src/Altirra/source/uiinputrebind.cpp`
- [ ] `src/Altirra/source/uiinputsetup.cpp`
- [ ] `src/Altirra/source/uiinstance.cpp`
- [ ] `src/Altirra/source/uikeyboard.cpp`
- [ ] `src/Altirra/source/uikeyboardcustomize.cpp`
- [ ] `src/Altirra/source/uilightpen.cpp`
- [ ] `src/Altirra/source/uimainwindow.cpp`
- [ ] `src/Altirra/source/uimenu.cpp`
- [ ] `src/Altirra/source/uimessagebox.cpp`
- [ ] `src/Altirra/source/uimrulist.cpp`
- [ ] `src/Altirra/source/uionscreenkeyboard.cpp`
- [ ] `src/Altirra/source/uipageddialog.cpp`
- [ ] `src/Altirra/source/uipclink.cpp`
- [ ] `src/Altirra/source/uiphysdisk.cpp`
- [ ] `src/Altirra/source/uiportmenus.cpp`
- [ ] `src/Altirra/source/uiprofilemenu.cpp`
- [ ] `src/Altirra/source/uiprofiler.cpp`
- [ ] `src/Altirra/source/uiprofiles.cpp`
- [ ] `src/Altirra/source/uiprogress.cpp`
- [ ] `src/Altirra/source/uiqueue.cpp`
- [ ] `src/Altirra/source/uiregistry.cpp`
- [ ] `src/Altirra/source/uirender.cpp`
- [ ] `src/Altirra/source/uisavestate.cpp`
- [ ] `src/Altirra/source/uiscreeneffects.cpp`
- [ ] `src/Altirra/source/uisettingsmain.cpp`
- [ ] `src/Altirra/source/uisettingswindow.cpp`
- [ ] `src/Altirra/source/uisetupwizard.cpp`
- [ ] `src/Altirra/source/uitapecontrol.cpp`
- [ ] `src/Altirra/source/uitapeeditor.cpp`
- [ ] `src/Altirra/source/uitraceviewer.cpp`
- [ ] `src/Altirra/source/uiupdatecheck.cpp`
- [ ] `src/Altirra/source/uiverifier.cpp`
- [ ] `src/Altirra/source/uivideodisplaywindow.cpp`
- [ ] `src/Altirra/source/uivideooutputmenu.cpp`
- [ ] `src/Altirra/source/uivideorecording.cpp`
- [ ] `src/Altirra/source/ultimate1mb.cpp`
- [ ] `src/Altirra/source/updatecheck.cpp`
- [ ] `src/Altirra/source/updatefeed.cpp`
- [ ] `src/Altirra/source/updatefeed_win32.cpp`
- [ ] `src/Altirra/source/vbxe.cpp`
- [ ] `src/Altirra/source/vbxestate.cpp`
- [ ] `src/Altirra/source/verifier.cpp`
- [ ] `src/Altirra/source/veronica.cpp`
- [ ] `src/Altirra/source/vgmplayer.cpp`
- [ ] `src/Altirra/source/vgmwriter.cpp`
- [ ] `src/Altirra/source/videomanager.cpp`
- [ ] `src/Altirra/source/videostillimage.cpp`
- [ ] `src/Altirra/source/videowriter.cpp`
- [ ] `src/Altirra/source/virtualscreen.cpp`
- [ ] `src/Altirra/source/warpos.cpp`
- [ ] `src/Altirra/source/xelcf.cpp`
- [ ] `src/Altirra/source/xep80.cpp`

## src/AltirraRMTC6502/h
- [ ] `src/AltirraRMTC6502/h/stdafx.h`

## src/AltirraRMTC6502/source
- [ ] `src/AltirraRMTC6502/source/main.cpp`
- [ ] `src/AltirraRMTC6502/source/rmtinterface.cpp`
- [ ] `src/AltirraRMTC6502/source/stdafx.cpp`

## src/AltirraRMTPOKEY/h
- [ ] `src/AltirraRMTPOKEY/h/stdafx.h`

## src/AltirraRMTPOKEY/interface
- [ ] `src/AltirraRMTPOKEY/interface/rmtbypass.h`

## src/AltirraRMTPOKEY/source
- [ ] `src/AltirraRMTPOKEY/source/main.cpp`
- [ ] `src/AltirraRMTPOKEY/source/rmtinterface.cpp`
- [ ] `src/AltirraRMTPOKEY/source/stdafx.cpp`

## src/AltirraShell/autogen
- [ ] `src/AltirraShell/autogen/menu.inl`

## src/Asuka/h
- [ ] `src/Asuka/h/filecreator.h`
- [ ] `src/Asuka/h/stdafx.h`
- [ ] `src/Asuka/h/symbols.h`
- [ ] `src/Asuka/h/utils.h`

## src/Asuka/res
- [ ] `src/Asuka/res/Asuka.rc`
- [ ] `src/Asuka/res/resource.h`

## src/Asuka/source
- [ ] `src/Asuka/source/checkimports.cpp`
- [ ] `src/Asuka/source/filecreator.cpp`
- [ ] `src/Asuka/source/fontencode.cpp`
- [ ] `src/Asuka/source/fontextract.cpp`
- [ ] `src/Asuka/source/fxc10.cpp`
- [ ] `src/Asuka/source/hash.cpp`
- [ ] `src/Asuka/source/main.cpp`
- [ ] `src/Asuka/source/makearray.cpp`
- [ ] `src/Asuka/source/maketables.cpp`
- [ ] `src/Asuka/source/resbind.cpp`
- [ ] `src/Asuka/source/resextract.cpp`
- [ ] `src/Asuka/source/signexport.cpp`
- [ ] `src/Asuka/source/signxml.cpp`
- [ ] `src/Asuka/source/stdafx.cpp`
- [ ] `src/Asuka/source/utils.cpp`

## src/Dita/h
- [ ] `src/Dita/h/stdafx.h`

## src/Dita/source
- [ ] `src/Dita/source/accel.cpp`
- [ ] `src/Dita/source/services.cpp`
- [ ] `src/Dita/source/stdafx.cpp`
- [ ] `src/Dita/source/w32accel.cpp`

## src/Kasumi/h
- [ ] `src/Kasumi/h/bitutils.h`
- [ ] `src/Kasumi/h/blt_setup.h`
- [ ] `src/Kasumi/h/blt_spanutils.h`
- [ ] `src/Kasumi/h/blt_spanutils_arm64.h`
- [ ] `src/Kasumi/h/blt_spanutils_x86.h`
- [ ] `src/Kasumi/h/defaultfont.inl`
- [ ] `src/Kasumi/h/resample_stages.h`
- [ ] `src/Kasumi/h/resample_stages_arm64.h`
- [ ] `src/Kasumi/h/resample_stages_reference.h`
- [ ] `src/Kasumi/h/resample_stages_x64.h`
- [ ] `src/Kasumi/h/resample_stages_x86.h`
- [ ] `src/Kasumi/h/stdafx.h`
- [ ] `src/Kasumi/h/uberblit.h`
- [ ] `src/Kasumi/h/uberblit_16f.h`
- [ ] `src/Kasumi/h/uberblit_base.h`
- [ ] `src/Kasumi/h/uberblit_fill.h`
- [ ] `src/Kasumi/h/uberblit_gen.h`
- [ ] `src/Kasumi/h/uberblit_input.h`
- [ ] `src/Kasumi/h/uberblit_interlace.h`
- [ ] `src/Kasumi/h/uberblit_pal.h`
- [ ] `src/Kasumi/h/uberblit_resample.h`
- [ ] `src/Kasumi/h/uberblit_resample_special.h`
- [ ] `src/Kasumi/h/uberblit_resample_special_x86.h`
- [ ] `src/Kasumi/h/uberblit_rgb.h`
- [ ] `src/Kasumi/h/uberblit_rgb_x86.h`
- [ ] `src/Kasumi/h/uberblit_swizzle.h`
- [ ] `src/Kasumi/h/uberblit_swizzle_x86.h`
- [ ] `src/Kasumi/h/uberblit_ycbcr.h`
- [ ] `src/Kasumi/h/uberblit_ycbcr_generic.h`
- [ ] `src/Kasumi/h/uberblit_ycbcr_sse2_intrin.h`
- [ ] `src/Kasumi/h/uberblit_ycbcr_x86.h`

## src/Kasumi/source
- [ ] `src/Kasumi/source/alphablt.cpp`
- [ ] `src/Kasumi/source/blitter.cpp`
- [ ] `src/Kasumi/source/blt.cpp`
- [ ] `src/Kasumi/source/blt_reference.cpp`
- [ ] `src/Kasumi/source/blt_reference_pal.cpp`
- [ ] `src/Kasumi/source/blt_reference_rgb.cpp`
- [ ] `src/Kasumi/source/blt_reference_yuv.cpp`
- [ ] `src/Kasumi/source/blt_reference_yuv2yuv.cpp`
- [ ] `src/Kasumi/source/blt_reference_yuvrev.cpp`
- [ ] `src/Kasumi/source/blt_setup.cpp`
- [ ] `src/Kasumi/source/blt_spanutils.cpp`
- [ ] `src/Kasumi/source/blt_spanutils_arm64.cpp`
- [ ] `src/Kasumi/source/blt_spanutils_x86.cpp`
- [ ] `src/Kasumi/source/blt_uberblit.cpp`
- [ ] `src/Kasumi/source/blt_x86.cpp`
- [ ] `src/Kasumi/source/pixel.cpp`
- [ ] `src/Kasumi/source/pixmaputils.cpp`
- [ ] `src/Kasumi/source/region.cpp`
- [ ] `src/Kasumi/source/region_neon.cpp`
- [ ] `src/Kasumi/source/region_sse2.cpp`
- [ ] `src/Kasumi/source/resample.cpp`
- [ ] `src/Kasumi/source/resample_kernels.cpp`
- [ ] `src/Kasumi/source/resample_stages.cpp`
- [ ] `src/Kasumi/source/resample_stages_arm64.cpp`
- [ ] `src/Kasumi/source/resample_stages_reference.cpp`
- [ ] `src/Kasumi/source/resample_stages_x64.cpp`
- [ ] `src/Kasumi/source/resample_stages_x86.cpp`
- [ ] `src/Kasumi/source/stdafx.cpp`
- [ ] `src/Kasumi/source/stretchblt_reference.cpp`
- [ ] `src/Kasumi/source/uberblit.cpp`
- [ ] `src/Kasumi/source/uberblit_16f.cpp`
- [ ] `src/Kasumi/source/uberblit_gen.cpp`
- [ ] `src/Kasumi/source/uberblit_resample.cpp`
- [ ] `src/Kasumi/source/uberblit_resample_special.cpp`
- [ ] `src/Kasumi/source/uberblit_resample_special_x86.cpp`
- [ ] `src/Kasumi/source/uberblit_swizzle.cpp`
- [ ] `src/Kasumi/source/uberblit_swizzle_x86.cpp`
- [ ] `src/Kasumi/source/uberblit_ycbcr_generic.cpp`
- [ ] `src/Kasumi/source/uberblit_ycbcr_sse2_intrin.cpp`
- [ ] `src/Kasumi/source/uberblit_ycbcr_x86.cpp`

## src/Misc/audiofilter
- [ ] `src/Misc/audiofilter/filter63to44.cpp`

## src/Misc/driveconv
- [ ] `src/Misc/driveconv/driveconv.cpp`

## src/Misc/font80
- [ ] `src/Misc/font80/font80.cpp`

## src/Riza/h
- [ ] `src/Riza/h/stdafx.h`

## src/Riza/source
- [ ] `src/Riza/source/bitmap.cpp`
- [ ] `src/Riza/source/stdafx.cpp`

## src/Tessa/h/D3D11
- [ ] `src/Tessa/h/D3D11/Context_D3D11.h`
- [ ] `src/Tessa/h/D3D11/FenceManager_D3D11.h`

## src/Tessa/h
- [ ] `src/Tessa/h/Program.h`
- [ ] `src/Tessa/h/stdafx.h`

## src/Tessa/source
- [ ] `src/Tessa/source/Config.cpp`
- [ ] `src/Tessa/source/Context.cpp`

## src/Tessa/source/D3D11
- [ ] `src/Tessa/source/D3D11/Context_D3D11.cpp`
- [ ] `src/Tessa/source/D3D11/FenceManager_D3D11.cpp`

## src/Tessa/source
- [ ] `src/Tessa/source/Format.cpp`
- [ ] `src/Tessa/source/Options.cpp`
- [ ] `src/Tessa/source/Program.cpp`
- [ ] `src/Tessa/source/stdafx.cpp`

## src/VDDisplay/autogen
- [ ] `src/VDDisplay/autogen/displayd3d9_shader.inl`
- [ ] `src/VDDisplay/autogen/image_shader.inl`

## src/VDDisplay/h
- [ ] `src/VDDisplay/h/bicubic.h`
- [ ] `src/VDDisplay/h/displaydrv3d.h`
- [ ] `src/VDDisplay/h/displaydrvd3d9.h`
- [ ] `src/VDDisplay/h/displaymgr.h`
- [ ] `src/VDDisplay/h/displaynode3d.h`
- [ ] `src/VDDisplay/h/renderer3d.h`
- [ ] `src/VDDisplay/h/stdafx.h`

## src/VDDisplay/h/vd2/VDDisplay/internal
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/bloom.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffect.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffectbase.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffectd3d11.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffectd3d9.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffectimpld3d11.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffectimpld3d9.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffectpassbase.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffectutils.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/customeffectutilsd3d.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/fontbitmap.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/fontgdi.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/options.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/renderergdi.h`
- [ ] `src/VDDisplay/h/vd2/VDDisplay/internal/screenfx.h`

## src/VDDisplay/source
- [ ] `src/VDDisplay/source/bicubic.cpp`
- [ ] `src/VDDisplay/source/bloom.cpp`
- [ ] `src/VDDisplay/source/customeffectbase.cpp`
- [ ] `src/VDDisplay/source/customeffectimpld3d11.cpp`
- [ ] `src/VDDisplay/source/customeffectimpld3d9.cpp`
- [ ] `src/VDDisplay/source/customeffectpassbase.cpp`
- [ ] `src/VDDisplay/source/customeffectutils.cpp`
- [ ] `src/VDDisplay/source/customeffectutilsd3d.cpp`
- [ ] `src/VDDisplay/source/direct3d.cpp`
- [ ] `src/VDDisplay/source/display.cpp`
- [ ] `src/VDDisplay/source/displaydrv.cpp`
- [ ] `src/VDDisplay/source/displaydrv3d.cpp`
- [ ] `src/VDDisplay/source/displaydrvd3d9.cpp`
- [ ] `src/VDDisplay/source/displaygdi.cpp`
- [ ] `src/VDDisplay/source/displaymgr.cpp`
- [ ] `src/VDDisplay/source/displaynode3d.cpp`
- [ ] `src/VDDisplay/source/displaytypes.cpp`
- [ ] `src/VDDisplay/source/fontbitmap.cpp`
- [ ] `src/VDDisplay/source/fontgdi.cpp`
- [ ] `src/VDDisplay/source/logging.cpp`
- [ ] `src/VDDisplay/source/minid3dx.cpp`
- [ ] `src/VDDisplay/source/options.cpp`
- [ ] `src/VDDisplay/source/rendercache.cpp`
- [ ] `src/VDDisplay/source/renderer.cpp`
- [ ] `src/VDDisplay/source/renderer3d.cpp`
- [ ] `src/VDDisplay/source/rendererd3d9.cpp`
- [ ] `src/VDDisplay/source/renderergdi.cpp`
- [ ] `src/VDDisplay/source/renderersoft.cpp`
- [ ] `src/VDDisplay/source/screenfx.cpp`
- [ ] `src/VDDisplay/source/stdafx.cpp`
- [ ] `src/VDDisplay/source/textrenderer.cpp`

## src/h/at/atappbase
- [ ] `src/h/at/atappbase/crthooks.h`
- [ ] `src/h/at/atappbase/exceptionfilter.h`

## src/h/at/ataudio
- [ ] `src/h/at/ataudio/audioconvolutionplayer.h`
- [ ] `src/h/at/ataudio/audiofilters.h`
- [ ] `src/h/at/ataudio/audioout.h`
- [ ] `src/h/at/ataudio/audiooutput.h`
- [ ] `src/h/at/ataudio/audiosampleplayer.h`
- [ ] `src/h/at/ataudio/audiosamplepool.h`
- [ ] `src/h/at/ataudio/pokey.h`
- [ ] `src/h/at/ataudio/pokeyrenderer.h`
- [ ] `src/h/at/ataudio/pokeysavestate.h`
- [ ] `src/h/at/ataudio/pokeytables.h`

## src/h/at/atcore
- [ ] `src/h/at/atcore/address.h`
- [ ] `src/h/at/atcore/asyncdispatcher.h`
- [ ] `src/h/at/atcore/asyncdispatcherimpl.h`
- [ ] `src/h/at/atcore/atascii.h`
- [ ] `src/h/at/atcore/audiomixer.h`
- [ ] `src/h/at/atcore/audiosource.h`
- [ ] `src/h/at/atcore/blockdevice.h`
- [ ] `src/h/at/atcore/bussignal.h`
- [ ] `src/h/at/atcore/checksum.h`
- [ ] `src/h/at/atcore/cio.h`
- [ ] `src/h/at/atcore/comsupport_win32.h`
- [ ] `src/h/at/atcore/configvar.h`
- [ ] `src/h/at/atcore/consoleoutput.h`
- [ ] `src/h/at/atcore/constants.h`
- [ ] `src/h/at/atcore/crc.h`
- [ ] `src/h/at/atcore/decmath.h`
- [ ] `src/h/at/atcore/deferredevent.h`
- [ ] `src/h/at/atcore/device.h`
- [ ] `src/h/at/atcore/devicecart.h`
- [ ] `src/h/at/atcore/devicecio.h`
- [ ] `src/h/at/atcore/devicediskdrive.h`
- [ ] `src/h/at/atcore/deviceimpl.h`
- [ ] `src/h/at/atcore/deviceindicators.h`
- [ ] `src/h/at/atcore/deviceparent.h`
- [ ] `src/h/at/atcore/deviceparentimpl.h`
- [ ] `src/h/at/atcore/devicepbi.h`
- [ ] `src/h/at/atcore/devicepia.h`
- [ ] `src/h/at/atcore/deviceport.h`
- [ ] `src/h/at/atcore/deviceprinter.h`
- [ ] `src/h/at/atcore/deviceserial.h`
- [ ] `src/h/at/atcore/devicesio.h`
- [ ] `src/h/at/atcore/devicesioimpl.h`
- [ ] `src/h/at/atcore/devicesnapshot.h`
- [ ] `src/h/at/atcore/devicestorage.h`
- [ ] `src/h/at/atcore/devicestorageimpl.h`
- [ ] `src/h/at/atcore/devicesystemcontrol.h`
- [ ] `src/h/at/atcore/deviceu1mb.h`
- [ ] `src/h/at/atcore/devicevideo.h`
- [ ] `src/h/at/atcore/devicevideosource.h`
- [ ] `src/h/at/atcore/enumparse.h`
- [ ] `src/h/at/atcore/enumparseimpl.h`
- [ ] `src/h/at/atcore/enumutils.h`
- [ ] `src/h/at/atcore/fft.h`
- [ ] `src/h/at/atcore/intrin_neon.h`
- [ ] `src/h/at/atcore/intrin_sse2.h`
- [ ] `src/h/at/atcore/ksyms.h`
- [ ] `src/h/at/atcore/logging.h`
- [ ] `src/h/at/atcore/md5.h`
- [ ] `src/h/at/atcore/media.h`
- [ ] `src/h/at/atcore/memoryutils.h`
- [ ] `src/h/at/atcore/notifylist.h`
- [ ] `src/h/at/atcore/profile.h`
- [ ] `src/h/at/atcore/progress.h`
- [ ] `src/h/at/atcore/propertyset.h`
- [ ] `src/h/at/atcore/randomization.h`
- [ ] `src/h/at/atcore/savestate.h`
- [ ] `src/h/at/atcore/scheduler.h`
- [ ] `src/h/at/atcore/serializable.h`
- [ ] `src/h/at/atcore/serialization.h`
- [ ] `src/h/at/atcore/sioutils.h`
- [ ] `src/h/at/atcore/snapshot.h`
- [ ] `src/h/at/atcore/snapshotimpl.h`
- [ ] `src/h/at/atcore/timerservice.h`
- [ ] `src/h/at/atcore/vfs.h`
- [ ] `src/h/at/atcore/wraptime.h`

## src/h/at/atcpu
- [ ] `src/h/at/atcpu/breakpoints.h`
- [ ] `src/h/at/atcpu/co6502.h`
- [ ] `src/h/at/atcpu/co65802.h`
- [ ] `src/h/at/atcpu/co6809.h`
- [ ] `src/h/at/atcpu/co8048.h`
- [ ] `src/h/at/atcpu/co8051.h`
- [ ] `src/h/at/atcpu/coz80.h`
- [ ] `src/h/at/atcpu/decode6502.h`
- [ ] `src/h/at/atcpu/decode65816.h`
- [ ] `src/h/at/atcpu/decode6809.h`
- [ ] `src/h/at/atcpu/decodez80.h`
- [ ] `src/h/at/atcpu/execstate.h`
- [ ] `src/h/at/atcpu/history.h`
- [ ] `src/h/at/atcpu/memorymap.h`
- [ ] `src/h/at/atcpu/states.h`
- [ ] `src/h/at/atcpu/states6502.h`
- [ ] `src/h/at/atcpu/states6809.h`
- [ ] `src/h/at/atcpu/statesz80.h`

## src/h/at/atdebugger
- [ ] `src/h/at/atdebugger/argparse.h`
- [ ] `src/h/at/atdebugger/breakpointsimpl.h`
- [ ] `src/h/at/atdebugger/debugdevice.h`
- [ ] `src/h/at/atdebugger/expression.h`
- [ ] `src/h/at/atdebugger/historytree.h`
- [ ] `src/h/at/atdebugger/historytreebuilder.h`
- [ ] `src/h/at/atdebugger/symbols.h`
- [ ] `src/h/at/atdebugger/target.h`

## src/h/at/atdevices
- [ ] `src/h/at/atdevices/devices.h`
- [ ] `src/h/at/atdevices/modemsound.h`

## src/h/at/atemulation
- [ ] `src/h/at/atemulation/acia.h`
- [ ] `src/h/at/atemulation/acia6850.h`
- [ ] `src/h/at/atemulation/crtc.h`
- [ ] `src/h/at/atemulation/ctc.h`
- [ ] `src/h/at/atemulation/diskutils.h`
- [ ] `src/h/at/atemulation/eeprom.h`
- [ ] `src/h/at/atemulation/flash.h`
- [ ] `src/h/at/atemulation/phone.h`
- [ ] `src/h/at/atemulation/riot.h`
- [ ] `src/h/at/atemulation/rtcds1305.h`
- [ ] `src/h/at/atemulation/rtcmcp7951x.h`
- [ ] `src/h/at/atemulation/rtcv3021.h`
- [ ] `src/h/at/atemulation/scsi.h`
- [ ] `src/h/at/atemulation/via.h`

## src/h/at/atio
- [ ] `src/h/at/atio/atfs.h`
- [ ] `src/h/at/atio/audioreader.h`
- [ ] `src/h/at/atio/blobimage.h`
- [ ] `src/h/at/atio/cartridgeimage.h`
- [ ] `src/h/at/atio/cartridgetypes.h`
- [ ] `src/h/at/atio/cassetteblock.h`
- [ ] `src/h/at/atio/cassettedecoder.h`
- [ ] `src/h/at/atio/cassetteimage.h`
- [ ] `src/h/at/atio/diskfs.h`
- [ ] `src/h/at/atio/diskfsdos2util.h`
- [ ] `src/h/at/atio/diskfssdx2util.h`
- [ ] `src/h/at/atio/diskfsutil.h`
- [ ] `src/h/at/atio/diskimage.h`
- [ ] `src/h/at/atio/image.h`
- [ ] `src/h/at/atio/partitiondiskview.h`
- [ ] `src/h/at/atio/partitiontable.h`
- [ ] `src/h/at/atio/programimage.h`
- [ ] `src/h/at/atio/savestate.h`
- [ ] `src/h/at/atio/wav.h`

## src/h/at/atnativeui
- [ ] `src/h/at/atnativeui/acceleditdialog.h`
- [ ] `src/h/at/atnativeui/accessibility_win32.h`
- [ ] `src/h/at/atnativeui/canvas_win32.h`
- [ ] `src/h/at/atnativeui/controlstyles.h`
- [ ] `src/h/at/atnativeui/debug_win32.h`
- [ ] `src/h/at/atnativeui/dialog.h`
- [ ] `src/h/at/atnativeui/dragdrop.h`
- [ ] `src/h/at/atnativeui/genericdialog.h`
- [ ] `src/h/at/atnativeui/hotkeyexcontrol.h`
- [ ] `src/h/at/atnativeui/hotkeyscandialog.h`
- [ ] `src/h/at/atnativeui/messagedispatcher.h`
- [ ] `src/h/at/atnativeui/messageloop.h`
- [ ] `src/h/at/atnativeui/nativewindowproxy.h`
- [ ] `src/h/at/atnativeui/progress.h`
- [ ] `src/h/at/atnativeui/theme.h`
- [ ] `src/h/at/atnativeui/theme_win32.h`
- [ ] `src/h/at/atnativeui/uiframe.h`
- [ ] `src/h/at/atnativeui/uinativewindow.h`
- [ ] `src/h/at/atnativeui/uipane.h`
- [ ] `src/h/at/atnativeui/uipanedialog.h`
- [ ] `src/h/at/atnativeui/uipanewindow.h`
- [ ] `src/h/at/atnativeui/uiproxies.h`
- [ ] `src/h/at/atnativeui/utils.h`

## src/h/at/atnetwork
- [ ] `src/h/at/atnetwork/emusocket.h`
- [ ] `src/h/at/atnetwork/ethernet.h`
- [ ] `src/h/at/atnetwork/ethernetbus.h`
- [ ] `src/h/at/atnetwork/ethernetframe.h`
- [ ] `src/h/at/atnetwork/gatewayserver.h`
- [ ] `src/h/at/atnetwork/socket.h`
- [ ] `src/h/at/atnetwork/tcp.h`
- [ ] `src/h/at/atnetwork/udp.h`

## src/h/at/atnetworksockets
- [ ] `src/h/at/atnetworksockets/nativesockets.h`
- [ ] `src/h/at/atnetworksockets/socketutils_win32.h`
- [ ] `src/h/at/atnetworksockets/vxlantunnel.h`
- [ ] `src/h/at/atnetworksockets/worker.h`

## src/h/at/attest
- [ ] `src/h/at/attest/test.h`

## src/h/at/atui
- [ ] `src/h/at/atui/accessibility.h`
- [ ] `src/h/at/atui/constants.h`
- [ ] `src/h/at/atui/uianchor.h`
- [ ] `src/h/at/atui/uicommandmanager.h`
- [ ] `src/h/at/atui/uicontainer.h`
- [ ] `src/h/at/atui/uidragdrop.h`
- [ ] `src/h/at/atui/uidrawingutils.h`
- [ ] `src/h/at/atui/uimanager.h`
- [ ] `src/h/at/atui/uimenulist.h`
- [ ] `src/h/at/atui/uiwidget.h`

## src/h/at/atuicontrols
- [ ] `src/h/at/atuicontrols/uibutton.h`
- [ ] `src/h/at/atuicontrols/uiimage.h`
- [ ] `src/h/at/atuicontrols/uilabel.h`
- [ ] `src/h/at/atuicontrols/uilistview.h`
- [ ] `src/h/at/atuicontrols/uislider.h`
- [ ] `src/h/at/atuicontrols/uitextedit.h`

## src/h/at/atvm
- [ ] `src/h/at/atvm/compiler.h`
- [ ] `src/h/at/atvm/vm.h`

## src/h/vd2/Dita
- [ ] `src/h/vd2/Dita/accel.h`
- [ ] `src/h/vd2/Dita/services.h`
- [ ] `src/h/vd2/Dita/w32accel.h`
- [ ] `src/h/vd2/Dita/w32interface.h`

## src/h/vd2/Kasumi
- [ ] `src/h/vd2/Kasumi/blitter.h`
- [ ] `src/h/vd2/Kasumi/pixel.h`
- [ ] `src/h/vd2/Kasumi/pixmap.h`
- [ ] `src/h/vd2/Kasumi/pixmapops.h`
- [ ] `src/h/vd2/Kasumi/pixmaputils.h`
- [ ] `src/h/vd2/Kasumi/region.h`
- [ ] `src/h/vd2/Kasumi/resample.h`
- [ ] `src/h/vd2/Kasumi/resample_kernels.h`

## src/h/vd2/Riza
- [ ] `src/h/vd2/Riza/avi.h`
- [ ] `src/h/vd2/Riza/bitmap.h`

## src/h/vd2/Tessa
- [ ] `src/h/vd2/Tessa/Config.h`
- [ ] `src/h/vd2/Tessa/Context.h`
- [ ] `src/h/vd2/Tessa/Format.h`
- [ ] `src/h/vd2/Tessa/Options.h`
- [ ] `src/h/vd2/Tessa/Types.h`

## src/h/vd2/VDDisplay
- [ ] `src/h/vd2/VDDisplay/compositor.h`
- [ ] `src/h/vd2/VDDisplay/direct3d.h`
- [ ] `src/h/vd2/VDDisplay/display.h`
- [ ] `src/h/vd2/VDDisplay/displaydrv.h`
- [ ] `src/h/vd2/VDDisplay/displaytypes.h`
- [ ] `src/h/vd2/VDDisplay/font.h`
- [ ] `src/h/vd2/VDDisplay/logging.h`
- [ ] `src/h/vd2/VDDisplay/minid3dx.h`
- [ ] `src/h/vd2/VDDisplay/rendercache.h`
- [ ] `src/h/vd2/VDDisplay/renderer.h`
- [ ] `src/h/vd2/VDDisplay/renderergdi.h`
- [ ] `src/h/vd2/VDDisplay/renderersoft.h`
- [ ] `src/h/vd2/VDDisplay/textrenderer.h`

## src/h/vd2/external
- [ ] `src/h/vd2/external/glATI.h`

## src/h/vd2/system
- [ ] `src/h/vd2/system/Error.h`
- [ ] `src/h/vd2/system/Fraction.h`
- [ ] `src/h/vd2/system/VDQueue.h`
- [ ] `src/h/vd2/system/VDRingBuffer.h`
- [ ] `src/h/vd2/system/VDString.h`
- [ ] `src/h/vd2/system/atomic.h`
- [ ] `src/h/vd2/system/binary.h`
- [x] `src/h/vd2/system/bitmath.h` — MSVC intrinsic branches replaced with __builtin_ctz/clz
- [ ] `src/h/vd2/system/cache.h`
- [ ] `src/h/vd2/system/cmdline.h`
- [ ] `src/h/vd2/system/color.h`
- [ ] `src/h/vd2/system/constexpr.h`
- [ ] `src/h/vd2/system/cpuaccel.h`
- [ ] `src/h/vd2/system/date.h`
- [ ] `src/h/vd2/system/debug.h`
- [ ] `src/h/vd2/system/debugx86.h`
- [ ] `src/h/vd2/system/event.h`
- [ ] `src/h/vd2/system/file.h`
- [ ] `src/h/vd2/system/fileasync.h`
- [ ] `src/h/vd2/system/filesys.h`
- [ ] `src/h/vd2/system/filewatcher.h`
- [ ] `src/h/vd2/system/function.h`
- [ ] `src/h/vd2/system/halffloat.h`
- [ ] `src/h/vd2/system/hash.h`
- [x] `src/h/vd2/system/int128.h` — unsigned-long ctor removed; AMD64 uses GCC __int128
- [ ] `src/h/vd2/system/linearalloc.h`
- [ ] `src/h/vd2/system/math.h`
- [ ] `src/h/vd2/system/memory.h`
- [ ] `src/h/vd2/system/process.h`
- [ ] `src/h/vd2/system/refcount.h`
- [ ] `src/h/vd2/system/registry.h`
- [ ] `src/h/vd2/system/registrymemory.h`
- [ ] `src/h/vd2/system/seh.h`
- [x] `src/h/vd2/system/strutil.h` — MSVC _stricmp/_wcsicmp replaced with POSIX strcasecmp + inline wide variants
- [ ] `src/h/vd2/system/text.h`
- [x] `src/h/vd2/system/thread.h` — rewrote around std::thread / std::mutex / std::shared_mutex / std::condition_variable
- [ ] `src/h/vd2/system/thunk.h`
- [ ] `src/h/vd2/system/time.h`
- [ ] `src/h/vd2/system/tls.h`
- [ ] `src/h/vd2/system/unknown.h`
- [ ] `src/h/vd2/system/vdalloc.h`
- [x] `src/h/vd2/system/vdstdc.h` — dropped MSVC/MinGW branches; plain vsnprintf/vswprintf/swprintf
- [ ] `src/h/vd2/system/vdstl.h`
- [ ] `src/h/vd2/system/vdstl_algorithm.h`
- [ ] `src/h/vd2/system/vdstl_block.h`
- [ ] `src/h/vd2/system/vdstl_fastdeque.h`
- [ ] `src/h/vd2/system/vdstl_fastvector.h`
- [ ] `src/h/vd2/system/vdstl_hash.h`
- [ ] `src/h/vd2/system/vdstl_hashmap.h`
- [ ] `src/h/vd2/system/vdstl_hashset.h`
- [ ] `src/h/vd2/system/vdstl_hashtable.h`
- [ ] `src/h/vd2/system/vdstl_ilist.h`
- [ ] `src/h/vd2/system/vdstl_span.h`
- [ ] `src/h/vd2/system/vdstl_structex.h`
- [ ] `src/h/vd2/system/vdstl_vector.h`
- [ ] `src/h/vd2/system/vdstl_vectorview.h`
- [ ] `src/h/vd2/system/vdtypes.h`
- [ ] `src/h/vd2/system/vecmath.h`
- [ ] `src/h/vd2/system/vecmath_neon.h`
- [ ] `src/h/vd2/system/vecmath_ref.h`
- [ ] `src/h/vd2/system/vecmath_sse2.h`
- [ ] `src/h/vd2/system/vectors.h`
- [ ] `src/h/vd2/system/vectors_float.h`
- [ ] `src/h/vd2/system/vectors_int.h`
- [ ] `src/h/vd2/system/w32assist.h`

## src/h/vd2/system/win32
- [ ] `src/h/vd2/system/win32/intrin.h`
- [ ] `src/h/vd2/system/win32/miniwindows.h`
- [ ] `src/h/vd2/system/win32/touch.h`

## src/h/vd2/system
- [ ] `src/h/vd2/system/zip.h`

## src/h/vd2/vdjson
- [ ] `src/h/vd2/vdjson/jsonnametable.h`
- [ ] `src/h/vd2/vdjson/jsonoutput.h`
- [ ] `src/h/vd2/vdjson/jsonreader.h`
- [ ] `src/h/vd2/vdjson/jsonvalue.h`
- [ ] `src/h/vd2/vdjson/jsonwriter.h`

## src/system/h
- [ ] `src/system/h/stdafx.h`

## src/system/source
- [x] `src/system/source/Error.cpp` — HWND in post() signature swapped for VDExceptionPostContext
- [x] `src/system/source/Fraction.cpp`
- [x] `src/system/source/VDString.cpp`
- [x] `src/system/source/binary.cpp`
- [x] `src/system/source/bitmath.cpp`
- [ ] `src/system/source/cache.cpp`
- [ ] `src/system/source/cmdline.cpp`
- [x] `src/system/source/constexpr.cpp`
- [x] `src/system/source/cpuaccel.cpp` — Win32 intrinsics + wtypes.h replaced with POSIX cpuid.h
- [x] `src/system/source/date.cpp` — rewrote around clock_gettime/localtime_r/strftime
- [x] `src/system/source/debug.cpp` — asserts + VDDebugPrint routed to stderr
- [ ] `src/system/source/debugx86.cpp`
- [ ] ~~`src/system/source/error_win32.cpp`~~ — Win32-only, skip (Error.cpp covers needs)
- [x] `src/system/source/event.cpp`
- [x] `src/system/source/file.cpp` — rewrote VDFile around POSIX file descriptors (open/read/write/lseek)
- [ ] `src/system/source/fileasync.cpp`
- [ ] `src/system/source/filestream.cpp`
- [x] `src/system/source/filesys.cpp` — path helpers kept; Win32 file ops replaced with stat/opendir/realpath/mmap
- [ ] `src/system/source/filewatcher.cpp`
- [x] `src/system/source/function.cpp`
- [x] `src/system/source/halffloat.cpp`
- [x] `src/system/source/hash.cpp`
- [x] `src/system/source/int128.cpp` — MSVC inline asm blocks removed; AMD64 uses GCC __int128
- [x] `src/system/source/linearalloc.cpp`
- [x] `src/system/source/math.cpp`
- [x] `src/system/source/memory.cpp` — VirtualAlloc → mmap, _aligned_malloc → posix_memalign
- [x] `src/system/source/process.cpp` — rewrote VDLaunchProgram with posix_spawnp
- [x] `src/system/source/refcount.cpp`
- [x] `src/system/source/registry.cpp` — default provider now VDRegistryProviderMemory (POSIX has no native registry)
- [x] `src/system/source/registrymemory.cpp`
- [x] `src/system/source/stdaccel.cpp`
- [x] `src/system/source/stdafx.cpp`
- [x] `src/system/source/strutil.cpp`
- [x] `src/system/source/text.cpp` — narrow↔wide routed through UTF-8 (POSIX uses UTF-8 everywhere)
- [x] `src/system/source/thread.cpp` — rewrote around std::thread / std::mutex / etc.
- [ ] ~~`src/system/source/thunk.cpp`~~ — Win32-only callback thunking, skip (Qt uses signals/slots)
- [x] `src/system/source/time.cpp` — VDCallbackTimer on std::thread + std::chrono; VDLazyTimer on std::thread
- [x] `src/system/source/tls.cpp`
- [x] `src/system/source/vdalloc.cpp`
- [x] `src/system/source/vdstl.cpp`
- [x] `src/system/source/vdstl_hash.cpp`
- [x] `src/system/source/vdstl_hashtable.cpp`
- [x] `src/system/source/vectors.cpp`
- [ ] ~~`src/system/source/w32assist.cpp`~~ — Win32-only, skip
- [x] `src/system/source/zip.cpp` — MSVC intrinsic includes swapped; rest is portable (compiles & links)

## src/system/source/win32
- [ ] ~~`src/system/source/win32/touch_win32.cpp`~~ — Win32-only, skip

## src/system/source
- [ ] `src/system/source/zip.cpp`

## src/vdjson/h
- [ ] `src/vdjson/h/stdafx.h`

## src/vdjson/source
- [ ] `src/vdjson/source/jsonnametable.cpp`
- [ ] `src/vdjson/source/jsonoutput.cpp`
- [ ] `src/vdjson/source/jsonreader.cpp`
- [ ] `src/vdjson/source/jsonvalue.cpp`
- [ ] `src/vdjson/source/jsonwriter.cpp`
- [ ] `src/vdjson/source/stdafx.cpp`
