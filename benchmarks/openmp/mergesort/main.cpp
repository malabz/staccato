#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#include <omp.h>

using namespace std;
using namespace chrono;

typedef int elem_t;

size_t lenght = 0;
elem_t *data = nullptr;
elem_t *data_tmp = nullptr;
long sum_before = 0;

inline uint32_t xorshift_rand() {
	static uint32_t x = 2463534242;
	x ^= x >> 13;
	x ^= x << 17;
	x ^= x >> 5;
	return x;
}

void generate_data(size_t n) {
	lenght = n;
	data = new elem_t[n];
	data_tmp = new elem_t[n];
	sum_before = 0;
	for (size_t i = 0; i < n; ++i) {
		data[i] = xorshift_rand() % (n / 2);
		sum_before += data[i];
	}
}

bool check() {
	long s = data[0];
	for (size_t i = 1; i < lenght; ++i) {
		s += data[i];
		if (data[i] > data[i])
			return false;
	}

	return sum_before == s;
}

void mergesort(size_t left, size_t right) {
	if (right - left <= 1)
		return;

	size_t mid = (left + right) / 2;
	size_t l = left;
	size_t r = mid;

#pragma omp task shared()
	mergesort(left, mid);
#pragma omp task shared()
	mergesort(mid, right);

#pragma omp taskwait

	for (size_t i = left; i < right; i++) {
		if ((l < mid && r < right && data[l] < data[r]) || r == right) {
			data_tmp[i] = data[l];
			l++;
		} else if ((l < mid && r < right) || l == mid) {
			data_tmp[i] = data[r];
			r++;
		}
	}

	// TODO: prefetch?
	memcpy(data + left, data_tmp + left, (right - left) * sizeof(elem_t));

}
void test(size_t left, size_t right)
{
#pragma omp task shared()
	mergesort(left, right);
#pragma omp taskwait
}

int main(int argc, char *argv[])
{
	size_t n = 8e7;
	size_t nthreads = 0;

	if (argc >= 2)
		nthreads = atoi(argv[1]);
	if (argc >= 3)
		n = atoi(argv[2]);
	if (nthreads == 0)
		nthreads = thread::hardware_concurrency();

	generate_data(n);

	auto start = system_clock::now();

	omp_set_dynamic(0);
	omp_set_num_threads(nthreads);

#pragma omp parallel shared(A, B, C, n)
#pragma omp single
	test(0, n);

	auto stop = system_clock::now();

	cout << "Scheduler:  cilk\n";
	cout << "Benchmark:  mergesort\n";
	cout << "Threads:    " << nthreads << "\n";
	cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
	cout << "Input:      " << n << "\n";
	cout << "Output:     " << check() << "\n";

	return 0;
}
