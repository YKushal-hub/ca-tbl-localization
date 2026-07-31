import csv
import pandas as pd

def main():
    try:
        df = pd.read_csv("results/5x3_analysis_results.csv")
    except FileNotFoundError:
        print("Error: results/5x3_analysis_results.csv not found.")
        return

    # Pivot table to make architectures the columns and methods the rows
    pivot_mean = df.pivot(index="method_name", columns="architecture", values="mean_error_m")
    pivot_median = df.pivot(index="method_name", columns="architecture", values="median_error_m")
    pivot_p90 = df.pivot(index="method_name", columns="architecture", values="p90_error_m")

    # Reorder columns
    cols = ["wan", "mesh", "hybrid"]
    pivot_mean = pivot_mean[cols]
    pivot_median = pivot_median[cols]
    pivot_p90 = pivot_p90[cols]

    print("\n=== Mean Localization Error (m) ===")
    print(pivot_mean.to_string())

    print("\n=== Median Localization Error (m) ===")
    print(pivot_median.to_string())

    print("\n=== P90 Localization Error (m) ===")
    print(pivot_p90.to_string())
    
    def df_to_markdown(df):
        markdown = "| " + " | ".join(["Method"] + list(df.columns)) + " |\n"
        markdown += "| " + " | ".join(["---"] * (len(df.columns) + 1)) + " |\n"
        for index, row in df.iterrows():
            markdown += f"| {index} | " + " | ".join([f"{val:.2f}" for val in row]) + " |\n"
        return markdown

    with open("results/5x3_comparative_summary.md", "w") as f:
        f.write("# 5x3 Architecture and Method Analysis\n\n")
        f.write("## Mean Localization Error (m)\n")
        f.write(df_to_markdown(pivot_mean) + "\n\n")
        f.write("## Median Localization Error (m)\n")
        f.write(df_to_markdown(pivot_median) + "\n\n")
        f.write("## 90th Percentile Localization Error (m)\n")
        f.write(df_to_markdown(pivot_p90) + "\n\n")
        
    print("\nSummary written to results/5x3_comparative_summary.md")

if __name__ == "__main__":
    main()
