#!/usr/bin/env python3
import logging

from fs import MADFS
from plot_gc import plot_gc
from runner import Runner
from utils import root_dir, get_timestamp

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("bench_gc")


def main():
    cmake_target = "micro_gc"
    result_dir = root_dir / "results" / "micro_gc" / "exp" / get_timestamp()
    runner = Runner(cmake_target, result_dir=result_dir)
    runner.build()

    data_path = MADFS().path / "test.txt"
    runner.run(prog_args=["--gc", "--io", "-f", data_path, "-o", result_dir / "gc"])
    runner.run(prog_args=["--io", "-f", data_path, "-o", result_dir / "io"])

    plot_gc(result_dir)


if __name__ == "__main__":
    main()
