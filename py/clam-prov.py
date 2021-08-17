#!/usr/bin/env python3

"""
Entry point to clam-prov
"""

import argparse as a
import atexit
#from datetime import datetime
import errno
import io
import os
import os.path
import platform
import resource
import shutil
import subprocess as sub
import signal
import stats
import sys
import tempfile
import threading


root = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
verbose = True
running_process = None

####### SPECIAL ERROR CODES USEFUL FOR DEBUGGING ############
# Exit codes are between 0 and 255.
# Do not use 1, 2, 126, 127, 128 and negative integers.
## special error codes for the frontend (clang + opt + pp)
FRONTEND_TIMEOUT=20
FRONTEND_MEMORY_OUT=21
#### specific errors for each frontend component
CLANG_ERROR = 22
OPT_ERROR = 23
PP_ERROR = 24
### special error codes for crab
CRAB_ERROR = 25    ## errors caught by crab
CRAB_TIMEOUT = 26
CRAB_MEMORY_OUT = 27
CRAB_SEGFAULT = 28 ## unexpected segfaults
#############################################################

llvm_version = "11.0"

def isexec(fpath):
    if fpath is None:
        return False
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

def which(program):
    if isinstance(program, str):
        choices = [program]
    else:
        choices = program

    for p in choices:
        fpath, _ = os.path.split(p)
        if fpath:
            if isexec(p):
                return p
        else:
            for path in os.environ["PATH"].split(os.pathsep):
                exe_file = os.path.join(path, p)
                if isexec(exe_file):
                    return exe_file
    return None

# Return a tuple (returnvalue:int, timeout:bool, out_of_memory:bool, segfault:bool, unknown:bool)
#   - Only one boolean flag can be enabled at any time.
#   - If all flags are false then returnvalue cannot be None.
def run_command_with_limits(cmd, cpu, mem, out = None):
    timeout = False
    out_of_memory = False
    segfault = False
    unknown_error = False
    returnvalue = 0

    def set_limits():
        if mem > 0:
            mem_bytes = mem * 1024 * 1024
            resource.setrlimit(resource.RLIMIT_AS, [mem_bytes, mem_bytes])
    def kill(proc):
        try:
            proc.terminate()
            proc.kill()
            proc.wait()
            global running_process
            running_process = None
        except OSError:
            pass

    if out is not None:
        p = sub.Popen(cmd, stdout = out, preexec_fn=set_limits)
    else:
        p = sub.Popen(cmd, preexec_fn=set_limits)

    global running_process
    running_process = p
    timer = threading.Timer(cpu, kill, [p])
    if cpu > 0:
        timer.start()

    try:
        (_, status, _) = os.wait4(p.pid, 0)
        signalvalue = status & 0xff
        returnvalue = status >> 8
        if signalvalue != 0:
            returnvalue = None
            if signalvalue > 127:
                segfault = True
            else:
                print("** Killed by signal " + str(signalvalue))
                # 9: 'SIGKILL', 14: 'SIGALRM', 15: 'SIGTERM'
                if signalvalue in (9, 14, 15):
                    ## kill sends SIGTERM by default.
                    ## The timer set above uses kill to stop the process.
                    timeout = True
                else:
                    unknown_error = True
        running_process = None
    except OSError as e:
        returnvalue = None
        print("** OS Error: " + str(e))
        if errno.errorcode[e.errno] == 'ECHILD':
            ## The children has been killed. We assume it has been killed by the timer.
            ## But I think it can be killed by others
            timeout = True
        elif errno.errorcode[e.errno] == 'ENOMEM':
            out_of_memory = True
        else:
            unknown_error = True
    finally:
        ## kill the timer if the process has terminated already
        if timer.is_alive():
            timer.cancel()

    return (returnvalue, timeout, out_of_memory, segfault, unknown_error)

