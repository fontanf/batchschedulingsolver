#include "batchschedulingsolver/instance_builder.hpp"

#include <gtest/gtest.h>

#include <fstream>

using namespace batchschedulingsolver;

TEST(InstanceBuilder, Chen2011)
{
    InstanceBuilder instance_builder;
    instance_builder.read(
            "test/instance_builder/chen2011.xml",
            "chen2011");
    const Instance instance = instance_builder.build();
    EXPECT_EQ(instance.objective(), Objective::Makespan);
    ASSERT_EQ(instance.number_of_machines(), 1);
    EXPECT_EQ(instance.machine(0).capacity, 10);
    ASSERT_EQ(instance.number_of_jobs(), 3);
    std::vector<Size> sizes = {6, 4, 5};
    std::vector<Time> processing_times = {4, 9, 4};
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        EXPECT_EQ(job.size, sizes[job_id]);
        EXPECT_EQ(job.processing_times[0], processing_times[job_id]);
        EXPECT_EQ(job.release_date, 0);
        EXPECT_EQ(job.due_date, -1);
        EXPECT_EQ(job.weight, 1);
    }
}

TEST(InstanceBuilder, Chen2011MissingField)
{
    std::string path = "chen2011_missing_field.xml";
    {
        std::ofstream file(path);
        file << "<InputDataSet><Job><ID>0</ID><Size>3</Size></Job>"
            << "<Machine><ID>0</ID><Capacity>10</Capacity></Machine></InputDataSet>";
    }
    InstanceBuilder instance_builder;
    EXPECT_THROW(instance_builder.read(path, "chen2011"), std::invalid_argument);
    std::remove(path.c_str());
}
