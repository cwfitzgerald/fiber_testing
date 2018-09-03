#include <ftl/task_scheduler.h>
#include <ftl/atomic_counter.h>
#include <vector>
#include <cstdlib>
#include <iterator>
#include <iomanip>
#include <array>
#include <random>
#include <iostream>

#define EXPAND(x) x
#define CONCAT_HELPER(a, b) a ## b
#define CONCAT(a, b) EXPAND(CONCAT_HELPER(a, b))
#ifdef _MSC_VER
#   define GET_ARG_COUNT(...)  INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#   define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define INTERNAL_EXPAND_ARGS_PRIVATE(...) EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#else
#   define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#endif

#define FTL_STRUCT_NAME(name) CONCAT(_argstruct_, name)

#define FTL_TASK_STRUCT(name, ...) struct FTL_STRUCT_NAME(name) { EXPAND(CONCAT(FTL_TASK_STRUCT_, GET_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)) }
#define FTL_TASK_STRUCT_3(arg2, ...) arg2; EXPAND(FTL_TASK_STRUCT_2(__VA_ARGS__))
#define FTL_TASK_STRUCT_2(arg1, ...) arg1; EXPAND(FTL_TASK_STRUCT_1(__VA_ARGS__))
#define FTL_TASK_STRUCT_1(arg0) arg0;

#define FTL_TASK_FUNCTION_PROTOTYPE(name, ...) void name(::ftl::TaskScheduler* scheduler, void* _args_impl )
#define FTL_TASK_FUNCTION(name, ...) EXPAND(FTL_TASK_STRUCT(name, __VA_ARGS__));  EXPAND(FTL_TASK_FUNCTION_PROTOTYPE(name, __VA_ARGS__))

#define FTL_GET_ARGS(name, ...) auto* _args_struct_impl = reinterpret_cast<FTL_STRUCT_NAME(name)*>(_args_impl); EXPAND(CONCAT(FTL_GET_ARGS_, GET_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__))
#define FTL_GET_ARGS_3(arg2, ...) auto&& arg2 = _args_struct_impl->arg2; EXPAND(FTL_GET_ARGS_2(__VA_ARGS__))
#define FTL_GET_ARGS_2(arg1, ...) auto&& arg1 = _args_struct_impl->arg1; EXPAND(FTL_GET_ARGS_1(__VA_ARGS__))
#define FTL_GET_ARGS_1(arg0) auto&& arg0 = _args_struct_impl->arg0;

#define FTL_CREATE_ARGS(name, ...) FTL_STRUCT_NAME(name){__VA_ARGS__}

FTL_TASK_FUNCTION(SortSubset, std::vector<std::int64_t>::iterator input_begin, std::vector<std::int64_t>::iterator input_end) {
	FTL_GET_ARGS(SortSubset, input_begin, input_end);

	auto const size = std::distance(input_begin, input_end);

	if (size < (2 << 14) / sizeof(std::int64_t)) {
		std::sort(input_begin, input_end);
	}
	else {
		auto const midpoint = input_begin + (size / 2);

		std::array<FTL_STRUCT_NAME(SortSubset), 2> args {
			FTL_CREATE_ARGS(SortSubset, input_begin, midpoint),
			FTL_CREATE_ARGS(SortSubset, midpoint, input_end)
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

int main() {
	std::size_t const data_size = (2 << 16);
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
	
	auto args = FTL_CREATE_ARGS(SortSubset, array_par.begin(), array_par.end());

	task_scheduler.Run(160, SortSubset, &args, 0, ftl::EmptyQueueBehavior::Spin);
	
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