def loadEnv(filename):
    if not os.path.isfile(filename): return

    f = open(filename)
    for line in f:
        sl = line.split('=', 1)
        # skip lines without equality
        if len(sl) != 2:
            continue
        (key, val) = sl
        os.environ [key] = os.path.expandvars(val.rstrip())


def parseArgs(argv):
    def str2bool(v):
        if isinstance(v, bool):
            return v
        if v.lower() in ('yes', 'true', 't', 'y', '1'):
            return True
        if v.lower() in ('no', 'false', 'f', 'n', '0'):
            return False
        raise a.ArgumentTypeError('Boolean value expected.')

    def add_bool_argument(parser, name, default,
                          help=None, dest=None, **kwargs):
        """
        Add boolean option that can be turned on and off
        """
        dest_name = dest if dest else name
        mutex_group = parser.add_mutually_exclusive_group(required=False)
        mutex_group.add_argument('--' + name, dest=dest_name, type=str2bool,
                                 nargs='?', const=True, help=help,
                                 metavar='BOOL', **kwargs)
        mutex_group.add_argument('--no-' + name, dest=dest_name,
                                 type=lambda v: not(str2bool(v)),
                                 nargs='?', const=False,
                                 help=a.SUPPRESS, **kwargs)
        default_value = {dest_name : default}
        parser.set_defaults(**default_value)

    p = a.ArgumentParser(description='Provenance Tracking with Clam',
                         formatter_class=a.RawTextHelpFormatter)
    
    p.add_argument ('-oll', '--oll', dest='asm_out_name', metavar='FILE',
                    help='Output analyzed bitecode')
    p.add_argument('-o', dest='out_name', metavar='FILE',
                    help='Output file name')
    p.add_argument("--save-temps", dest="save_temps",
                    help="Do not delete temporary files",
                    action="store_true",
                    default=False)
    p.add_argument("--temp-dir", dest="temp_dir", metavar='DIR',
                    help="Temporary directory",
                    default=None)
    p.add_argument('--cpu', type=int, dest='cpu', metavar='SEC',
                    help='CPU time limit (seconds)', default=-1)
    p.add_argument('--mem', type=int, dest='mem', metavar='MB',
                    help='MEM limit (MB)', default=-1)
    p.add_argument('--llvm-version',
                    help='Print llvm version', dest='llvm_version',
                    default=False, action='store_true')
    p.add_argument('--clang-version',
                    help='Print clang version', dest='clang_version',
                    default=False, action='store_true')

    ## clang options
    p.add_argument('-g', default=False, action='store_true', dest='debug_info',
                    help='Compile with debug information')
    p.add_argument('-m', type=int, dest='machine',
                    help='Machine architecture MACHINE:[32,64]', default=32)
    p.add_argument ('-I', default=None, dest='include_dir', help='Include')
    p.add_argument('-O', type=int, dest='L', metavar='INT',
                    help='Optimization level L:[0,1,2,3]', default=0)
    
    ## seaopt
    p.add_argument('--llvm-inline-threshold', dest='inline_threshold',
                   # Hidden option
                   #help='Inline threshold (default = 255)',
                   help=a.SUPPRESS,
                   type=int, metavar='NUM')
    
    ## clam-pp options
    # add_bool_argument(p, 'promote-malloc',
    #                   help='Promote top-level malloc to alloca',
    #                   dest='promote_malloc', default=True)    
    p.add_argument('--inline', dest='inline', help='Inline all functions',
                    default=False, action='store_true')
    p.add_argument('--llvm-pp-loops',
                    help='Optimizing loops',
                    dest='pp_loops', default=False, action='store_true')
    p.add_argument('--llvm-peel-loops', dest='peel_loops',
                    type=int, metavar='NUM', default=0,
                    help='Number of iterations to peel (default = 0)')
    p.add_argument('--turn-undef-nondet',
                    help='Turn undefined behaviour into non-determinism',
                    dest='undef_nondet', default=False, action='store_true')
    p.add_argument('--disable-scalarize',
                    help='Disable lowering of vector operations into scalar ones',
                    dest='disable_scalarize', default=False, action='store_true')
    p.add_argument('--disable-lower-constant-expr',
                    help='Disable lowering of constant expressions to instructions',
                    dest='disable_lower_cst_expr', default=False, action='store_true')
    p.add_argument('--disable-lower-switch',
                    help='Disable lowering of switch instructions',
                    dest='disable_lower_switch', default=False, action='store_true')
    p.add_argument('--devirt-functions',
                    help="Resolve indirect calls (needed for soundness):\n"
                    "- none : do not resolve indirect calls (default)\n"
                    "- types: select all functions with same type signature\n"
                    "- sea-dsa: use sea-dsa analysis to select the callees\n",
                    dest='devirt',
                    choices=['none','types','sea-dsa'],
                    default='none')
    p.add_argument ('--externalize-functions',
                    help='Externalize these functions',
                    dest='extern_funcs', type=str, metavar='str,...')    
    p.add_argument('--externalize-addr-taken-functions',
                    help='Externalize uses of address-taken functions (potentially unsound)',
                    dest='extern_addr_taken_funcs', default=False,
                    action='store_true')
    # clang/seaopt/clam-pp options
    p.add_argument('--print-after-all',
                    help='Print IR after each pass (for debugging)',
                    dest='print_after_all', default=False,
                    action='store_true')
    p.add_argument('--debug-pass',
                    help='Print all LLVM passes executed (--debug-pass=Structure)',
                    dest='debug_pass', default=False,
                    action='store_true')
    ## clam-prov
    p.add_argument('--add-metadata-config',
                   help='File to mark sources and sinks for the Tag analysis',
                   dest='input_config', type=str, metavar='FILE')
    p.add_argument('--dependency-map-file',
                   help='Results of the Tag analysis',
                   dest='dependency_map', type=str, metavar='FILE')
    p.add_argument('--log', dest='log', default=None,
                    metavar='STR', help='Log level for clam')
    add_bool_argument(p, 'print-sources-sinks',
                      help='Print info about sources/sinks and exit',
                      dest='print_sources_sinks', default=False)
    add_bool_argument(p, 'enable-recursive',
                      help='Precise analysis of recursive calls (default false)',
                      dest='enable_recursive', default=False)
    add_bool_argument(p, 'enable-warnings',
                      help='Enable warnings messages (default false)',
                      dest='enable_warnings', default=False)
    add_bool_argument(p, 'print-invariants',
                      help='Print invariants (default false)',
                      dest='print_invariants', default=False)
    p.add_argument('--verbose',
                   help='Level of verbosity (default 0)',
                   dest='verbose', type=int, default=0)
    
    p.add_argument('file', metavar='FILE', help='Input file')

    args = p.parse_args(argv)

    if args.L < 0 or args.L > 3:
        p.error("Unknown option: -O%s" % args.L)

    if args.machine != 32 and args.machine != 64:
        p.error("Unknown option -m%s" % args.machine)

    return args

