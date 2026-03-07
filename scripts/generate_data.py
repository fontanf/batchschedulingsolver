import os
import contextlib
import sys

generator_main = os.path.join(
        "install",
        "bin",
        "batchschedulingsolver_generator")

objectives = [
        ("makespan", "makespan"),
        ("total_flow_time", "tft"),
        ("total_weighted_flow_time", "twft"),
        ("total_tardiness", "tt"),
        ("total_weighted_tardiness", "twt"),
        ("throughput", "throughput")]

instances_paths = {}
for objective, objective_short in objectives:
    for environnement in ["single", "parallel"]:
        instances_path = os.path.join(
                "data",
                "test_" + objective_short + "_" + environnement)
        instances_path += ".txt"
        instances_paths[(objective, environnement)] = instances_path

with contextlib.ExitStack() as stack:
    files = {
            instances_path:
            stack.enter_context(open(instances_path, 'w'))
            for instances_path in instances_paths.values()}

    for objective, objective_short in objectives:
        for number_of_machines in [1, 2, 3, 4, 5]:
            environnement = "single" if number_of_machines == 1 else "parallel"
            for number_of_batches_per_machine in [1, 2, 3, 4, 5]:
                for number_of_jobs_per_batch in [1, 2, 3, 4, 5]:
                    for seed in [0, 1]:
                        instances_path = instances_paths[(objective, environnement)]

                        instance_base_path = os.path.join(
                                "tests",
                                objective + "_" + environnement,
                                objective_short
                                + f"_n{number_of_machines}"
                                f"x{number_of_batches_per_machine}"
                                f"x{number_of_jobs_per_batch}"
                                f"_s{seed}"
                                f".json")
                        instance_full_path = os.path.join("data", instance_base_path)
                        if not os.path.exists(os.path.dirname(instance_full_path)):
                            os.makedirs(os.path.dirname(instance_full_path))

                        weight_range = 1
                        if objective_short in ["twft", "twt", "throughput"]:
                            weight_range = 100

                        command = generator_main
                        command += "  --objective " + objective.replace("_weighted", "").replace("_", "-")
                        command += f"  --number-of-machines \"{number_of_machines}\""
                        command += f"  --number-of-batches-per-machine \"{number_of_batches_per_machine}\""
                        command += f"  --number-of-jobs-per-batch \"{number_of_jobs_per_batch}\""
                        command += f"  --capacity 100"
                        command += f"  --processing-times-range 100"
                        command += f"  --weights-range {weight_range}"
                        command += f"  --seed {seed}"
                        command += f"  --output \"{instance_full_path}\""
                        print(command)
                        status = os.system(command)
                        if status != 0:
                            sys.exit(1)

                        files[instances_path].write(f"{instance_base_path}\n")


instances_path = os.path.join(
        "data",
        "test_makespan_identical_sizes.txt")
instances_file = open(instances_path, 'w')
number_of_machines = 1
for number_of_batches_per_machine in [1, 2, 3, 4, 5]:
    for number_of_jobs_per_batch in [1, 2, 3, 4, 5]:
        for seed in [0, 1]:
            instance_base_path = os.path.join(
                    "tests",
                    "makespan_identical_sizes",
                    f"_n{number_of_machines}"
                    f"x{number_of_batches_per_machine}"
                    f"x{number_of_jobs_per_batch}"
                    "_identical_sizes"
                    f"_s{seed}"
                    f".json")
            instance_full_path = os.path.join("data", instance_base_path)
            if not os.path.exists(os.path.dirname(instance_full_path)):
                os.makedirs(os.path.dirname(instance_full_path))

            weight_range = 1

            command = generator_main
            command += "  --objective makespan"
            command += f"  --number-of-machines \"{number_of_machines}\""
            command += f"  --number-of-batches-per-machine \"{number_of_batches_per_machine}\""
            command += f"  --number-of-jobs-per-batch \"{number_of_jobs_per_batch}\""
            command += f"  --capacity 100"
            command += f"  --identical-sizes 1"
            command += f"  --processing-times-range 100"
            command += f"  --weights-range {weight_range}"
            command += f"  --seed {seed}"
            command += f"  --output \"{instance_full_path}\""
            print(command)
            status = os.system(command)
            if status != 0:
                sys.exit(1)

            instances_file.write(f"{instance_base_path}\n")
