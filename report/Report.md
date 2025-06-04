
## Lab Report
### The purpose of the work:
To conduct a comparative analysis of the performance of sequential and parallel versions of image rotation and Gauss filtering algorithms. To prove the effectiveness of using OpenMP for parallelization of calculations in image processing tasks.

---

### Implementation description:

The solution of the previous laboratory work was taken, where the following functions were implemented on the BMP image:

- Turn 90° clockwise
- Turn 90° counterclockwise
- Application of the Gauss filter

**parallel versions of these functions** have been added to the source code using the OpenMP directives (`#pragma omp parallel for`), leaving the original sequential versions for comparison.

The testing was carried out on the same image (input.bmp) with run-time measurement via `std::chrono'.

### Experimental results:

| Operation           | Sequentially (ms) | Parallel (ms) | Boost     |
| ------------------- | ----------------- | ------------- | --------- |
| `Rotate90()`        | 25.36 мс          | 22.15 мс      | **1.14x** |
| `RotateCounter90()` | 24.81 мс          | 20.56 мс      | **1.21x** |
| `GaussianFilter()`  | 94.68 мс          | 43.96 мс      | **2.15x** |

---

### Analysis of results:

The results showed that the use of parallelism made it possible to achieve **real acceleration** of all three operations.:

- The Gauss filter demonstrated the greatest acceleration — more than **2 times faster**
- The turns also showed positive dynamics — acceleration was **~1.1–1.2x**

The moderate acceleration during rotation is explained by the fact that the operation contains fewer independent calculations and has a complex memory access structure, which limits the potential acceleration.