def createWorkDir(dname = None, save = False):
    if dname is None:
        workdir = tempfile.mkdtemp(prefix='clam-')
    else:
        workdir = dname

    if False: #verbose:
        print("Working directory {0}".format(workdir))

    if not save:
        atexit.register(shutil.rmtree, path=workdir)
    return workdir

def getClamPP():
    clamPP_cmd = None
    if 'CLAMPP' in os.environ:
        clamPP_cmd = os.environ ['CLAMPP']
    if not isexec(clamPP_cmd):
        clamPP_cmd = os.path.join(root, "bin/clam-pp")
    if not isexec(clamPP_cmd): clamPP_cmd = which('clam-pp')
    if not isexec(clamPP_cmd):
        raise IOError("Cannot find clam pre-processor")
    return clamPP_cmd

def getClangVersion(clang_cmd):
    p = sub.Popen([clang_cmd,'--version'], stdout = sub.PIPE)
    out, _ = p.communicate()
    clang_version = "not-found"
    found = False # true if string 'version' is found
    tokens = out.split()
    for t in tokens:
        if found is True:
            clang_version = t
            break
        if t == 'version':
            found = True
    return clang_version

def getClang(is_plus_plus):
    cmd_name = None
    if is_plus_plus:
        cmd_name = which (['clang++-mp-' + llvm_version, 'clang++-' + llvm_version, 'clang++'])
    else:
        cmd_name = which (['clang-mp-' + llvm_version, 'clang-' + llvm_version, 'clang'])
    if cmd_name is None:
        raise IOError('clang was not found')
    return cmd_name

