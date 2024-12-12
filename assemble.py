import sys
from contextlib import contextmanager
import click
import adafruit_pioasm

@contextmanager
def temporary_stdout(filename):
    old_stdout = sys.stdout
    try:
        with open(filename, "w", encoding="utf-8") as sys.stdout:
            yield sys.stdout
    finally:
        sys.stdout = old_stdout
    
@click.command
@click.argument("infile")
@click.argument("outfile")
def main(infile, outfile):
    program_name = infile.rpartition("/")[2].partition(".")[0]
    print(program_name)
    program = adafruit_pioasm.Program.from_file(infile, build_debuginfo=True)
    
    with temporary_stdout(outfile):
        program.print_c_program(program_name)

if __name__ == '__main__':
    main()
