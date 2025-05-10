import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("readwritepercent_results.log")
df["write_percent"] = 100 - df["read_write_percent"]

plt.figure()
plt.plot(df["write_percent"], df["bandwidth_read"], marker='o', label='Read Bandwidth (KB/s)')
plt.plot(df["write_percent"], df["bandwidth_write"], marker='s', label='Write Bandwidth (KB/s)')
plt.xlabel("Write Percentage")
plt.ylabel("Bandwidth (KB/s)")
plt.title("Bandwidth at different write percentages")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("bandwidth_plot.png")

# Plot Read vs Write IOPS
plt.figure()
plt.plot(df["write_percent"], df["iops_read"], marker='o', label='Read IOPS')
plt.plot(df["write_percent"], df["iops_write"], marker='s', label='Write IOPS')
plt.xlabel("Write Percentage")
plt.ylabel("IOPS")
plt.title("IOPS at different write percentages")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("iops_plot.png")

# Plot Read vs Write Latency
plt.figure()
plt.plot(df["write_percent"], df["latency_read"], marker='o', label='Read Latency (µs)')
plt.plot(df["write_percent"], df["latency_write"], marker='s', label='Write Latency (µs)')
plt.xlabel("Write Percentage")
plt.ylabel("Latency (µs)")
plt.title("Latency at different write percentages")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("latency_plot.png")

plt.show()