# return a pair: the first element is the command and the second is a
# bool that it is true if seaopt has been found.
def getOptLlvm ():
    cmd_name = which (['seaopt'])
    if cmd_name is not None:
        return (cmd_name, True)

    cmd_name = which (['opt-mp-' + llvm_version, 'opt-' + llvm_version, 'opt'])
    if cmd_name is None:
        raise IOError ('neither seaopt nor opt where found')
    return (cmd_name, False)

def getClamProv():
    clam_cmd = None
    if 'CLAMPROV' in os.environ:
        clam_cmd = os.environ ['CLAMPROV']
    if not isexec(clam_cmd):
        clam_cmd = os.path.join(root, "bin/clam-prov")
    if not isexec(clam_cmd):
        clam_cmd = which('clam-prov')
    if not isexec(clam_cmd):
        raise IOError("Cannot find clam-prov")
    return clam_cmd

### Passes
def defBCName(name, wd=None):
    base = os.path.basename(name)
    if wd is None:
        wd = os.path.dirname (name)
    fname = os.path.splitext(base)[0] + '.bc'
    return os.path.join(wd, fname)
def defPPName(name, wd=None):
    base = os.path.basename(name)
    if wd is None:
        wd = os.path.dirname (name)
    fname = os.path.splitext(base)[0] + '.pp.bc'
    return os.path.join(wd, fname)
def defOptName(name, wd=None):
    base = os.path.basename(name)
    if wd is None:
        wd = os.path.dirname (name)
    fname = os.path.splitext(base)[0] + '.o.bc'
    return os.path.join(wd, fname)

def defOutPPName(name, wd=None):
    base = os.path.basename(name)
    if wd is None:
        wd = os.path.dirname (name)
    fname = os.path.splitext(base)[0] + '.ll'
    return os.path.join(wd, fname)


def _plus_plus_file(name):
    ext = os.path.splitext(name)[1]
    return ext in ('.cpp', '.cc')

