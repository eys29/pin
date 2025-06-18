import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import sys


benchmarks = ["2dconv", "dwt53", "histeq"]

LARGE_POSITIVE = 1e2
LARGE_NEGATIVE = 0


def read_data(filename, data_dict):
    with open(filename, "r") as file:
        raw_data = file.read()

    data = [line.split() for line in raw_data.split("\n") if line]


    for row in data:
        instr = row[0]
        address = row[1]
        float_row = [
            LARGE_POSITIVE if x == "inf" else LARGE_NEGATIVE if (x == "t" or x == "e") else float(x)
            for x in row[2:]
        ]
        
        if address in data_dict:
            data_dict[address] = np.minimum(data_dict[address], float_row)  # Consolidate by taking the minimum
        else:
            data_dict[address] = np.array(float_row)

def gen_heatmap(benchmark):
    data_dict = {}
    for sample in range(1):
        read_data("benchmarks/"+benchmark+"/random_err/sample"+str(sample)+".out", data_dict)

    # Sort addresses in reverse order
    addresses = sorted(data_dict.keys(), key=lambda x: int(x), reverse=True)
    hex_addresses = [hex(int(addr)) for addr in addresses]  # Convert to hex

    values = np.array([data_dict[address] for address in addresses])

    # Create DataFrame
    df = pd.DataFrame(values, index=hex_addresses)
    df.columns =  range(2, df.shape[1]*2+2, 2)


    # Plot heatmap
    # colors = ["#FF0000", "#a6bae2", "#002868"]  # red, purple, blue
    colors = ["#FF0000", "#e2a6a6", "#e2a6a6", "#a6bae2", "#a6bae2", "#003180"]
    cmap = mcolors.LinearSegmentedColormap.from_list("custom_cmap", colors, N=256)
    norm = mcolors.TwoSlopeNorm(vmin=LARGE_NEGATIVE , vcenter=50, vmax=LARGE_POSITIVE)

    plt.figure(figsize=(20, 15))
    map = sns.heatmap(df, cmap=cmap, norm=norm, linewidths=0.5, linecolor='black', xticklabels=1, cbar_kws={"label": "SNR (dB)"})  # Reversed colormap
    colorbar = map.collections[0].colorbar
    colorbar.set_label('SNR (dB)', fontsize=30)
    colorbar.ax.tick_params(labelsize=20)
    plt.xlabel("# Errors per Word", fontsize=30)
    plt.ylabel("Memory Address", fontsize=30)
    plt.title(benchmark, fontsize=50)
    plt.subplots_adjust(left=0.15)  # Increase this value if needed
    plt.xticks(fontsize=20)

    plt.savefig("map_"+benchmark)
    plt.show()

for bench in benchmarks:
    gen_heatmap(bench)