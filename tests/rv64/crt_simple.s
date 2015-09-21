	# crt_simple.s: this file is part of the Microgrid simulator
	#
	# Copyright (C) 2013 the Microgrid project.
	#
	# This program is free software, you can redistribute it and/or
	# modify it under the terms of the GNU General Public License
	# as published by the Free Software Foundation, either version 2
	# of the License, or (at your option) any later version.
	#
	# The complete GNU General Public Licence Notice can be found as the
	# `COPYING' file in the root directory.
	#
        .text
        .align 2
	.globl _start
        .type _start, @function
_start:
        # create a pseudo-stack.
        lui     sp,%hi(_stack+4096)
        add     sp,sp,%lo(_stack+4096)
        add     sp,sp,-16

        call test

        # end simulation with exit code
        sw      a0,632(zero)
        nop
        nop
        nop
        nop
        nop
        nop
        .size   _start, .-_start
        .comm   _stack,4096,8
