# cache-sim - cs378
### Authors
- Nathan Williams
- John Smith

## Compilation
- The code can be compiled by executing the make command, which produces the ./csim binary.
- Run make release to compile without any extra console logging.
- Run make debug to compile with added logging and predictable eviction scheme (always choose first set to evict)

## Usage
- Flags
  - -f <file name of the trace to run csim on>
  - -a <custom cache associativity level; note- this applies across all memory levels>
- The default associativity is 1 for L1, 4 for L2, and 1 for DRAM. The user can specify a value from 1 to 8 for further experiments.

- Run the run.sh file to print the output of all trace files automatically.