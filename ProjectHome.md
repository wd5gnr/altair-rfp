This is an Altair 8800 simulator, primarily written in C++. It can run in a stand alone mode or communicate with a Briel Computers Micro 8800 device using it as a front panel.

The emulator supports up to 64K of memory and an Intel 8080 processor with an SIO board. Under Linux or Cygwin a variety of telnet ports provide debugging, tracing etc. The code will also compile under Windows but with no telnet support (traces can be sent to files, for example).