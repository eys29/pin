import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors


# Read data from file
with open("heatmap_multi.out", "r") as file:
    raw_data = file.read()

data = [line.split() for line in raw_data.split("\n") if line]

# Convert values to float, replacing inf and -inf with large positive and negative numbers
LARGE_POSITIVE = 1e6
LARGE_NEGATIVE = -1e6

data_dict = {}
max_len = max(len(row) for row in data) - 1  # Exclude address column

for row in data:
    address = row[0]
    float_row = [
        LARGE_POSITIVE if x == "inf" else LARGE_NEGATIVE if x == "-inf" else float(x)
        for x in row[1:]
    ]
    float_row += [np.nan] * (max_len - len(float_row))  # Pad to uniform length
    
    if address in data_dict:
        data_dict[address] = np.minimum(data_dict[address], float_row)  # Consolidate by taking the minimum
    else:
        data_dict[address] = np.array(float_row)

# Sort addresses in reverse order
addresses = sorted(data_dict.keys(), key=lambda x: int(x), reverse=True)
values = np.array([data_dict[address] for address in addresses])

# Create DataFrame
df = pd.DataFrame(values, index=addresses)

# Plot heatmap
colors = ["#bf0a30", "#a6bae2", "#002868"]  # red, purple, blue
cmap = mcolors.LinearSegmentedColormap.from_list("custom_cmap", colors, N=256)
plt.figure(figsize=(32, 32))
sns.heatmap(df, cmap=cmap, linewidths=0.5, linecolor='black', xticklabels=10, cbar_kws={"label": "SNR"})  # Reversed colormap
plt.xlabel("Bit Location")
plt.ylabel("Memory Address")
plt.title("Bit Flip Location SNR Across Memory Addresses")
plt.subplots_adjust(left=0.15)  # Increase this value if needed

plt.savefig("heatmap.png")
plt.show()
