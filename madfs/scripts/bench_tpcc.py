#!/usr/bin/env python3

import argparse
import logging

from args import add_common_args, parse_args
from fs import available_fs
from init import init
from plot_tpcc import plot_tpcc
from runner import Runner
from utils import root_dir, system, get_timestamp

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("bench_tpcc")


def bench_tpcc(result_dir, build_type, cmake_args, fs_names, run_config):
    init()
    sql_path = root_dir / "data" / "tpcc.sql"

    if run_config["prog_args"]:
        logging.warning("prog_args will be ignored for tpcc benchmark")
    del run_config["prog_args"]

    for fs_name in fs_names:
        fs = available_fs[fs_name]

        db_path = fs.path / "tpcc.db"
        system(f"rm -f {db_path} {db_path}-shm {db_path}-wal {db_path}-journal")

        runner = Runner("tpcc_create", result_dir=result_dir / fs_name / "create", build_type=build_type)
        runner.run(cmd=f"sqlite3 {db_path} '.read {sql_path}'".split(), fs=fs, **run_config)

        runner = Runner("tpcc_load", result_dir=result_dir / fs_name / "load", build_type=build_type)
        runner.build(cmake_args=cmake_args)
        runner.run(prog_args=f"-w 4 -d {db_path}".split(), fs=fs, **run_config)

        runner = Runner("tpcc_start", result_dir=result_dir / fs_name / "start", build_type=build_type)
        runner.build(cmake_args=cmake_args)
        runner.run(prog_args=f"-w 4 -c 1 -t 200000 -d {db_path}".split(), fs=fs, **run_config)

    return result_dir


def main(**kwargs):
    result_dir = root_dir / "results" / "bench_tpcc" / get_timestamp()
    bench_tpcc(result_dir=result_dir, **kwargs)
    plot_tpcc(result_dir)
    logger.info(f"Results saved to {result_dir}")


if __name__ == "__main__":
    argparser = argparse.ArgumentParser()
    add_common_args(argparser)

    args, run_cfg = parse_args(argparser)
    logger.info(f"args={args}, run_config={run_cfg}")
    main(**vars(args), run_config=run_cfg)
