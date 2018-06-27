#!/bin/sh

echo "****************************************************"
echo "****** Validating GDB pretty printers support ******"
echo "****************************************************"

set -e

GDB=
if [ -f ~/.gdbinit ]; then
  GDB=`cat ~/.gdbinit | grep gppfs-0.2 | wc -l`
fi

if [ "$GDB" != "1" ]; then
  echo "****************************************************"
  echo "****** GDB pretty printer support for STLport ******"
  echo "****************************************************"
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
fi