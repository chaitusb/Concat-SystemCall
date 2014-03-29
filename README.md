Concat-SystemCall
=================

To create a new system call that can concatenate one or more files into a destination target file.

A new multi-mode system call called

	    int sys_xconcat(void *args, int argslen)

      The reason I want to use a single void * is because and it would allow us to change the number and types of args we want to pass.I would be passing a void * generic pointer to the syscall, but inside of it I would be packing all the args I need for different modes of this system call. "argslen" is the length of the void * buffer that the kernel should access.

Return value: number of bytes successfully concatenated on success;
              appropriate -errno on failure
              
Operations Performed by my SystemCall: 
=====================================
My system call should open each of the files listed in infiles, in order, read their content, and then concatenate the their content into the file named in outfile.  In Unix, this can be achieved by the cat(1) utility, for example:

$ /bin/cat file1 file2 file3 > newfile

The newly created file should have a permission mode as defined in the 'mode' parameter.  See open(2) or creat(2) for description of this parameter.

The oflags parameter should behave exactly as the open-flags parameter to the open(2) syscall; consult the man page for more info.  My systemcall supports the following flags only (they only affect the outfile): O_APPEND, O_CREAT,
O_TRUNC, O_EXCL.

In addition, the flags parameter will have the following special behavior, depending on its value:

0x00: no change in behavior (default)
0x01: return num-files instead of number of bytes written
0x02: return percentage of total data written out
0x04: "atomic" concatenation mode

Normally, the system call should return to the user-level caller the number of bytes successfully written to the output file.  If the 0x01 flag is given, it should return the number of files whose data was successfully read and written to the output file.  If the 0x02 flag is given, the system call should return the percentage of total bytes, out of all input files, that were written out to the output file (percentage should be an integer scaled 0 to 100, rounded as needed).

The most important thing system calls do first is ensure the validity of the input they are given.  I must check for ALL possible bad conditions that could occur as the result of bad inputs to the system call.

A C program called "xhw1" that will call my syscall in its different modes.  The program output some indication of success and use perror() to print out information about what errors occurred.  The program should take three arguments in the following order:

	./xhw1 [flags] outfile infile1 infile2 ...

where flags is

-a: append mode (O_APPEND)
-c: O_CREATE
-t: O_TRUNC
-e: O_EXCL
-A: Atomic concat mode
-N: return num-files instead of num-bytes written
-P: return percentage of data written out
-m ARG: set default mode to ARG (e.g., octal 755, see chmod(2) and umask(1))
-h: print short usage string

Atomic Concate (EXTRA CREDIT):
=============================

The Atomic Concate code is wrapped in

	#ifdef EXTRA_CREDIT
		// EC code here
	#else
		// base assignment code here
	#endif

Normally, my system call should open each of the input files for reading; if any file cannot be open for reading, return an appropriate error.  If you CAN open all input files for reading, then start reading them one at a time,
and concatenating their content into the output file.  If for any reason you fail to read one of the input files mid-way, or fail to write to the output file mid-way, you should by default NOT consider this an error.  Instead,
simply return the number of bytes that you successfully written to the output file.  This is similar to the behavior of the write(2) system call, and is called a "partial write."

In the "atomic mode," however, if you're unable to read any input file mid-way, or unable to write to the output file mid-way, it should be considered an error and you should return an error.  In that case, you must
also cleanup after yourself: don't leave a partially written out or modified output file (this can be tricky especially if you've already appended to an existing output file).  In other words, the only success we consider in this
atomic-concatenation mode is if you were able to read ALL input files and wrote out all of them to the output file.