# Run Clang
def clang(in_name, out_name, args, arch=32, extra_args=[]):

    if os.path.splitext(in_name)[1] == '.bc':
        if verbose:
            print('--- Clang skipped: input file is already bitecode')
        shutil.copy2(in_name, out_name)
        return

    if out_name in ('', None):
        out_name = defBCName(in_name)

    clang_cmd = getClang(_plus_plus_file(in_name))
    clang_version = getClangVersion(clang_cmd)
    if clang_version != "not-found":
        if not clang_version.startswith(llvm_version):
            print("WARNING clam-prov.py: clang version " + clang_version +  \
                  " different from " + llvm_version)

    clang_args = [clang_cmd, '-emit-llvm', '-o', out_name, '-c', in_name ]

    # New for clang >= 5.0: to avoid add optnone if -O0
    # Otherwise, seaopt cannot optimize.
    clang_args.append('-Xclang')
    clang_args.append('-disable-O0-optnone')

    clang_args.extend (extra_args)
    clang_args.append ('-m{0}'.format (arch))

    if args.include_dir is not None:
        if ':' in args.include_dir:
            idirs = ["-I{}".format(x.strip())  \
                for x in args.include_dir.split(":") if x.strip() != '']
            clang_args.extend(idirs)
        else:
            clang_args.append ('-I' + args.include_dir)

    include_dir = os.path.dirname (sys.argv[0])
    include_dir = os.path.dirname (include_dir)
    include_dir = os.path.join (include_dir, 'include')
    clang_args.append ('-I' + include_dir)


    # Disable always vectorization
    if not args.disable_scalarize:
        clang_args.append('-fno-vectorize') ## disable loop vectorization
        clang_args.append('-fno-slp-vectorize') ## disable store/load vectorization

    ## Hack for OSX Mojave that no longer exposes libc and libstd headers by default
    osx_sdk_dirs = ['/Applications/Xcode.app/Contents/Developer/Platforms/' + \
                     'MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk',
                     '/Applications/Xcode.app/Contents/Developer/Platforms/' + \
                     'MacOSX.platform/Developer/SDKs/MacOSX10.15.sdk'] + \
                    ['/Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk']

    for osx_sdk_dir in osx_sdk_dirs:
        if os.path.isdir(osx_sdk_dir):
            clang_args.append('--sysroot=' + osx_sdk_dir)
            break

    if verbose:
        print('Clang command: ' + ' '.join(clang_args))
    returnvalue, timeout, out_of_mem, segfault, unknown = \
        run_command_with_limits(clang_args, -1, -1)
    if timeout:
        sys.exit(FRONTEND_TIMEOUT)
    elif out_of_mem:
        sys.exit(FRONTEND_MEMORY_OUT)
    elif segfault or unknown or returnvalue != 0:
        sys.exit(CLANG_ERROR)

# Run llvm optimizer
def clamOpt(in_name, out_name, args, extra_args=[], cpu = -1, mem = -1):
    if out_name in ('', None):
        out_name = defOptName(in_name)
    opt_cmd, is_seaopt = getOptLlvm()

    opt_args = [opt_cmd, '-f']
    if out_name is not None: opt_args.extend(['-o', out_name])
    opt_args.append('-O{0}'.format(args.L))

    # disable sinking instructions to end of basic block
    # this might create unwanted aliasing scenarios
    # for now, there is no option to undo this switch
    opt_args.append('--simplifycfg-sink-common=false')

    # disable always vectorization
    ## With LLVM 10: loop vectorization must be enabled
    # opt_args.append('--disable-loop-vectorization')
    opt_args.append('--disable-slp-vectorization')

    ## Unavailable after porting to LLVM10
    # if is_seaopt:
    #     # disable always loop rotation. Loop rotation converts to loops
    #     # that are much harder to reason about them using crab due to
    #     # several reasons:
    #     #
    #     # 1. Complex loops that break widening heuristics
    #     # 2. Rewrite loop exits by adding often disequalities
    #     # 3. Introduce new *unsigned* loop variables.
    #     opt_args.append('--disable-loop-rotate')

    # These two should be optional
    #opt_args.append('--enable-indvar=true')
    #opt_args.append('--enable-loop-idiom=true')

    ## Unavailable after porting to LLVM10
    # if is_seaopt:
    #     if args.undef_nondet:
    #         opt_args.append('--enable-nondet-init=true')
    #     else:
    #         opt_args.append('--enable-nondet-init=false')

    if args.inline_threshold is not None:
        opt_args.append('--inline-threshold={t}'.format
                        (t=args.inline_threshold))
    # if args.unroll_threshold is not None:
    #     opt_args.append('--unroll-threshold={t}'.format
    #                     (t=args.unroll_threshold))
    if args.print_after_all: opt_args.append('--print-after-all')
    if args.debug_pass: opt_args.append('--debug-pass=Structure')
    opt_args.extend(extra_args)
    opt_args.append(in_name)

    if verbose:
        print('seaopt command: ' + ' '.join(opt_args))
    returnvalue, timeout, out_of_mem, _, unknown = \
        run_command_with_limits(opt_args, cpu, mem)
    if timeout:
        sys.exit(FRONTEND_TIMEOUT)
    elif out_of_mem:
        sys.exit(FRONTEND_MEMORY_OUT)
    elif unknown or returnvalue != 0:
        sys.exit(OPT_ERROR)

