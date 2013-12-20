set(ASM_DIALECT "_YASM")
set(CMAKE_ASM${ASM_DIALECT}_SOURCE_FILE_EXTENSIONS asm)

if(X64)
    set(ASM_FLAGS "-DARCH_X86_64=1 -m amd64 -DPIC")
    if(APPLE)
        set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1 "-f macho64 -DPREFIX")
    elseif(UNIX)
        set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1 "-f elf64")
    else()
        set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1 "-f win64")
    endif()
else()
    set(ASM_FLAGS -DARCH_X86_64=0)
    if(APPLE)
        set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1 "-f macho")
    elseif(UNIX)
        set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1 "-f elf32")
    else()
        set(CMAKE_ASM${ASM_DIALECT}_COMPILER_ARG1 "-f win32 -DPREFIX")
    endif()
endif()

if (GCC)
    set(ASM_FLAGS "${ASM_FLAGS} -DHAVE_ALIGNED_STACK=1")
else()
    set(ASM_FLAGS "${ASM_FLAGS} -DHAVE_ALIGNED_STACK=0")
endif()

if (HIGH_BIT_DEPTH)
    set(ASM_FLAGS "${ASM_FLAGS} -DHIGH_BIT_DEPTH=1 -DBIT_DEPTH=10")
else()
    set(ASM_FLAGS "${ASM_FLAGS} -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8")
endif()

# This section exists to override the one in CMakeASMInformation.cmake
# (the default Information file). This removes the <FLAGS>
# thing so that your C compiler flags that have been set via
# set_target_properties don't get passed to yasm and confuse it.
if(NOT CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT)
    set(CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT "<CMAKE_ASM${ASM_DIALECT}_COMPILER> ${ASM_FLAGS} -o <OBJECT> <SOURCE>")
endif()

include(CMakeASMInformation)
set(ASM_DIALECT)
