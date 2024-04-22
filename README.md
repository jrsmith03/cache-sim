# cache-sim - cs378
## Authors
- Nathan Williams
- John Smith

## Compilation
- All source code is located in the `src` directory.
- The code can be compiled by entering `src` and executing the `make release` command, which produces the `./csim` binary. The binary will be located in the root directory.
- Run `make release` to compile without any extra console logging.
- Run `make debug` to compile with added logging and predictable eviction scheme (always choose first set to evict)

## Usage
- Flags
  - `-f` (file name of the trace to run csim on)
  - `-a` (custom cache associativity level; note- this applies across all memory levels)
- The default associativity is 1 for L1, 4 for L2, and 1 for DRAM. The user can specify a value from 1 to 8 for further experiments.

### Run the `run.sh` script to print the output of all trace files automatically.
- The script will ask for the path to the trace files. If you do not uncompress the files, the script will do this automatically.
- If you wish to run cache-sim for all associativity levels (2, 4 and 8), enter 'yes' when prompted. If not, do not enter anything.
  -  __Entering nothing when prompted will simply run cache-sim on all 15 traces once.__