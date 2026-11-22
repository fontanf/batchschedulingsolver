"""Solve test instances with an algorithm and store the optimal solutions.

For each instance of the lists, run the algorithm with a time limit. If it
proves optimality, the instance is written to the output list, and its
solution is stored as the certificate of the instance,
'data/<instance path>_solution.json', unless the instance already has a
certificate. In this case, the value of the solution must be equal to the
value of the certificate, otherwise the script stops: two algorithms disagree
on the optimal value of the instance.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument(
        "--algorithm",
        type=str,
        required=True,
        help="algorithm proving optimality")
parser.add_argument(
        "--instances",
        type=str,
        nargs="+",
        required=True,
        help="files listing the instances, relative to 'data/'")
parser.add_argument(
        "--output",
        type=str,
        required=True,
        help="list of the instances solved to optimality")
parser.add_argument(
        "--time-limit",
        type=float,
        default=1,
        help="time limit per instance, in seconds")
parser.add_argument(
        "--max-jobs",
        type=int,
        help="skip the instances with more jobs")
args = parser.parse_args()


main = os.path.join(
        "install",
        "bin",
        "batchschedulingsolver")


def certificate_value(instance, certificate):
    """Value of a certificate, from the values of its machines."""
    objective = instance.get("objective", "Makespan").lower().replace("-", " ")
    machines = certificate["machines"]
    if objective == "makespan":
        return max((m["makespan"] for m in machines), default=0)
    if objective in ("total flow time", "tft"):
        return sum(m["total_flow_time"] for m in machines)
    if objective in ("total tardiness", "tt"):
        return sum(m["total_tardiness"] for m in machines)
    if objective in ("maximum lateness", "ml"):
        return max((m["maximum_lateness"] for m in machines), default=0)
    raise ValueError(f"unsupported objective \"{objective}\"")


number_of_new_certificates = 0
number_of_checked_certificates = 0
with open(args.output, "w") as output_file, \
        tempfile.TemporaryDirectory() as tmp_directory:
    for instances_path in args.instances:
        with open(instances_path, "r") as instances_file:
            for line in instances_file:
                instance_base_path = line.strip()
                if not instance_base_path:
                    continue
                instance_full_path = os.path.join("data", instance_base_path)
                with open(instance_full_path, "r") as instance_file:
                    instance = json.load(instance_file)
                if (args.max_jobs is not None
                        and len(instance["jobs"]) > args.max_jobs):
                    continue

                certificate_full_path = f"{instance_full_path}_solution.json"
                certificate_tmp_path = os.path.join(tmp_directory, "solution.json")
                json_output_path = os.path.join(tmp_directory, "output.json")
                command = [
                        main,
                        "--verbosity-level", "0",
                        "--input", instance_full_path,
                        "--format", "json",
                        "--algorithm", args.algorithm,
                        "--certificate", certificate_tmp_path,
                        "--output", json_output_path,
                        "--time-limit", str(args.time_limit)]
                result = subprocess.run(command)
                if result.returncode != 0:
                    print(" ".join(command))
                    sys.exit(1)

                # Keep the instance only if it is solved to optimality.
                with open(json_output_path, "r") as json_output_file:
                    json_output = json.load(json_output_file)["Output"]
                if not bool(json_output["Optimal"]):
                    continue
                value = json_output["Value"]
                if json_output["Bound"] != value:
                    continue

                if os.path.exists(certificate_full_path):
                    with open(certificate_full_path, "r") as certificate_file:
                        reference = certificate_value(
                                instance,
                                json.load(certificate_file))
                    if reference != value:
                        print(f"{instance_base_path}: the algorithm proves "
                              f"the value {value}, but the certificate has "
                              f"the value {reference}.")
                        sys.exit(1)
                    number_of_checked_certificates += 1
                else:
                    shutil.copyfile(certificate_tmp_path, certificate_full_path)
                    number_of_new_certificates += 1
                print(f"{instance_base_path} {value}")
                output_file.write(f"{instance_base_path}\n")

print(f"Checked certificates: {number_of_checked_certificates}")
print(f"New certificates: {number_of_new_certificates}")
