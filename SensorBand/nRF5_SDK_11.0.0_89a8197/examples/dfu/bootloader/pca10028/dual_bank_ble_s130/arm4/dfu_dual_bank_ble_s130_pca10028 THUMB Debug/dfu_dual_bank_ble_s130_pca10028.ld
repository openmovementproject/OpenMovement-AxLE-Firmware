MEMORY
{
  UNPLACED_SECTIONS (wx) : ORIGIN = 0x100000000, LENGTH = 0
  AHB (wx) : ORIGIN = 0x50000000, LENGTH = 0x00001000
  APB (wx) : ORIGIN = 0x40000000, LENGTH = 0x00020000
  RAM (wx) : ORIGIN = 0x20000000, LENGTH = 0x00004000
  UICR (wx) : ORIGIN = 0x10001000, LENGTH = 0x00000100
  FICR (wx) : ORIGIN = 0x10000000, LENGTH = 0x00000100
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x00040000
}


SECTIONS
{
  __AHB_segment_start__ = 0x50000000;
  __AHB_segment_end__ = 0x50001000;
  __AHB_segment_size__ = 0x00001000;
  __APB_segment_start__ = 0x40000000;
  __APB_segment_end__ = 0x40020000;
  __APB_segment_size__ = 0x00020000;
  __RAM_segment_start__ = 0x20000000;
  __RAM_segment_end__ = 0x20004000;
  __RAM_segment_size__ = 0x00004000;
  __UICR_segment_start__ = 0x10001000;
  __UICR_segment_end__ = 0x10001100;
  __UICR_segment_size__ = 0x00000100;
  __FICR_segment_start__ = 0x10000000;
  __FICR_segment_end__ = 0x10000100;
  __FICR_segment_size__ = 0x00000100;
  __FLASH_segment_start__ = 0x00000000;
  __FLASH_segment_end__ = 0x00040000;
  __FLASH_segment_size__ = 0x00040000;

  __HEAPSIZE__ = 128;
  __STACKSIZE_ABT__ = 0;
  __STACKSIZE_FIQ__ = 0;
  __STACKSIZE_IRQ__ = 0;
  __STACKSIZE_PROCESS__ = 0;
  __STACKSIZE_SVC__ = 0;
  __STACKSIZE_UND__ = 0;
  __STACKSIZE__ = 2048;

  __uicr_start_load_start__ = 0x10001000;
  .uicr_start 0x10001000 : AT(0x10001000)
  {
    __uicr_start_start__ = .;
    *(.uicr_start .uicr_start.*)
    . = ALIGN(MAX(__uicr_start_start__ + 0x14 , .), 4);
  }
  __uicr_start_end__ = __uicr_start_start__ + SIZEOF(.uicr_start);
  __uicr_start_size__ = SIZEOF(.uicr_start);
  __uicr_start_load_end__ = __uicr_start_end__;

  . = ASSERT(__uicr_start_start__ == __uicr_start_end__ || (__uicr_start_end__ >= __UICR_segment_start__ && __uicr_start_end__ <= __UICR_segment_end__) , "error: .uicr_start is too large to fit in UICR memory segment");

  __uicrBootStartAddress_load_start__ = 0x10001014;
  .uicrBootStartAddress 0x10001014 : AT(0x10001014)
  {
    __uicrBootStartAddress_start__ = .;
    *(.uicrBootStartAddress .uicrBootStartAddress.*)
    . = ALIGN(MAX(__uicrBootStartAddress_start__ + 0x04 , .), 4);
  }
  __uicrBootStartAddress_end__ = __uicrBootStartAddress_start__ + SIZEOF(.uicrBootStartAddress);
  __uicrBootStartAddress_size__ = SIZEOF(.uicrBootStartAddress);
  __uicrBootStartAddress_load_end__ = __uicrBootStartAddress_end__;

  __UICR_segment_used_end__ = 0x10001014 + SIZEOF(.uicrBootStartAddress);
  __UICR_segment_used_size__ = __UICR_segment_used_end__ - __UICR_segment_start__;

  . = ASSERT(__uicrBootStartAddress_start__ == __uicrBootStartAddress_end__ || (__uicrBootStartAddress_end__ >= __UICR_segment_start__ && __uicrBootStartAddress_end__ <= __UICR_segment_end__) , "error: .uicrBootStartAddress is too large to fit in UICR memory segment");

  . = ASSERT(__uicr_start_end__ <= __uicrBootStartAddress_start__ , "error: section .uicr_start overlaps absolute placed section .uicrBootStartAddress");

  __S130_RAM_load_start__ = 0x20000000;
  .S130_RAM 0x20000000 (NOLOAD) : AT(0x20000000)
  {
    __S130_RAM_start__ = .;
    *(.S130_RAM .S130_RAM.*)
    . = ALIGN(MAX(__S130_RAM_start__ + 0x2C00 , .), 256);
  }
  __S130_RAM_end__ = __S130_RAM_start__ + SIZEOF(.S130_RAM);
  __S130_RAM_size__ = SIZEOF(.S130_RAM);
  __S130_RAM_load_end__ = __S130_RAM_end__;

  . = ASSERT(__S130_RAM_start__ == __S130_RAM_end__ || (__S130_RAM_end__ >= __RAM_segment_start__ && __S130_RAM_end__ <= __RAM_segment_end__) , "error: .S130_RAM is too large to fit in RAM memory segment");

  __vectors_ram_load_start__ = 0x20002C00;
  .vectors_ram 0x20002C00 (NOLOAD) : AT(0x20002C00)
  {
    __vectors_ram_start__ = .;
    *(.vectors_ram .vectors_ram.*)
  }
  __vectors_ram_end__ = __vectors_ram_start__ + SIZEOF(.vectors_ram);
  __vectors_ram_size__ = SIZEOF(.vectors_ram);
  __vectors_ram_load_end__ = __vectors_ram_end__;

  . = ASSERT(__vectors_ram_start__ == __vectors_ram_end__ || (__vectors_ram_end__ >= __RAM_segment_start__ && __vectors_ram_end__ <= __RAM_segment_end__) , "error: .vectors_ram is too large to fit in RAM memory segment");

  . = ASSERT(__S130_RAM_end__ <= __vectors_ram_start__ , "error: section .S130_RAM overlaps absolute placed section .vectors_ram");

  __S130_SoftDevice_load_start__ = 0x0000;
  .S130_SoftDevice 0x0000 : AT(0x0000)
  {
    __S130_SoftDevice_start__ = .;
    *(.S130_SoftDevice .S130_SoftDevice.*)
    . = ALIGN(MAX(__S130_SoftDevice_start__ + 0x1B000 , .), 256);
  }
  __S130_SoftDevice_end__ = __S130_SoftDevice_start__ + SIZEOF(.S130_SoftDevice);
  __S130_SoftDevice_size__ = SIZEOF(.S130_SoftDevice);
  __S130_SoftDevice_load_end__ = __S130_SoftDevice_end__;

  . = ASSERT(__S130_SoftDevice_start__ == __S130_SoftDevice_end__ || (__S130_SoftDevice_end__ >= __FLASH_segment_start__ && __S130_SoftDevice_end__ <= __FLASH_segment_end__) , "error: .S130_SoftDevice is too large to fit in FLASH memory segment");

  __reserved_for_app_load_start__ = 0x1B000;
  .reserved_for_app 0x1B000 : AT(0x1B000)
  {
    __reserved_for_app_start__ = .;
    *(.reserved_for_app .reserved_for_app.*)
    . = ALIGN(MAX(__reserved_for_app_start__ + 0x1D000 , .), 256);
  }
  __reserved_for_app_end__ = __reserved_for_app_start__ + SIZEOF(.reserved_for_app);
  __reserved_for_app_size__ = SIZEOF(.reserved_for_app);
  __reserved_for_app_load_end__ = __reserved_for_app_end__;

  . = ASSERT(__reserved_for_app_start__ == __reserved_for_app_end__ || (__reserved_for_app_end__ >= __FLASH_segment_start__ && __reserved_for_app_end__ <= __FLASH_segment_end__) , "error: .reserved_for_app is too large to fit in FLASH memory segment");

  . = ASSERT(__S130_SoftDevice_end__ <= __reserved_for_app_start__ , "error: section .S130_SoftDevice overlaps absolute placed section .reserved_for_app");

  __vectors_load_start__ = 0x38000;
  .vectors 0x38000 : AT(0x38000)
  {
    __vectors_start__ = .;
    *(.vectors .vectors.*)
  }
  __vectors_end__ = __vectors_start__ + SIZEOF(.vectors);
  __vectors_size__ = SIZEOF(.vectors);
  __vectors_load_end__ = __vectors_end__;

  . = ASSERT(__vectors_start__ == __vectors_end__ || (__vectors_end__ >= __FLASH_segment_start__ && __vectors_end__ <= __FLASH_segment_end__) , "error: .vectors is too large to fit in FLASH memory segment");

  . = ASSERT(__reserved_for_app_end__ <= __vectors_start__ , "error: section .reserved_for_app overlaps absolute placed section .vectors");

  __init_load_start__ = ALIGN(__vectors_end__ , 4);
  .init ALIGN(__vectors_end__ , 4) : AT(ALIGN(__vectors_end__ , 4))
  {
    __init_start__ = .;
    *(.init .init.*)
  }
  __init_end__ = __init_start__ + SIZEOF(.init);
  __init_size__ = SIZEOF(.init);
  __init_load_end__ = __init_end__;

  . = ASSERT(__init_start__ == __init_end__ || (__init_end__ >= __FLASH_segment_start__ && __init_end__ <= __FLASH_segment_end__) , "error: .init is too large to fit in FLASH memory segment");

  __init_rodata_load_start__ = ALIGN(__init_end__ , 4);
  .init_rodata ALIGN(__init_end__ , 4) : AT(ALIGN(__init_end__ , 4))
  {
    __init_rodata_start__ = .;
    *(.init_rodata .init_rodata.*)
  }
  __init_rodata_end__ = __init_rodata_start__ + SIZEOF(.init_rodata);
  __init_rodata_size__ = SIZEOF(.init_rodata);
  __init_rodata_load_end__ = __init_rodata_end__;

  . = ASSERT(__init_rodata_start__ == __init_rodata_end__ || (__init_rodata_end__ >= __FLASH_segment_start__ && __init_rodata_end__ <= __FLASH_segment_end__) , "error: .init_rodata is too large to fit in FLASH memory segment");

  __text_load_start__ = ALIGN(__init_rodata_end__ , 4);
  .text ALIGN(__init_rodata_end__ , 4) : AT(ALIGN(__init_rodata_end__ , 4))
  {
    __text_start__ = .;
    *(.text .text.* .glue_7t .glue_7 .gnu.linkonce.t.* .gcc_except_table .ARM.extab* .gnu.linkonce.armextab.*)
  }
  __text_end__ = __text_start__ + SIZEOF(.text);
  __text_size__ = SIZEOF(.text);
  __text_load_end__ = __text_end__;

  . = ASSERT(__text_start__ == __text_end__ || (__text_end__ >= __FLASH_segment_start__ && __text_end__ <= __FLASH_segment_end__) , "error: .text is too large to fit in FLASH memory segment");

  __dtors_load_start__ = ALIGN(__text_end__ , 4);
  .dtors ALIGN(__text_end__ , 4) : AT(ALIGN(__text_end__ , 4))
  {
    __dtors_start__ = .;
    KEEP (*(SORT(.dtors.*))) KEEP (*(.dtors)) KEEP (*(.fini_array)) KEEP (*(SORT(.fini_array.*)))
  }
  __dtors_end__ = __dtors_start__ + SIZEOF(.dtors);
  __dtors_size__ = SIZEOF(.dtors);
  __dtors_load_end__ = __dtors_end__;

  . = ASSERT(__dtors_start__ == __dtors_end__ || (__dtors_end__ >= __FLASH_segment_start__ && __dtors_end__ <= __FLASH_segment_end__) , "error: .dtors is too large to fit in FLASH memory segment");

  __ctors_load_start__ = ALIGN(__dtors_end__ , 4);
  .ctors ALIGN(__dtors_end__ , 4) : AT(ALIGN(__dtors_end__ , 4))
  {
    __ctors_start__ = .;
    KEEP (*(SORT(.ctors.*))) KEEP (*(.ctors)) KEEP (*(.init_array)) KEEP (*(SORT(.init_array.*)))
  }
  __ctors_end__ = __ctors_start__ + SIZEOF(.ctors);
  __ctors_size__ = SIZEOF(.ctors);
  __ctors_load_end__ = __ctors_end__;

  . = ASSERT(__ctors_start__ == __ctors_end__ || (__ctors_end__ >= __FLASH_segment_start__ && __ctors_end__ <= __FLASH_segment_end__) , "error: .ctors is too large to fit in FLASH memory segment");

  __rodata_load_start__ = ALIGN(__ctors_end__ , 4);
  .rodata ALIGN(__ctors_end__ , 4) : AT(ALIGN(__ctors_end__ , 4))
  {
    __rodata_start__ = .;
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  }
  __rodata_end__ = __rodata_start__ + SIZEOF(.rodata);
  __rodata_size__ = SIZEOF(.rodata);
  __rodata_load_end__ = __rodata_end__;

  . = ASSERT(__rodata_start__ == __rodata_end__ || (__rodata_end__ >= __FLASH_segment_start__ && __rodata_end__ <= __FLASH_segment_end__) , "error: .rodata is too large to fit in FLASH memory segment");

  __ARM.exidx_load_start__ = ALIGN(__rodata_end__ , 4);
  .ARM.exidx ALIGN(__rodata_end__ , 4) : AT(ALIGN(__rodata_end__ , 4))
  {
    __ARM.exidx_start__ = .;
    __exidx_start =   __ARM.exidx_start__;
    *(.ARM.exidx .ARM.exidx.*)
  }
  __ARM.exidx_end__ = __ARM.exidx_start__ + SIZEOF(.ARM.exidx);
  __ARM.exidx_size__ = SIZEOF(.ARM.exidx);
  __exidx_end =   __ARM.exidx_end__;
  __ARM.exidx_load_end__ = __ARM.exidx_end__;

  . = ASSERT(__ARM.exidx_start__ == __ARM.exidx_end__ || (__ARM.exidx_end__ >= __FLASH_segment_start__ && __ARM.exidx_end__ <= __FLASH_segment_end__) , "error: .ARM.exidx is too large to fit in FLASH memory segment");

  __fast_load_start__ = ALIGN(__ARM.exidx_end__ , 4);
  .fast ALIGN(__vectors_ram_end__ , 4) : AT(ALIGN(__ARM.exidx_end__ , 4))
  {
    __fast_start__ = .;
    *(.fast .fast.*)
  }
  __fast_end__ = __fast_start__ + SIZEOF(.fast);
  __fast_size__ = SIZEOF(.fast);
  __fast_load_end__ = __fast_load_start__ + SIZEOF(.fast);

  . = ASSERT(__fast_load_start__ == __fast_load_end__ || (__fast_load_end__ >= __FLASH_segment_start__ && __fast_load_end__ <= __FLASH_segment_end__) , "error: .fast is too large to fit in FLASH memory segment");

  .fast_run ALIGN(__vectors_ram_end__ , 4) (NOLOAD) :
  {
    __fast_run_start__ = .;
    . = MAX(__fast_run_start__ + SIZEOF(.fast), .);
  }
  __fast_run_end__ = __fast_run_start__ + SIZEOF(.fast_run);
  __fast_run_size__ = SIZEOF(.fast_run);
  __fast_run_load_end__ = __fast_run_end__;

  . = ASSERT(__fast_run_start__ == __fast_run_end__ || (__fast_run_end__ >= __RAM_segment_start__ && __fast_run_end__ <= __RAM_segment_end__) , "error: .fast_run is too large to fit in RAM memory segment");

  __data_load_start__ = ALIGN(__fast_load_start__ + SIZEOF(.fast) , 4);
  .data ALIGN(__fast_run_end__ , 4) : AT(ALIGN(__fast_load_start__ + SIZEOF(.fast) , 4))
  {
    __data_start__ = .;
    *(.data .data.* .gnu.linkonce.d.*)
  }
  __data_end__ = __data_start__ + SIZEOF(.data);
  __data_size__ = SIZEOF(.data);
  __data_load_end__ = __data_load_start__ + SIZEOF(.data);

  . = ASSERT(__data_load_start__ == __data_load_end__ || (__data_load_end__ >= __FLASH_segment_start__ && __data_load_end__ <= __FLASH_segment_end__) , "error: .data is too large to fit in FLASH memory segment");

  .data_run ALIGN(__fast_run_end__ , 4) (NOLOAD) :
  {
    __data_run_start__ = .;
    . = MAX(__data_run_start__ + SIZEOF(.data), .);
  }
  __data_run_end__ = __data_run_start__ + SIZEOF(.data_run);
  __data_run_size__ = SIZEOF(.data_run);
  __data_run_load_end__ = __data_run_end__;

  . = ASSERT(__data_run_start__ == __data_run_end__ || (__data_run_end__ >= __RAM_segment_start__ && __data_run_end__ <= __RAM_segment_end__) , "error: .data_run is too large to fit in RAM memory segment");

  __tdata_load_start__ = ALIGN(__data_load_start__ + SIZEOF(.data) , 4);
  .tdata ALIGN(__data_run_end__ , 4) : AT(ALIGN(__data_load_start__ + SIZEOF(.data) , 4))
  {
    __tdata_start__ = .;
    *(.tdata .tdata.*)
  }
  __tdata_end__ = __tdata_start__ + SIZEOF(.tdata);
  __tdata_size__ = SIZEOF(.tdata);
  __tdata_load_end__ = __tdata_load_start__ + SIZEOF(.tdata);

  . = ASSERT(__tdata_load_start__ == __tdata_load_end__ || (__tdata_load_end__ >= __FLASH_segment_start__ && __tdata_load_end__ <= __FLASH_segment_end__) , "error: .tdata is too large to fit in FLASH memory segment");

  .tdata_run ALIGN(__data_run_end__ , 4) (NOLOAD) :
  {
    __tdata_run_start__ = .;
    . = MAX(__tdata_run_start__ + SIZEOF(.tdata), .);
  }
  __tdata_run_end__ = __tdata_run_start__ + SIZEOF(.tdata_run);
  __tdata_run_size__ = SIZEOF(.tdata_run);
  __tdata_run_load_end__ = __tdata_run_end__;

  . = ASSERT(__tdata_run_start__ == __tdata_run_end__ || (__tdata_run_end__ >= __RAM_segment_start__ && __tdata_run_end__ <= __RAM_segment_end__) , "error: .tdata_run is too large to fit in RAM memory segment");

  __bss_load_start__ = ALIGN(__tdata_run_end__ , 4);
  .bss ALIGN(__tdata_run_end__ , 4) (NOLOAD) : AT(ALIGN(__tdata_run_end__ , 4))
  {
    __bss_start__ = .;
    *(.bss .bss.* .gnu.linkonce.b.*) *(COMMON)
  }
  __bss_end__ = __bss_start__ + SIZEOF(.bss);
  __bss_size__ = SIZEOF(.bss);
  __bss_load_end__ = __bss_end__;

  . = ASSERT(__bss_start__ == __bss_end__ || (__bss_end__ >= __RAM_segment_start__ && __bss_end__ <= __RAM_segment_end__) , "error: .bss is too large to fit in RAM memory segment");

  __tbss_load_start__ = ALIGN(__bss_end__ , 4);
  .tbss ALIGN(__bss_end__ , 4) (NOLOAD) : AT(ALIGN(__bss_end__ , 4))
  {
    __tbss_start__ = .;
    *(.tbss .tbss.*)
  }
  __tbss_end__ = __tbss_start__ + SIZEOF(.tbss);
  __tbss_size__ = SIZEOF(.tbss);
  __tbss_load_end__ = __tbss_end__;

  . = ASSERT(__tbss_start__ == __tbss_end__ || (__tbss_end__ >= __RAM_segment_start__ && __tbss_end__ <= __RAM_segment_end__) , "error: .tbss is too large to fit in RAM memory segment");

  __non_init_load_start__ = ALIGN(__tbss_end__ , 4);
  .non_init ALIGN(__tbss_end__ , 4) (NOLOAD) : AT(ALIGN(__tbss_end__ , 4))
  {
    __non_init_start__ = .;
    *(.non_init .non_init.*)
  }
  __non_init_end__ = __non_init_start__ + SIZEOF(.non_init);
  __non_init_size__ = SIZEOF(.non_init);
  __non_init_load_end__ = __non_init_end__;

  . = ASSERT(__non_init_start__ == __non_init_end__ || (__non_init_end__ >= __RAM_segment_start__ && __non_init_end__ <= __RAM_segment_end__) , "error: .non_init is too large to fit in RAM memory segment");

  __heap_load_start__ = ALIGN(__non_init_end__ , 4);
  .heap ALIGN(__non_init_end__ , 4) (NOLOAD) : AT(ALIGN(__non_init_end__ , 4))
  {
    __heap_start__ = .;
    *(.heap .heap.*)
    . = ALIGN(MAX(__heap_start__ + __HEAPSIZE__ , .), 4);
  }
  __heap_end__ = __heap_start__ + SIZEOF(.heap);
  __heap_size__ = SIZEOF(.heap);
  __heap_load_end__ = __heap_end__;

  . = ASSERT(__heap_start__ == __heap_end__ || (__heap_end__ >= __RAM_segment_start__ && __heap_end__ <= __RAM_segment_end__) , "error: .heap is too large to fit in RAM memory segment");

  __stack_load_start__ = ALIGN(__heap_end__ , 8);
  .stack ALIGN(__heap_end__ , 8) (NOLOAD) : AT(ALIGN(__heap_end__ , 8))
  {
    __stack_start__ = .;
    *(.stack .stack.*)
    . = ALIGN(MAX(__stack_start__ + __STACKSIZE__ , .), 8);
  }
  __stack_end__ = __stack_start__ + SIZEOF(.stack);
  __stack_size__ = SIZEOF(.stack);
  __stack_load_end__ = __stack_end__;

  . = ASSERT(__stack_start__ == __stack_end__ || (__stack_end__ >= __RAM_segment_start__ && __stack_end__ <= __RAM_segment_end__) , "error: .stack is too large to fit in RAM memory segment");

  __stack_process_load_start__ = ALIGN(__stack_end__ , 8);
  .stack_process ALIGN(__stack_end__ , 8) (NOLOAD) : AT(ALIGN(__stack_end__ , 8))
  {
    __stack_process_start__ = .;
    *(.stack_process .stack_process.*)
    . = ALIGN(MAX(__stack_process_start__ + __STACKSIZE_PROCESS__ , .), 8);
  }
  __stack_process_end__ = __stack_process_start__ + SIZEOF(.stack_process);
  __stack_process_size__ = SIZEOF(.stack_process);
  __stack_process_load_end__ = __stack_process_end__;

  . = ASSERT(__stack_process_start__ == __stack_process_end__ || (__stack_process_end__ >= __RAM_segment_start__ && __stack_process_end__ <= __RAM_segment_end__) , "error: .stack_process is too large to fit in RAM memory segment");

  __rtc_state_load_start__ = 0x20003F60;
  .rtc_state 0x20003F60 (NOLOAD) : AT(0x20003F60)
  {
    __rtc_state_start__ = .;
    *(.rtc_state .rtc_state.*)
    . = ALIGN(MAX(__rtc_state_start__ + 0x20 , .), 4);
  }
  __rtc_state_end__ = __rtc_state_start__ + SIZEOF(.rtc_state);
  __rtc_state_size__ = SIZEOF(.rtc_state);
  __rtc_state_load_end__ = __rtc_state_end__;

  . = ASSERT(__rtc_state_start__ == __rtc_state_end__ || (__rtc_state_end__ >= __RAM_segment_start__ && __rtc_state_end__ <= __RAM_segment_end__) , "error: .rtc_state is too large to fit in RAM memory segment");

  . = ASSERT(__stack_process_end__ <= __rtc_state_start__ , "error: section .stack_process overlaps absolute placed section .rtc_state");

  __noinit_load_start__ = 0x20003F80;
  .noinit 0x20003F80 (NOLOAD) : AT(0x20003F80)
  {
    __noinit_start__ = .;
    *(.noinit .noinit.*)
    . = ALIGN(MAX(__noinit_start__ + 0x80 , .), 4);
  }
  __noinit_end__ = __noinit_start__ + SIZEOF(.noinit);
  __noinit_size__ = SIZEOF(.noinit);
  __noinit_load_end__ = __noinit_end__;

  __RAM_segment_used_end__ = 0x20003F80 + SIZEOF(.noinit);
  __RAM_segment_used_size__ = __RAM_segment_used_end__ - __RAM_segment_start__;

  . = ASSERT(__noinit_start__ == __noinit_end__ || (__noinit_end__ >= __RAM_segment_start__ && __noinit_end__ <= __RAM_segment_end__) , "error: .noinit is too large to fit in RAM memory segment");

  . = ASSERT(__rtc_state_end__ <= __noinit_start__ , "error: section .rtc_state overlaps absolute placed section .noinit");

  __bootloaderSettings_load_start__ = 0x0003FC00;
  .bootloaderSettings 0x0003FC00 : AT(0x0003FC00)
  {
    __bootloaderSettings_start__ = .;
    *(.bootloaderSettings .bootloaderSettings.*)
    . = ALIGN(MAX(__bootloaderSettings_start__ + 0x0400 , .), 4);
  }
  __bootloaderSettings_end__ = __bootloaderSettings_start__ + SIZEOF(.bootloaderSettings);
  __bootloaderSettings_size__ = SIZEOF(.bootloaderSettings);
  __bootloaderSettings_load_end__ = __bootloaderSettings_end__;

  __FLASH_segment_used_end__ = 0x0003FC00 + SIZEOF(.bootloaderSettings);
  __FLASH_segment_used_size__ = __FLASH_segment_used_end__ - __FLASH_segment_start__;

  . = ASSERT(__bootloaderSettings_start__ == __bootloaderSettings_end__ || (__bootloaderSettings_end__ >= __FLASH_segment_start__ && __bootloaderSettings_end__ <= __FLASH_segment_end__) , "error: .bootloaderSettings is too large to fit in FLASH memory segment");

  . = ASSERT(__tdata_load_end__ <= __bootloaderSettings_start__ , "error: section .tdata overlaps absolute placed section .bootloaderSettings");

}

