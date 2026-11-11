/***************************************************************************
 *   Copyright (C) 2026 by Marcelo Jorge Vieira                            *
 *   metal@alucinados.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/* Common definitions */

_head << "\n"
         "; A frame of a page or more grows one page at a time, touching each\n"
         "; one, so that neither the frame nor the first push below it can\n"
         "; skip the guard page below the stack and land in other memory.\n"
         "; Windows commits the stack only through that guard page.\n"
         "%macro  begin 1\n"
         "    push    rbp\n"
         "    mov     rbp,rsp\n"
         "  %if %1 >= 4096\n"
         "    mov     rax, %1 / 4096\n"
         "  %%probe:\n"
         "    sub     rsp, 4096\n"
         "    or      qword [rsp], 0\n"
         "    dec     rax\n"
         "    jnz     %%probe\n"
         "    %if %1 & 4095\n"
         "    sub     rsp, %1 & 4095\n"
         "    %endif\n"
         "  %elif %1\n"
         "    sub     rsp,%1\n"
         "  %endif\n"
         "%endmacro\n"
         "\n"
         "%macro return 0\n"
         "    mov rsp, rbp\n"
         "    pop rbp\n"
         "    ret\n"
         "%endmacro\n"
         "\n"
         "; Every argument takes one 8-byte slot on the stack.\n"
         "%macro addarg 1\n"
         "    push %1\n"
         "%endmacro\n"
         "\n"
         "%macro clargs 1\n"
         "    add rsp, 8 * %1\n"
         "%endmacro\n"
         "\n"
         "%macro print_lf 0\n"
         "  addarg 10\n"
         "  call print_c\n"
         "  clargs 1\n"
         "%endmacro\n"
         "\n"
         "%define SIZEOF_SLOT  8\n"
         "%define MEMORY_SIZE  1048576\n"
         "%define BUFFER_SIZE  1024\n"
         "\n";
