import argparse
import sys
import os
import json
import shutil

import brute_force


def run_command(command):
    print(command)
    status = os.system(command)
    if status != 0:
        sys.exit(1)
    print()


parser = argparse.ArgumentParser(description=(
        "Compute the reference values of test instances. With --algorithm, "
        "keep the instances solved to optimality by the algorithm and copy "
        "its solution as the certificate of the instance. With --criterion "
        "objective, compute the optimal value by brute force (see "
        "scripts/brute_force.py) and write '<instance path> <value>'."))
group = parser.add_mutually_exclusive_group(required=True)
group.add_argument(
        "--algorithm",
        type=str,
        help='algorithm proving optimality')
group.add_argument(
        "--criterion",
        type=str,
        choices=["objective"],
        help='brute force on the objective of the instances')
parser.add_argument(
        "--max-jobs",
        type=int,
        default=8,
        help='with --criterion, skip the instances with more jobs')
parser.add_argument(
        "--instances",
        type=str,
        nargs='+',
        required=True,
        help='')
parser.add_argument(
        "--output",
        type=str,
        required=True,
        help='')

args = parser.parse_args()


main = os.path.join(
        "install",
        "bin",
        "batchschedulingsolver")

if args.criterion is not None:
    with open(args.output, 'w') as output_file:
        for instances_path in args.instances:
            with open(instances_path, 'r') as instances_file:
                for line in instances_file:
                    instance_base_path = line.strip()
                    if not instance_base_path:
                        continue
                    instance_full_path = os.path.join("data", instance_base_path)
                    objective, capacities, jobs = brute_force.read_instance(
                            instance_full_path)
                    if len(jobs) > args.max_jobs:
                        continue
                    if not brute_force.supported(capacities, jobs):
                        continue
                    value = brute_force.solve(objective, capacities, jobs)
                    if value is None:
                        continue
                    print(f"{instance_base_path} {value}")
                    output_file.write(f"{instance_base_path} {value}\n")
    sys.exit(0)

with open(args.output, 'w') as output_file:
    for instances_path in args.instances:
        with open(instances_path, 'r') as instances_file:
            for line in instances_file:
                instance_base_path = line[:-1]
                instance_full_path = os.path.join("data", instance_base_path)
                certificate_full_path = f"{instance_full_path}_solution.json"
                certificate_tmp_path = f"solution.json"
                json_output_path = "output.json"
                command = (
                        main
                        + f"  --verbosity-level 1"
                        + f"  --input \"{instance_full_path}\""
                        " --format json"
                        + f"  --algorithm {args.algorithm}"
                        + f"  --certificate \"{certificate_tmp_path}\""
                        + f"  --output \"{json_output_path}\""
                        + f"  --time-limit 1")
                run_command(command)

                # If solved to optimality, add to output file.
                json_output_file = open(json_output_path, "r")
                json_data = json.load(json_output_file)
                if not bool(json_data["Output"]["Optimal"]):
                    continue
                bound = int(json_data["Output"]["Bound"])
                value = int(json_data["Output"]["Value"])
                if bound == value:
                    output_file.write(f"{instance_base_path}\n")
                    shutil.copyfile(
                            certificate_tmp_path,
                            certificate_full_path)
