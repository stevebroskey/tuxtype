cmake_minimum_required(VERSION 3.16)
project(tuxtype VERSION 1.8.3 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Build type ─────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# ── Code coverage (enable with -DCOVERAGE=ON) ──────────────────────────────
option(COVERAGE "Enable gcov code coverage instrumentation" OFF)
if(COVERAGE)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(WARNING "Coverage is only supported with GCC or Clang")
  else()
    add_compile_options(-O0 -g --coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
    message(STATUS "Coverage instrumentation enabled")
  endif()
endif()

# ── Required dependencies (via pkg-config) ─────────────────────────────────
find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2       REQUIRED IMPORTED_TARGET sdl2>=2.0.0)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
pkg_check_modules(SDL2_TTF   REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(LIBXML2    REQUIRED IMPORTED_TARGET libxml-2.0)

# ── Optional: t4k_common (provides TTS + shared audio helpers) ─────────────
# Install from: https://github.com/tux4kids/t4kcommon
pkg_check_modules(T4K_COMMON IMPORTED_TARGET t4k_common)
if(T4K_COMMON_FOUND)
  message(STATUS "t4k_common found — TTS features enabled")
  set(HAVE_LIBT4K_COMMON 1)
else()
  message(STATUS "t4k_common NOT found — TTS will be disabled")
  set(HAVE_LIBT4K_COMMON 0)
endif()

# ── Optional: gettext / iconv (for i18n) ───────────────────────────────────
find_package(Intl QUIET)
find_package(Iconv QUIET)

# ENABLE_NLS must be set BEFORE config.h is generated.
# On Ubuntu 22/24 SDL2 headers transitively pull in libintl.h, which declares
# ngettext() as a function.  If gettext.h's #else-branch has already defined
# ngettext() as a fallback macro, the two definitions collide and the compiler
# emits "expected identifier before 'const'" errors all over globals.h.
# The fix: always tell gettext.h whether real NLS is available so it either
# (a) includes libintl.h itself first (ENABLE_NLS=1), or
# (b) deliberately skips its fallback macros and we rely on libintl directly.
# Either way libintl.h is never included twice with conflicting declarations.
if(Intl_FOUND)
  set(HAVE_NLS 1)
  message(STATUS "libintl found — NLS enabled (ENABLE_NLS=1)")
else()
  set(HAVE_NLS 0)
  message(STATUS "libintl not found — NLS disabled (ENABLE_NLS=0)")
endif()

# ── Install paths ──────────────────────────────────────────────────────────
include(GNUInstallDirs)

set(TUXTYPE_DATA_PREFIX
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/tuxtype"
    CACHE PATH "Read-only game data (images, sounds, word lists)")
set(TUXTYPE_VAR_PREFIX
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LOCALSTATEDIR}/lib/tuxtype"
    CACHE PATH "Mutable shared data (custom word lists, high scores)")
set(TUXTYPE_CONF_PREFIX
    "${CMAKE_INSTALL_SYSCONFDIR}/tuxtype"
    CACHE PATH "System-wide configuration")

# ── Generate config.h ──────────────────────────────────────────────────────
configure_file(cmake/config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

# ── Source files ───────────────────────────────────────────────────────────
set(TUXTYPE_SOURCES
  src/alphabet.c
  src/audio.c
  src/braille.c
  src/convert_utf.c
  src/editor.c
  src/globals.c
  src/input_methods.c
  src/laser.c
  src/loaders.c
  src/main.c
  src/menu.c
  src/mysetenv.c
  src/options.c
  src/pause.c
  src/pixels.c
  src/playgame.c
  src/practice.c
  src/scandir.c
  src/scripting.c
  src/SDL_extras.c
  src/setup.c
  src/snow.c
  src/theme.c
  src/titlescreen.c
)

add_executable(tuxtype ${TUXTYPE_SOURCES})

target_include_directories(tuxtype PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}"   # config.h lives here
  src                              # local headers (globals.h, funcs.h, etc.)
)

target_link_libraries(tuxtype PRIVATE
  PkgConfig::SDL2
  PkgConfig::SDL2_IMAGE
  PkgConfig::SDL2_MIXER
  PkgConfig::SDL2_TTF
  PkgConfig::LIBXML2
  m
)

if(T4K_COMMON_FOUND)
  target_link_libraries(tuxtype PRIVATE PkgConfig::T4K_COMMON)
endif()

if(Intl_FOUND)
  target_link_libraries(tuxtype PRIVATE Intl::Intl)
endif()

if(Iconv_FOUND)
  target_link_libraries(tuxtype PRIVATE Iconv::Iconv)
endif()

# Needed on some Linux distros for pthreads (used by SDL2):
find_package(Threads REQUIRED)
target_link_libraries(tuxtype PRIVATE Threads::Threads)

# ── Compile definitions ────────────────────────────────────────────────────
# ENABLE_NLS must be passed as a -D flag so gettext.h sees it even before
# config.h is included (some translation units include gettext.h very early).
target_compile_definitions(tuxtype PRIVATE ENABLE_NLS=${HAVE_NLS})

# ── Compile flags ──────────────────────────────────────────────────────────
target_compile_options(tuxtype PRIVATE
  -Wall
  -Wno-unused-result
)

# ── Install rules ──────────────────────────────────────────────────────────
install(TARGETS tuxtype RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY data/   DESTINATION "${TUXTYPE_DATA_PREFIX}")
install(DIRECTORY po/     DESTINATION "${CMAKE_INSTALL_LOCALEDIR}" OPTIONAL)

# ── Tests (enable with -DBUILD_TESTING=ON) ────────────────────────────────
include(CTest)
if(BUILD_TESTING)
  pkg_check_modules(CMOCKA IMPORTED_TARGET cmocka)
  if(CMOCKA_FOUND)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
      add_subdirectory(tests)
      message(STATUS "CMocka found — unit tests will be built")
    else()
      message(STATUS "CMocka found — no tests/ directory yet, skipping (add tests to enable)")
    endif()
  else()
    message(WARNING "cmocka not found — unit tests skipped (install libcmocka-dev)")
  endif()
endif()
