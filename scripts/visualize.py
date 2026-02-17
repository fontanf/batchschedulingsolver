import matplotlib.pyplot as plt
import random
import json
import argparse
from matplotlib.patches import Patch
import plotly.express as px

def plot_schedule_with_jobs(data, save_path=None):
    _, ax = plt.subplots(figsize=(14, 6))

    yticks = []
    yticklabels = []

    machine_positions = {}
    current_y = 0

    # Fixed colors
    batch_color = 'gray'
    batch_alpha = 0.2
    batch_size_text_color = 'darkgreen'
    job_size_text_color = 'darkred'

    # Job colors
    color_list = px.colors.qualitative.Plotly
    random.shuffle(color_list)
    job_colors = {}

    for machine_id, machine in enumerate(data["machines"]):
        # Get machine capacity 
        machine_capacity = machine["capacity"]
        total_batch_height = machine_capacity

        machine_y_base = current_y
        machine_center_y = machine_y_base + total_batch_height / 2
        yticks.append(machine_center_y)
        yticklabels.append(machine_id)
        machine_positions[machine_id] = machine_y_base
        current_y += total_batch_height + 2  # spacing between machines

        for batch in machine["batches"]:
            start = batch["start"]
            batch_duration = batch["processing_time"]
            batch_size = batch.get("size", 1)

            batch_y = machine_y_base + machine_capacity / 2

            # Draw batch background with fixed height = machine capacity
            ax.barh(
                y=batch_y,
                width=batch_duration,
                left=start,
                height=machine_capacity,
                align='center',
                alpha=batch_alpha,
                edgecolor='black',
                linewidth=1.5,
                color=batch_color
            )

            # Show batch size (top-right, dynamic offset)
            ax.text(
                x=start + batch_duration/2,
                y=batch_y + machine_capacity / 2 + 0.2,
                s=f"({batch_size}, {batch_duration})",
                va='top',
                ha='center',
                fontsize=8,
                fontweight='bold',
                color=batch_size_text_color
            )

            # Stack jobs inside the batch, aligned at the bottom of batch block
            job_y_offset = batch_y - machine_capacity / 2  # bottom of batch block
            for job in batch["jobs"]:
                job_id = job["job_id"]
                job_duration = job["processing_time"]
                job_size = job["size"]

                if job_id not in job_colors:
                    job_colors[job_id] = color_list[len(job_colors) % len(color_list)]
                job_color = job_colors[job_id]

                job_y = job_y_offset + job_size / 2
                rotation = 90 if job_duration < 3 else 0
                fontsize = min(7, max(4,job_duration))
                # Draw job bar
                ax.barh(
                    y=job_y,
                    width=job_duration,
                    left=start,
                    height=job_size,
                    align='center',
                    alpha=0.9,
                    edgecolor='black',
                    color=job_color
                )

                # Show job size (in red)
                ax.text(
                    x=start + job_duration / 2,
                    y=job_y,
                    s=f"({job_size}, {job_duration})",
                    va='center',
                    ha='center',
                    fontsize=fontsize,
                    rotation=rotation,
                    color=job_size_text_color
                )

                job_y_offset += job_size  # move up for next job

    # Y-axis setup
    ax.set_yticks(yticks)
    ax.set_yticklabels(yticklabels)
    ax.invert_yaxis()

    # X-axis setup
    ax.set_xlabel("Time")
    #ax.set_title("Batch Scheduling Gantt Chart with Machine Capacity")
    ax.xaxis.grid(True, linestyle='--', which='major', color='lightgray', alpha=0.7)

    # Legend
    handles = [
        Patch(facecolor=batch_color, edgecolor='black', alpha=batch_alpha, label='Batch (machine capacity)'),
        Patch(facecolor=batch_size_text_color, label='Batch (size,duration) - text'),
        Patch(facecolor=job_size_text_color, label='Job (size,duration) - text')
    ]

    sorted_job_ids = sorted(job_colors.keys())
    for jid in sorted_job_ids:
        handles.append(Patch(facecolor=job_colors[jid], edgecolor='black', label=f"Job {jid}"))

    ax.legend(handles=handles, title="Legend", bbox_to_anchor=(1.05, 1), loc='upper left')

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path)
        print(f"Figure saved as '{save_path}'")
    else:
        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Plot batch scheduling Gantt chart with machine capacity')
    parser.add_argument('json_file', type=str, help='Path to input JSON schedule file')
    parser.add_argument('--save', type=str, default=None, help='Optional: path to save the figure (PNG format)')
    args = parser.parse_args()

    with open(args.json_file, 'r', encoding='utf-8') as f:
        data = json.load(f)
        plot_schedule_with_jobs(data, save_path=args.save)
