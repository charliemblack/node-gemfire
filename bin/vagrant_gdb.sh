#!/bin/sh

echo ****************************
echo ****** Installing GDB ******
echo ****************************

set -e

sh -c "cat > ~/.gdbinit" <<'EOF'
python
import sys

sys.path.insert (0, '/vagrant/tmp/gppfs-0.2')
import stlport.printers
stlport.printers.register_stlport_printers (None)

# see the python module for a description of these options
# stlport.printers.stlport_version           = 5.2
# stlport.printers.print_vector_with_indices = False

end
EOF
