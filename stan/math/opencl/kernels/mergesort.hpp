#ifndef STAN_MATH_OPENCL_KERNELS_MERGESORT_HPP
#define STAN_MATH_OPENCL_KERNELS_MERGESORT_HPP
#ifdef STAN_OPENCL

#include <stan/math/opencl/kernel_cl.hpp>
#include <stan/math/opencl/buffer_types.hpp>
#include <stan/math/opencl/matrix_cl_view.hpp>
#include <string>

namespace stan {
namespace math {
namespace opencl_kernels {

// \cond
static const std::string mergesort_kernel_code = STRINGIFY(
    // \endcond
    /** \ingroup opencl_kernels
     */
    void merge(__global SCAL* A, __global SCAL* B, __global SCAL* res,
               int A_size, int B_size) {
      int A_i = 0;
      int B_i = 0;
      int gid = get_global_id(0);
      if (B_i < B_size && A_i < A_size) {
        while (1) {
          if (A[A_i] < B[B_i]) {
            res[A_i + B_i] = A[A_i];
            //            printf("%d in1: %d\n", gid, A_i);
            A_i++;
            //            printf("%d in2: %d\n", gid, A_i);
            if (A_i >= A_size) {
              break;
            }
          } else {
            res[A_i + B_i] = B[B_i];
            B_i++;
            if (B_i >= B_size) {
              break;
            }
          }
        }
      }
//      printf("%d mid: %d %d / %d %d\n", gid, A_i, B_i, A_size, B_size);
      for (; A_i < A_size; A_i++) {
        res[A_i + B_i] = A[A_i];
      }
      for (; B_i < B_size; B_i++) {
        res[A_i + B_i] = B[B_i];
      }
//      printf("%d end: %d %d / %d %d\n", gid, A_i, B_i, A_size, B_size);
    }

    /**
     * Searches for the index of the element that is larger than or equal to
     * given value in given range.
     */
    int binary_search(__global SCAL* input, int start, int end, SCAL value) {
      while (end > start) {
        int mid = start + (end - start) / 2;
        if (value < input[mid]) {
          end = mid;
        } else {
          start = mid + 1;
        }
      }
      return start;
    }

    __kernel void merge_step(__global SCAL* output, __global SCAL* input,
                             int run_len, int size, int tasks) {
      int gid = get_global_id(0);
      int n_threads = get_global_size(0);
      if (tasks >= n_threads) {
        for (int task_id = gid; task_id < tasks; task_id += n_threads) {
          int start_a = (2 * task_id) * run_len;
          int start_b = min(size, (2 * task_id + 1) * run_len);
          int end = min(size, (2 * task_id + 2) * run_len);
          //      printf("%d %d %d :%d %d %d \n", gid, n_threads, size, start_a,
          //      start_b,
          //             end);
          merge(input + start_a, input + start_b, output + start_a,
                start_b - start_a, end - start_b);
        }
      } else {
        int task_id = gid % tasks;
        int threads_with_task_id = n_threads / tasks;
        threads_with_task_id
            += n_threads - threads_with_task_id * tasks > task_id;
        int id_in_task = gid / tasks;

        int start_a = (2 * task_id) * run_len;
        int start_b = min(size, (2 * task_id + 1) * run_len);
        int end = min(size, (2 * task_id + 2) * run_len);

        int my_start_a
            = start_a
              + (start_b - start_a) * (long)id_in_task / threads_with_task_id;
        int my_end_a = start_a
                       + (start_b - start_a) * (long)(id_in_task + 1)
                             / threads_with_task_id;
        int my_start_b = id_in_task == 0 ? start_b
                                         : binary_search(input, start_b, end,
                                                         input[my_start_a]);
        int my_end_b
            = id_in_task == threads_with_task_id - 1
                  ? end
                  : binary_search(input, start_b, end, input[my_end_a]);

        int output_start
            = start_a + my_start_a - start_a + my_start_b - start_b;

//        printf("%d: %d %d %d  (%d %d %d) Av[%lf %lf] A %d %d B %d %d O %d \n",
//               gid, task_id, threads_with_task_id, id_in_task, start_a, start_b,
//               end, input[my_start_a], input[my_end_a], my_start_a, my_end_a,
//               my_start_b, my_end_b, output_start);

        merge(input + my_start_a, input + my_start_b, output + output_start,
              my_end_a - my_start_a, my_end_b - my_start_b);
      }
    }
    // \cond
);
// \endcond

/** \ingroup opencl_kernels
 * See the docs for \link kernels/add.hpp add_batch() \endlink
 */
const kernel_cl<out_buffer, in_buffer, int, int, int> merge_step(
    "merge_step", {"#define SCAL double\n", mergesort_kernel_code});

}  // namespace opencl_kernels
}  // namespace math
}  // namespace stan

#endif
#endif