# Run clamPP
def clamPP(in_name, out_name, args, extra_args=[], cpu = -1, mem = -1):
    if out_name in ('', None):
        out_name = defPPName(in_name)

    clamPP_args = [getClamPP(), '-o', out_name, in_name ]

    # disable sinking instructions to end of basic block
    # this might create unwanted aliasing scenarios
    # for now, there is no option to undo this switch
    clamPP_args.append('--simplifycfg-sink-common=false')

    # if args.promote_malloc:
    #     clamPP_args.append('--crab-promote-malloc=true')
    # else:
    #     clamPP_args.append('--crab-promote-malloc=false')

    if args.inline:
        clamPP_args.append('--crab-inline-all')
    if args.pp_loops:
        clamPP_args.append('--clam-pp-loops')
    if args.peel_loops > 0:
        clamPP_args.append('--clam-peel-loops={0}'.format(args.peel_loops))
    if args.undef_nondet:
        clamPP_args.append('--crab-turn-undef-nondet')

    if args.disable_scalarize:
        clamPP_args.append('--crab-scalarize=false')
    else:
        # Force to scalarize everthing
        clamPP_args.append('--scalarize-load-store=true')
    if args.disable_lower_cst_expr:
        clamPP_args.append('--crab-lower-constant-expr=false')
    if args.disable_lower_switch:
        clamPP_args.append('--crab-lower-switch=false')

    # Postponed until clam is run, otherwise it can be undone by the clamOpt
    # if args.lower_unsigned_icmp:
    #     clamPP_args.append( '--crab-lower-unsigned-icmp')
    if args.devirt != 'none':
        clamPP_args.append('--crab-devirt')
        if args.devirt == 'types':
            clamPP_args.append('--devirt-resolver=types')
        elif args.devirt == 'sea-dsa':
            clamPP_args.append('--devirt-resolver=sea-dsa')
            clamPP_args.append('--sea-dsa-type-aware=true')
        elif args.devirt == 'dsa':
            clamPP_args.append('--devirt-resolver=dsa')
            
    if args.extern_funcs:
        for f in args.extern_funcs.split(','):
            clamPP_args.append('--crab-externalize-function={0}'.format(f))
    if args.extern_addr_taken_funcs:
        clamPP_args.append('--crab-externalize-addr-taken-funcs')
        
    if args.print_after_all: clamPP_args.append('--print-after-all')
    if args.debug_pass: clamPP_args.append('--debug-pass=Structure')

    clamPP_args.extend(extra_args)
    if verbose:
        print('clam-pp command: ' + ' '.join(clamPP_args))
    returnvalue, timeout, out_of_mem, segfault, unknown = \
        run_command_with_limits(clamPP_args, cpu, mem)
    if timeout:
        sys.exit(FRONTEND_TIMEOUT)
    elif out_of_mem:
        sys.exit(FRONTEND_MEMORY_OUT)
    elif segfault or unknown or returnvalue != 0:
        sys.exit(PP_ERROR)

