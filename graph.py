import pandas as pd
import matplotlib.pyplot as plt
import os

def main():
    policies = ['First_Fit', 'Best_Fit', 'Worst_Fit']
    colors = {'First_Fit': 'blue', 'Best_Fit': 'green', 'Worst_Fit': 'red'}

    # GRAPH 1: Speed vs Size (Log-Log Scale)
    plt.figure(figsize=(14, 6))

    # Subplot 1: tmalloc() speed
    plt.subplot(1, 2, 1)
    for policy in policies:
        filename = f'speed_{policy}.csv'
        if os.path.exists(filename):
            df = pd.read_csv(filename)
            plt.plot(df['Size(Bytes)'], df['MallocTime(ns)'], marker='o', markersize=4, 
                     label=policy.replace('_', ' '), color=colors[policy])
        else:
            print(f"Warning: {filename} not found.")

    plt.xscale('log', base=2)
    plt.yscale('log', base=10) # Added Log Scale for Y-axis
    plt.xlabel('Allocation Size (Bytes) [Log Scale]')
    plt.ylabel('Time (ns) [Log Scale]')
    plt.title('tmalloc() Speed vs. Allocation Size')
    plt.legend()
    plt.grid(True, which="both", ls="--", alpha=0.5)

    # Subplot 2: tfree() speed
    plt.subplot(1, 2, 2)
    for policy in policies:
        filename = f'speed_{policy}.csv'
        if os.path.exists(filename):
            df = pd.read_csv(filename)
            plt.plot(df['Size(Bytes)'], df['FreeTime(ns)'], marker='x', markersize=4, 
                     linestyle='--', label=policy.replace('_', ' '), color=colors[policy])

    plt.xscale('log', base=2)
    plt.yscale('log', base=10) # Added Log Scale for Y-axis
    plt.xlabel('Allocation Size (Bytes) [Log Scale]')
    plt.ylabel('Time (ns) [Log Scale]')
    plt.title('tfree() Speed vs. Allocation Size')
    plt.legend()
    plt.grid(True, which="both", ls="--", alpha=0.5)

    plt.tight_layout()
    plt.savefig('speed_comparison.png')
    print("Saved speed comparison graph to 'speed_comparison.png'")

    # GRAPH 2: Memory Utilization Over Time
    plt.figure(figsize=(10, 6))
    for policy in policies:
        filename = f'utilization_{policy}.csv'
        if os.path.exists(filename):
            df = pd.read_csv(filename)
            plt.plot(df['Time(ns)'], df['Utilization(%)'], alpha=0.7, 
                     label=policy.replace('_', ' '), color=colors[policy])
        else:
            print(f"Warning: {filename} not found.")

    plt.xlabel('Time (ns)')
    plt.ylabel('Memory Utilization (%)')
    plt.title('Memory Utilization Over Time')
    plt.legend()
    plt.grid(True, alpha=0.5)
    
    plt.tight_layout()
    plt.savefig('utilization_comparison.png')
    print("Saved utilization graph to 'utilization_comparison.png'")

    # Show the plots on the screen
    plt.show()

if __name__ == "__main__":
    main()