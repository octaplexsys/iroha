# Iroha resource classes

## Overview

A resource represents an operation and its circuit.

Various kinds of operation can be represented as a resource including

* Logic or arithmatic operators
* Non synthesizable operators (can be transformed to synthesizable)
* In circuit communications
* Adhoc information storage

and so on.

Resources are described like this and each resource are associated to a resource class.

    (RESOURCE
      ; resource id
      123
      ; resource class name
      add
      ; input types
      ((UINT 32) (UINT 32))
      ; output types
      ((UINT 32))
      ; parameters mainly for RTL generation
      (PARAMS))

Each insn uses a resource in a state like this.

    (INSN
      ; insn id
      456
      ; resource class
      add
      ; resource id
      123
      ; operand
      ()
      ; next states
      ()
      ; input register ids
      (1 4)
      ; output register ids. e.g. r2 <= add(r1, r4)
      (2))

Some classes of resources can have a parent or a set of children.
For example, following RESOURCE has a parent resource.

    (RESOURCE 5 shared-reg-reader
      () () (PARAMS)
      (PARENT-RESOURCE 1 2 3)

In this case, the shared-reg-reader is supposed to have a shared-reg as its parent and the shared-reg can have multiple shared-reg-reader(s), shared-reg-writer(s) and so on.

## Details

### State transition

#### tr (transition)

Specifies next state(s) to be transitioned.

This can take a 1 bit register to specifiy conditional transition to 2 states. If no condition is specified, the insn specifies 1 next state to be transitioned unconditionally.

TODO: Decide how to handle multi bit condition.

### Arithmatic operations

#### set

Assigns the value of input register to output register.

#### select

Takes 3 input values; one is 1 bit condition and outputs 2nd or 3rd value depending on the condition value.

#### add

Adds 2 input values and outputs the result.

#### sub

Subtracts 2 input[1] from input[0].

#### mul

Multiplies 2 input values and outputs the result.

#### gt

Compares 2 inputs and outputs 1 if input[0] > input[1].

#### gte

Compares 2 inputs and outputs 1 if input[0] >= input[1].

#### eq

Compares 2 inputs and outputs 1 if input[0] == input[1].

#### shift

Shift input[0] by input[1] bits to the direction specified by operand.
input[1] should be a constant.

#### bit-and

Applies bit AND operation to 2 inputs and outputs the result.

#### bit-or

Applies bit OR operation to 2 inputs and outputs the result.

#### bit-xor

Applies bit XOR operation to 2 inputs and outputs the result.

#### bit-inv

Inverts bits of 1 input and outputs the result.

#### bit-sel

Selects specified range of bits from input. The output is v[msb:lsb] where
v = input[0], msb = input[1], lsb = input[2].

#### bit-concat

Concatenates all inputs in bit wise. The order is from left to right.
e.g. input[0] :: input[1] :: ... :: input[n-1]

### Pseudo instructions

Pseudo resources are resources which can't be translated into synthesizable RTL.

#### pseudo

Pseudo insn can be used inside frontend and optimization passes.

Other passes may not be able to accept a design with pseudo resource and insns using it, so it is recommended to remove pseudo resource at some phase.

#### phi

Phi appears in optimization passes use SSA form.

#### print

Takes 1 inputs and displays the result ($display in Verilog)

#### assert

Takes 1 input and displays error message if it is false.


### Resource access

#### axi-master-port

#### axi-slave-port

#### array

Array represents internal or external memory (either ROM or RAM).

Array itself isn't synthesizable and should be converted to mapped resource.

### Synthesizable

#### mapped

Mapped resource can be used in RTL generation phase. This can represent actual bus protocol, pin names and so on.

For now, this is used only for SRAM I/F to internal and external memory.

#### ext-input

Reads from input pin connected to outside of the design.

#### ext-output

Writes to output pin connected to outside of the design.

### Communication

#### FIFO

##### fifo

A fifo can have multiple multiple readers and writers as its children.

##### fifo-reader

##### fifo-writer

#### task-call

Kicks the specified state machine in a task table.

#### Shared register

TODO: Describe notification and mailbox.

##### shared-reg

This resource holds a value and can be read or written from other tables via shared-reg-reader(s) and shared-reg-writer(s).
It also can be read or written from the belonging table.
If writes come from multiple shared-reg-writers, arbitration (fixed priority for now) selects one value.

##### shared-reg-writer

Attach to a shared-reg in another table and allow to write to it via this resource.
Multiple shared-reg-writer can write a same shared-reg.

##### shared-reg-reader

Attach to a shared-reg in another table and allow to read it via this resource.
Multiple shared-reg-writer can read from a same shared-reg.

#### Shared memory

##### shared-memory

This resource represents an array and can be accessed from other tables.

##### shared-memory-reader

##### shared-memory-writer

### Table type modifier

If there is an insn using following resource in the initial state, the meaning of table will be different from normal state machine.

#### task

The table will be configured as a task. Other tables can kick the table.

#### dataflow-in

The table will be configured as a data flow pipeline instead of a state machine.

#### ext-flow-call, ext-flow-result

#### ext-task, ext-task-done, ext-task-call, ext-task-wait

This supports common method interface proposed at here https://gist.github.com/ikwzm/bab67c180f2f1f3291998fc7dbb5fbf0

#### ticker

A counter incremented by every clock cycle. This might be used to measure performance of the circuit, optimizer and so on.
