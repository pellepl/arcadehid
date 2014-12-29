/* Entry Point */
ENTRY(Reset_Handler)

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 128K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 20K
}

FLASH_START = ORIGIN(FLASH);
FLASH_SIZE  = LENGTH(FLASH);
RAM_START   = ORIGIN(RAM);
RAM_SIZE    = LENGTH(RAM);

/* Shared memory size */
_Shared_Memory_Size = 128;

/* Generate a link error if stack don't fit into RAM */
_Min_Stack_Size = 0x2000; /* required amount of stack */

/* Highest address of the user mode stack */
_estack = RAM_START + RAM_SIZE - _Shared_Memory_Size;

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    /*KEEP(*(.isr_vector)) *//* Startup code *//*moved down to text segment for sake of disassembly */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    ___SVECTOR = .;
    KEEP(*(.isr_vector)) /* Startup code */
    ASSERT (. != ___SVECTOR, "No isr_vector input-section!");
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text.*)         /* .text* sections (code) */
    *(.gnu.linkonce.t.*)
    /**(.plt)*/
    /**(.gnu.warning)*/
    /**(.vfp11_veneer)*/
    /**(.eh_frame)*/

    *(EXCLUDE_FILE (*bootloader*) .rodata)         /* .rodata sections (constants, strings, etc.) */
    *(EXCLUDE_FILE (*bootloader*) .rodata.*)       /* .rodata* sections (constants, strings, etc.) */
    *(.gnu.linkonce.r.*)

    *(.ARM.extab* .gnu.linkonce.armextab.*)
    /**(.gcc_except_table)*/
    /**(.eh_frame_hdr)*/
    /**(.eh_frame)*/
    
    . = ALIGN(4);
    KEEP (*(.init))

    . = ALIGN(4);
    __preinit_array_start = .;
    KEEP (*(.preinit_array))
    __preinit_array_end = .;

    . = ALIGN(4);
    __init_array_start = .;
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    __init_array_end = .;

    . = ALIGN(4);
    KEEP (*(.fini))

    . = ALIGN(4);
    __fini_array_start = .;
    KEEP (*(.fini_array))
    KEEP (*(SORT(.fini_array.*)))
    __fini_array_end = .;
  } >FLASH

  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } >FLASH
  
  .text.align_etext :
  {
    . = ALIGN(8);
    _etext = .;
  } >FLASH

  /* 
  Define bootloader text and data segments placement.
  This will overlap .data segment, must not coexist with app.
  */

  _bootloader_text_laddr = .;
  _bootloader_text_vaddr = RAM_START;
  _bootloader_text_lma_start = _bootloader_text_laddr;
  
  .bootloader_text _bootloader_text_vaddr : AT ( _bootloader_text_laddr )
  {
    _bootloader_text_vma_start = .;
    *(.bootloader_text)
    *(.bootloader_text.*)
    _bootloader_text_vma_end = .;
    . = ALIGN(8);
  }
  
  _bootloader_data_laddr = _bootloader_text_laddr + SIZEOF (.bootloader_text);
  _bootloader_data_vaddr =  _bootloader_text_vaddr + SIZEOF (.bootloader_text); 
  _bootloader_data_lma_start = _bootloader_data_laddr;
  
  .bootloader_data _bootloader_data_vaddr : AT ( _bootloader_data_laddr )
  {
    _bootloader_data_vma_start = .;
    *bootloader*(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *bootloader*(.rodata.*)       /* .rodata sections (constants, strings, etc.) */
    *(.bootloader_data)
    *(.bootloader_data.*)
    _bootloader_data_vma_end = .;
    . = ALIGN(4);
  }
  
  /* Reset pointer */ 
  
  . = _bootloader_data_laddr + SIZEOF (.bootloader_data);

  /* used by the startup to initialize data */
  _sidata = .;

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : AT ( _sidata )
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.got.plt) *(.got)
    *(.shdata)
    *(.data)           /* .data sections */
    *(.data.*)          /* .data* sections */
    *(.gnu.linkonce.d.*)

    . = ALIGN(4);
    *(.ram_functions)

    . = ALIGN(8);
    _edata = .;        /* define a global symbol at data end */
  } >RAM

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)

    . = ALIGN(8);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

    . = ALIGN(4);
  __stack_start__ =  .;
    . = ALIGN(4);
  __stack_end__ = _estack;

  /* Shared memory area, not wiped at reset */	
  .shmem (RAM_START + RAM_SIZE - _Shared_Memory_Size): {
    __shared_memory_address__ = .;         /* define a global symbol at shared memory start */
    *(.shmem)
    *(.shmem.*)
    . = ALIGN(4);
  } >RAM
	
  .stab 0 (NOLOAD) : { *(.stab) }
  .stabstr 0 (NOLOAD) : { *(.stabstr) }
  /* DWARF debug sections.
   * Symbols in the DWARF debugging sections are relative to the beginning
   * of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }

  .note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) }
  .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) }

  /* Remove information from the standard libraries */
  /*/DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
    *(.note.GNU-stack)
  }*/
}
