#!/usr/bin/env python3

from argparse import ArgumentParser
from pathlib import Path

from matplotlib import pyplot as plt

from plot_utils import read_files, parse_name, export_results, plot_single_bm, get_latest_result
from utils import root_dir


def plot_mt(result_dir):
    df = read_files(result_dir)
    df["benchmark"] = df["name"].apply(parse_name, args=(0,))
    df["x"] = df["name"].apply(parse_name, args=(-1,))
    xlabel = "Threads"

    benchmarks = df.groupby("benchmark")
    is_cc = "OCC" in df["label"].unique()

    for name, benchmark in benchmarks:
        benchmark["y"] = benchmark["bytes_per_second"].apply(
            lambda x: float(x) / 1024 ** 3
        )
        ylabel = "Throughput (GB/s)"

        export_results(result_dir, benchmark, name=name)

        def post_plot(ax, **kwargs):
            ax.set_xlabel(xlabel, labelpad=0)
            ax.set_ylabel(ylabel, labelpad=0)

            ax.set_xticks(['1', '4', '8', '12', '16'])

            _, ymax = ax.get_ylim()
            if ymax < 1.2:
                tick_size = 0.3
            elif ymax < 2.5:
                tick_size = 0.5
            elif ymax < 4:
                tick_size = 1
            else:
                tick_size = 2

            ymax = (int(ymax / tick_size) + 1) * tick_size
            ax.set_ylim([0, ymax])
            ax.yaxis.set_major_locator(plt.MultipleLocator(tick_size))
            if tick_size >= 1:
                ax.yaxis.set_major_formatter('  {x:.0f}')
            else:
                ax.yaxis.set_major_formatter('{x:.1f}')

            titles = {
                "unif_0R": "100% Write",
                "unif_50R": "50% Read + 50% Write",
                "unif_95R": "95% Read + 5% Write",
                "unif_100R": "100% Read",
                "zipf_4k": r"4 KB Write w/ Zipf",
                "zipf_2k": r"2 KB Write w/ Zipf",
            }

            ax.set_title(titles.get(name), pad=3, fontsize=11)

        if is_cc:
            plot_single_bm(
                benchmark,
                name=name,
                result_dir=result_dir,
                post_plot=post_plot,
                markers=("o", "v", ">", "<"),
                colors=("tab:red", "tab:cyan", "tab:purple", "tab:pink"),
            )
        else:
            plot_single_bm(
                benchmark,
                name=name,
                result_dir=result_dir,
                post_plot=post_plot,
            )


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("-r", "--result_dir", help="Directory with results", type=Path,
                        default=get_latest_result(root_dir / "results" / "micro_mt" / "exp"))
    args = parser.parse_args()
    plot_mt(args.result_dir)
