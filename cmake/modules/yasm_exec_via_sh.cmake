# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# 1. Redistributions of source code must retain the copyright
#   notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products 
#   derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if (UNIX)

file(WRITE "${_filename}" 
"#!/bin/sh
# created by cmake, don't edit, changes will be lost

${_library_path_variable}=${_ld_library_path}\${${_library_path_variable}:+:\$${_library_path_variable}} \"${_executable}\" \"$@\"
")

# make it executable
# since this is only executed on UNIX, it is safe to call chmod
exec_program(chmod ARGS ug+x \"${_filename}\" OUTPUT_VARIABLE _dummy )

else (UNIX)

file(TO_NATIVE_PATH "${_ld_library_path}" win_path)

file(WRITE "${_filename}" 
"
@echo off
set PATH=${win_path};$ENV{PATH}
\"${_executable}\" %*
")

endif (UNIX)