def clamProv(in_name, out_name, args, extra_opts, cpu = -1, mem = -1):
    clam_args = [ getClamProv(), in_name, '-oll', out_name]
    clam_args = clam_args + extra_opts
        
    # disable sinking instructions to end of basic block
    # this might create unwanted aliasing scenarios
    # for now, there is no option to undo this switch
    clam_args.append('--simplifycfg-sink-common=false')

    if verbose:
        print('clam command: ' + ' '.join(clam_args))

    if args.log is not None:
        for l in args.log.split(':'): clam_args.extend(['-log', l])
        
    if args.out_name is not None:
        clam_args.append('-o={0}'.format(args.out_name))

    if args.print_after_all:
        clam_args.append('--print-after-all')

    if args.debug_pass:
        clam_args.append('--debug-pass=Structure')

    if args.print_sources_sinks:
        clam_args.append('--print-sources-sinks')
    else:
        
        if args.input_config is not None:
            clam_args.append('--add-metadata-config={0}'.format(args.input_config))

        if args.dependency_map is not None:
            clam_args.append('--dependency-map-file={0}'.format(args.dependency_map))

    if args.enable_recursive:
        clam_args.append('--enable-recursive')
    if args.enable_warnings:
        clam_args.append('--enable-warnings')
    if args.print_invariants:
        clam_args.append('--print-invariants')
    if args.verbose > 0:
        clam_args.append('--verbose={0}'.format(args.verbose))
        
    if verbose:
        print('clam-prov command: ' + ' '.join(clam_args))
            
    returnvalue, timeout, out_of_mem, segfault, unknown = \
        run_command_with_limits(clam_args, cpu, mem)
    if timeout:
        sys.exit(CRAB_TIMEOUT)
    elif out_of_mem:
        sys.exit(CRAB_MEMORY_OUT)
    elif segfault:
        sys.exit(CRAB_SEGFAULT)
    elif unknown or returnvalue != 0:
        # crab returns EXIT_FAILURE which in most platforms is 1 but not in all.
        sys.exit(CRAB_ERROR)
        
def main(argv):
    #def stat(key, val): stats.put(key, val)
    os.setpgrp()
    loadEnv(os.path.join(root, "env.common"))

    ## add directory containing this file to the PATH
    os.environ ['PATH'] =  os.path.dirname(os.path.realpath(__file__)) + \
                           os.pathsep + os.environ['PATH']

    if '--llvm-version' in argv[1:] or '-llvm-version' in argv[1:]:
        print("LLVM version " + llvm_version)
        return 0

    if '--clang-version' in argv[1:] or '-clang-version' in argv[1:]:
        print("Clang version " + getClangVersion(getClang(False)))
        return 0


    print("Platform: {0} {1}".format(platform.system(), platform.release()))
    print("LLVM version: {0}".format(llvm_version))
          
    args  = parseArgs(argv[1:])
    workdir = createWorkDir(args.temp_dir, args.save_temps)
    in_name = args.file

    bc_out = defBCName(in_name, workdir)
    if bc_out != in_name:
        extra_args = []
        if args.debug_info: extra_args.append('-g')
        with stats.timer('Clang'):
            clang(in_name, bc_out, args, arch=args.machine, extra_args=extra_args)
    in_name = bc_out

    pp_out = defPPName(in_name, workdir)
    if pp_out != in_name:
        with stats.timer('ClamPP'):
            clamPP(in_name, pp_out, args=args, cpu=args.cpu, mem=args.mem)
    in_name = pp_out

    if args.L > 0:
        o_out = defOptName(in_name, workdir)
        if o_out != in_name:
            extra_args = []
            with stats.timer('SeaOpt'):
                clamOpt(in_name, o_out, args, extra_args, cpu=args.cpu, mem=args.mem)
        in_name = o_out

    pp_out = defOutPPName(in_name, workdir)
    with stats.timer('ClamProv'):
        extra_opts = []
        clamProv(in_name, pp_out, args, extra_opts, cpu=args.cpu, mem=args.mem)

    if args.asm_out_name is not None and args.asm_out_name != pp_out:
        if False: #verbose:
            print('cp {0} {1}'.format(pp_out, args.asm_out_name))
        shutil.copy2(pp_out, args.asm_out_name)

    return 0

def killall():
    global running_process
    if running_process is not None:
        try:
            running_process.terminate()
            running_process.kill()
            running_process.wait()
            running_process = None
        except OSError: pass

if __name__ == '__main__':
    # unbuffered output
    sys.stdout = io.TextIOWrapper(open(sys.stdout.fileno(), 'wb', 0), write_through=True)    
    try:
        signal.signal(signal.SIGTERM, lambda x, y: killall())
        sys.exit(main(sys.argv))
    except KeyboardInterrupt: pass
    finally:
        killall()
        stats.brunch_print()
