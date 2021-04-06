require 'mkmf'

CONFIG['warnflags'].gsub!(/-W.* /, '')

$srcs = %w{src/rn.c src/rnc.c src/rnd.c src/rnl.c src/rnv.c src/rnx.c src/drv.c src/ary.c src/xsd.c src/xsd_tm.c
src/sc.c src/u.c src/ht.c src/er.c src/xmlc.c src/s.c src/m.c src/rx.c ruby_rnv.c}

$INCFLAGS << " -I$(srcdir)/src"

# add folder, where compiler can search source files
$VPATH << "$(srcdir)/src"

extension_name = 'rnv'
dir_config(extension_name)
create_makefile(extension_name)
