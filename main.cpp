#include <ftl/task_scheduler.h>
#include <ftl/atomic_counter.h>
#include <ftl/macros.h>
#include <vector>
#include <cstdlib>
#include <iterator>
#include <iomanip>
#include <array>
#include <random>
#include <iostream>

FTL_TASK_FUNCTION(SortSubset, std::vector<std::int64_t>::iterator input_begin, std::vector<std::int64_t>::iterator input_end){
	FTL_GET_ARGS(SortSubset, input_begin, input_end);

	auto const size = std::distance(input_begin, input_end);

	if (size < (2 << 20) / sizeof(std::int64_t)) {
		std::sort(input_begin, input_end);
	}
	else {
		auto const midpoint = input_begin + (size / 2);

		std::array<FTL_STRUCT_TYPE(SortSubset)*, 2> args {
			FTL_CREATE_ARGS_ALLOC(SortSubset, input_begin, midpoint),
			FTL_CREATE_ARGS_ALLOC(SortSubset, midpoint, input_end)
		};
		std::array<ftl::Task, 2> tasks {
			ftl::Task{SortSubset, &args[0]},
			ftl::Task{SortSubset, &args[1]},
		};

		ftl::AtomicCounter counter(scheduler);
		scheduler->AddTasks(2, tasks.data(), &counter);

		scheduler->WaitForCounter(&counter, 0);

		std::inplace_merge(input_begin, midpoint, input_end);
	}
}

void test_sorting() {
	std::size_t const data_size = (2 << 24);
	std::size_t const array_count = data_size / sizeof(std::int64_t);

	auto const get_mb_sec = [&](auto start, auto end) {
		return (data_size / static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()));
	};

	ftl::TaskScheduler task_scheduler;

	////////////////
	// Generation //
	////////////////

	std::cout << "Generating " << std::fixed << std::setprecision(1) << data_size / static_cast<double>(1024 * 1024) << "MB Data\n";

	std::mt19937 rng{ std::random_device{}() };
	std::vector<std::int64_t> array_par, array_seq;
	array_par.reserve(array_count);
	array_seq.reserve(array_count);

	auto const gen_start = std::chrono::high_resolution_clock::now();

	std::uniform_int_distribution<std::int64_t> uid(std::numeric_limits<std::int64_t>::min(),
	                                                std::numeric_limits<std::int64_t>::max());

	std::generate_n(std::back_inserter(array_par), array_count, [&] { return uid(rng); });

	auto const gen_end = std::chrono::high_resolution_clock::now();

	std::copy(array_par.begin(), array_par.end(), std::back_inserter(array_seq));

	std::cout << "Done Generating: " << std::fixed << std::setprecision(1) << get_mb_sec(gen_start, gen_end) <<  "MB/s \n";

	///////////////////
	// Parallel Sort //
	///////////////////

	auto const par_sort_start = std::chrono::high_resolution_clock::now();

	auto args = FTL_CREATE_ARGS_ALLOC(SortSubset, array_par.begin(), array_par.end());

	task_scheduler.Run(160, SortSubset, &args, 4, ftl::EmptyQueueBehavior::Spin);

	auto const par_sort_end = std::chrono::high_resolution_clock::now();

	std::cout << "Parallel Sort - Sorted: " << std::boolalpha << std::is_sorted(array_par.begin(), array_par.end()) << ". Bandwidth: " << std::fixed << std::setprecision(1) << get_mb_sec(par_sort_start, par_sort_end) << "MB/s\n";

	/////////////////////
	// Sequential Sort //
	/////////////////////

	auto const seq_sort_start = std::chrono::high_resolution_clock::now();

	std::sort(array_seq.begin(), array_seq.end());

	auto const seq_sort_end = std::chrono::high_resolution_clock::now();

	std::cout << "Sequential Sort - Sorted: " << std::boolalpha << std::is_sorted(array_seq.begin(), array_seq.end()) << ". Bandwidth: " << std::fixed << std::setprecision(1) << get_mb_sec(seq_sort_start, seq_sort_end) << "MB/s\n";

}

FTL_TASK_FUNCTION(does_nothing) {
	FTL_GET_ARGS(does_nothing);
	// nothing
}

FTL_TASK_FUNCTION(spawns_10) {
	FTL_GET_ARGS(spawns_10);

	std::array<FTL_STRUCT_TYPE(does_nothing)*, 10> subargs{};

	for (auto& arg : subargs) {
		arg = FTL_CREATE_ARGS_ALLOC(does_nothing);
	}

	std::array<ftl::Task, 10> subtasks{};

	for(std::size_t i = 0; i < 10; ++i) {
		subtasks[i].Function = does_nothing;
		subtasks[i].ArgData = &subargs[i];
	}

	ftl::AtomicCounter counter(scheduler);
	scheduler->AddTasks(10, subtasks.data(), &counter);

	scheduler->WaitForCounter(&counter, 0);
}

FTL_TASK_FUNCTION(spawn_nothings, std::size_t task_count) {
	FTL_GET_ARGS(spawn_nothings, task_count);

	std::cout << "Generating " << task_count << " tasks\n";

	std::vector<ftl::Task> tasks (task_count, ftl::Task{does_nothing, nullptr});
	std::vector<ftl::Task> subtasks (task_count / 10, ftl::Task{spawns_10, nullptr});

	auto start = std::chrono::high_resolution_clock::now();

	ftl::AtomicCounter counter(scheduler);
	scheduler->AddTasks(task_count, tasks.data(), &counter);

	scheduler->WaitForCounter(&counter, 0);

	auto end = std::chrono::high_resolution_clock::now();

	auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	auto micros = std::chrono::duration_cast<std::chrono::microseconds>(nanos);

	std::cout << "No suboperations done in " << micros.count() << "us. " << nanos.count() / task_count << "ns per operation. " << (1'000'000. / micros.count()) * task_count << " per second.\n";

	start = std::chrono::high_resolution_clock::now();

	scheduler->AddTasks(task_count, tasks.data(), &counter);

	scheduler->WaitForCounter(&counter, 0);

	end = std::chrono::high_resolution_clock::now();

	nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	micros = std::chrono::duration_cast<std::chrono::microseconds>(nanos);

	std::cout << "Suboperations done in " << micros.count() << "us. " << nanos.count() / task_count << "ns per operation. " << (1'000'000. / micros.count()) * task_count << " per second\n";
}

void test_switching_speed() {
	ftl::TaskScheduler taskScheduler;

	constexpr std::size_t task_count = (1ULL << 22);

	auto* spawner_args = FTL_CREATE_ARGS_ALLOC(spawn_nothings, task_count);

	taskScheduler.Run(100, spawn_nothings, &spawner_args);
}

int main() {
	// std::cout << "==== Sorting ===\n";
	// test_sorting();
	std::cout << "\n==== Empty Tasks ===\n";
	test_switching_speed();
}
