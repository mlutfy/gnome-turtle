project('gnome-turtle', 'c',
  version : '0.2.0',
  license : 'GPL-2.0-or-later',
  default_options : ['warning_level=2', 'c_std=gnu11'],
)

gnome = import('gnome')

gtk_dep  = dependency('gtk4',        version : '>= 4.10')
adw_dep  = dependency('libadwaita-1', version : '>= 1.5')
m_dep    = meson.get_compiler('c').find_library('m', required : false)

flex  = find_program('flex')
bison = find_program('bison')

# Bison: generate the parser plus a header (turtle-parser.h) that the scanner
# includes for token definitions.
parser = custom_target('turtle-parser',
  input  : 'src/turtle-parser.y',
  output : ['turtle-parser.c', 'turtle-parser.h'],
  command : [bison, '-d', '-o', '@OUTPUT0@', '--defines=@OUTPUT1@', '@INPUT@'],
)

# Flex: the scanner.
scanner = custom_target('turtle-scanner',
  input  : 'src/turtle-scanner.l',
  output : 'turtle-scanner.c',
  command : [flex, '-o', '@OUTPUT@', '@INPUT@'],
)

sources = [
  'src/gnome-turtle.c',
  'src/turtle-helpers.c',
  parser,
  scanner,
]

executable('gnome-turtle',
  sources,
  dependencies : [gtk_dep, adw_dep, m_dep],
  c_args : ['-DVERSION="@0@"'.format(meson.project_version())],
  # generated turtle-parser.h lands in the build dir
  include_directories : include_directories('src'),
  install : true,
)

install_data('gnome-turtle.desktop',
  install_dir : get_option('datadir') / 'applications',
)

install_data('gnome-turtle.png',
  install_dir : get_option('datadir') / 'icons' / 'hicolor' / '48x48' / 'apps',
)

install_subdir('examples',
  install_dir : get_option('datadir') / 'gnome-turtle',
)
