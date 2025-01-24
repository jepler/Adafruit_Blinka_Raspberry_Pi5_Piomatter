import io
import pathlib
from contextlib import redirect_stdout

import adafruit_pioasm
import click


@click.command
@click.argument("infile")
@click.argument("outfile")
def main(infile, outfile):
    program_name = pathlib.Path(infile).stem
    program = adafruit_pioasm.Program.from_file(infile, build_debuginfo=True)

    c_program = io.StringIO()
    with redirect_stdout(c_program):
        program.print_c_program(program_name)

    with open(outfile, "w", encoding="utf-8") as out:
        print("#pragma once", file=out)
        print("", file=out)
        print(c_program.getvalue().rstrip().replace("True", "true"), file=out)

if __name__ == '__main__':
    main()
