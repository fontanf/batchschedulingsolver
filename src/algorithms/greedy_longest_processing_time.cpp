#include "batchschedulingsolver/algorithms/greedy_longest_processing_time.hpp"

#include "batchschedulingsolver/algorithm_formatter.hpp"


using namespace batchschedulingsolver;

Output batchschedulingsolver::greedy_longest_processing_time(
    const Instance& instance,
    const Parameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance,parameters,output);
    algorithm_formatter.start("greedy_longest_processing_time");

    /*Solution sol = SolutionBuilder()
        .set_instance(instance)
        .build();*/

    SolutionBuilder builder;
    builder.set_instance(instance);



	MachineId machine_id = 0; // assuming single machine for LPT
    
    /*Solution::Machine solution_machine = sol.machine(machine_id);
    std::cout << "###########################: ";
    std::cout << (BatchId)solution_machine.batches.size() << " ";*/


    // create table containing job_id in descending order
    std::vector<JobId> indices(instance.number_of_jobs());
    for (JobId i = 0; i < instance.number_of_jobs(); ++i) {
        indices[i] = i;
    }

    // sort id depending on job processing time
    std::sort(indices.begin(), indices.end(), [&instance,machine_id](int i, int j) {
        return instance.job(i).processing_times[machine_id] > instance.job(j).processing_times[machine_id];
        });

	Time max_time_current_batch = 0;
    Size current_batch_size = 0;
	Time start_time = 0;    
    const Size machine_capacity = instance.machine(machine_id).capacity;
	// create first batch
    builder.append_batch(machine_id, start_time);
    for (JobId i :indices) {
       if(current_batch_size+instance.job(i).size <=instance.machine(machine_id).capacity){
           // if job can be added to current batch
           builder.add_job_to_last_batch(machine_id, i);
           max_time_current_batch = std::max(max_time_current_batch,instance.job(i).processing_times[machine_id]);
           current_batch_size += instance.job(i).size;
       } else {
           // create new batch
           start_time += max_time_current_batch;
           builder.append_batch(machine_id, start_time);
           builder.add_job_to_last_batch(machine_id, i);
           current_batch_size = instance.job(i).size;
	   }
	   
    }

	Solution sol = builder.build();
    algorithm_formatter.update_solution(sol, "greedy");
    algorithm_formatter.print_header();

    algorithm_formatter.end();
    return output;
}