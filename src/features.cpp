#include "features.h"
#include <float.h> // optional
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h> // for memcpy if needed

/**
 * @brief Compute time from min to max: index of max - index of min
 */
static float compute_time_min_to_max(const float *data, int n) {
  int idx_min = 0, idx_max = 0;
  float val_min = data[0], val_max = data[0];

  for (int i = 1; i < n; i++) {
    if (data[i] < val_min) {
      val_min = data[i];
      idx_min = i;
    }
    if (data[i] > val_max) {
      val_max = data[i];
      idx_max = i;
    }
  }

  return (float)(idx_max - idx_min);
}

/**
 * @brief Compute summed absolute differences: sum(abs(x[i] - x[i-1]))
 */
static float compute_summed_abs_diff(const float *data, int n) {
  float sum_abs_diff = 0.0f;
  for (int i = 1; i < n; i++) {
    sum_abs_diff += fabsf(data[i] - data[i - 1]);
  }
  return sum_abs_diff;
}

static float compute_min(const float *data, int n) {
  float min_val = data[0];
  for (int i = 1; i < n; i++) {
    if (data[i] < min_val) {
      min_val = data[i];
    }
  }
  return min_val;
}

static float compute_max(const float *data, int n) {
  float max_val = data[0];
  for (int i = 1; i < n; i++) {
    if (data[i] > max_val) {
      max_val = data[i];
    }
  }
  return max_val;
}

/**
 * @brief Compute mean of an array
 */
static float compute_mean(const float *data, int n) {
  float sum = 0.0f;
  for (int i = 0; i < n; i++)
    sum += data[i];
  return sum / (float)n;
}

/**
 * @brief Compute standard deviation (sample-based, n-1 in denominator)
 */
static float compute_std(const float *data, int n, float mean) {
  float sum_sq = 0.0f;
  for (int i = 0; i < n; i++) {
    float diff = data[i] - mean;
    sum_sq += diff * diff;
  }
  // sample-based denominator => (n-1)
  return sqrtf(sum_sq / (float)(n - 1));
}

/**
 * @brief Simple bubble sort for median
 */
static void sort_array(float *temp, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - 1 - i; j++) {
      if (temp[j] > temp[j + 1]) {
        float tmp = temp[j];
        temp[j] = temp[j + 1];
        temp[j + 1] = tmp;
      }
    }
  }
}

/**
 * @brief Compute median (destroys temp)
 */
static float compute_median(float *temp, int n) {
  sort_array(temp, n);
  if ((n % 2) == 0) {
    return 0.5f * (temp[n / 2 - 1] + temp[n / 2]);
  } else {
    return temp[n / 2];
  }
}

static float compute_median_from_data(const float *data, int n) {
  float temp[MAX_SAMPLES];
  for (int i = 0; i < n; i++) {
    temp[i] = data[i];
  }
  return compute_median(temp, n);
}

static float compute_quantile_linear(const float *data, int n, float q) {
  float temp[MAX_SAMPLES];
  for (int i = 0; i < n; i++) {
    temp[i] = data[i];
  }
  sort_array(temp, n);

  float pos = ((float)n - 1.0f) * q;
  int lower = (int)floorf(pos);
  int upper = (int)ceilf(pos);
  if (lower == upper) {
    return temp[lower];
  }
  float weight = pos - (float)lower;
  return temp[lower] + weight * (temp[upper] - temp[lower]);
}

/**
 * @brief Compute skewness matching pandas/Series.skew() behavior.
 * Uses the bias-corrected Fisher-Pearson standardized moment coefficient.
 */
static float compute_skew(const float *data, int n, float mean, float std_dev) {
  if (n < 3 || std_dev < 1e-12f) {
    return 0.0f;
  }

  float m2 = 0.0f;
  float m3 = 0.0f;
  for (int i = 0; i < n; i++) {
    float diff = data[i] - mean;
    float diff2 = diff * diff;
    m2 += diff2;
    m3 += diff2 * diff;
  }

  m2 /= (float)n;
  m3 /= (float)n;

  if (m2 < 1e-20f) {
    return 0.0f;
  }

  float g1 = m3 / powf(m2, 1.5f);
  float correction = sqrtf((float)n * (float)(n - 1)) / (float)(n - 2);
  return correction * g1;
}

/**
 * @brief Compute excess kurtosis matching pandas/Series.kurt() behavior.
 * Uses Fisher definition with small-sample bias correction.
 */
static float compute_kurtosis(const float *data, int n, float mean,
                              float std_dev) {
  if (n < 4 || std_dev < 1e-12f) {
    return 0.0f;
  }

  float m2 = 0.0f;
  float m4 = 0.0f;

  for (int i = 0; i < n; i++) {
    float diff = data[i] - mean;
    float diff2 = diff * diff;
    m2 += diff2;
    m4 += diff2 * diff2;
  }

  m2 /= (float)n;
  m4 /= (float)n;

  if (m2 < 1e-20f) {
    return 0.0f;
  }

  float g2 = (m4 / (m2 * m2)) - 3.0f;
  float n_f = (float)n;
  float correction = ((n_f - 1.0f) / ((n_f - 2.0f) * (n_f - 3.0f))) *
                     ((n_f + 1.0f) * g2 + 6.0f);
  return correction;
}

/**
 * @brief Compute RMS = sqrt( (1/n) sum(x^2) )
 */
static float compute_rms(const float *data, int n) {
  float sum_sq = 0.0f;
  for (int i = 0; i < n; i++) {
    sum_sq += data[i] * data[i];
  }
  return sqrtf(sum_sq / (float)n);
}

static float compute_abs_mean(const float *data, int n) {
  float sum_abs = 0.0f;
  for (int i = 0; i < n; i++) {
    sum_abs += fabsf(data[i]);
  }
  return sum_abs / (float)n;
}

static float compute_energy(const float *data, int n) {
  float sum_sq = 0.0f;
  for (int i = 0; i < n; i++) {
    sum_sq += data[i] * data[i];
  }
  return sum_sq;
}

static float compute_signed_max_dev_from_mean(const float *data, int n,
                                              float mean) {
  float best_signed = data[0] - mean;
  float best_abs = fabsf(best_signed);

  for (int i = 1; i < n; i++) {
    float dev = data[i] - mean;
    float dev_abs = fabsf(dev);
    if (dev_abs > best_abs) {
      best_abs = dev_abs;
      best_signed = dev;
    }
  }

  return best_signed;
}

static void compute_longest_run_start_and_len(const float *data, int n,
                                              int direction, int *out_start,
                                              int *out_len) {
  int best_start = -1;
  int best_len = 0;
  int current_start = -1;
  int current_len = 0;

  for (int i = 0; i < n - 1; i++) {
    float diff = data[i + 1] - data[i];
    int is_match = (direction < 0) ? (diff < 0.0f) : (diff > 0.0f);

    if (is_match) {
      if (current_len == 0) {
        current_start = i;
      }
      current_len += 1;
      if (current_len > best_len) {
        best_len = current_len;
        best_start = current_start;
      }
    } else {
      current_len = 0;
    }
  }

  *out_start = best_start;
  *out_len = best_len;
}

static float compute_down_before_up(const float *data, int n) {
  int down_start = -1;
  int down_len = 0;
  int up_start = -1;
  int up_len = 0;

  compute_longest_run_start_and_len(data, n, -1, &down_start, &down_len);
  compute_longest_run_start_and_len(data, n, +1, &up_start, &up_len);

  if (down_len > 0 && up_len > 0) {
    return (down_start < up_start) ? 1.0f : 0.0f;
  }
  return NAN;
}

/**
 * @brief Extract the requested features for each axis
 */
void extract_features(const float *x_arr, const float *y_arr,
                      const float *z_arr, const float *mag_arr, int n,
                      Features_t *out_feat) {
  // === 1) Means (for std, kurtosis)
  float x_mean = compute_mean(x_arr, n);
  float y_mean = compute_mean(y_arr, n);
  float z_mean = compute_mean(z_arr, n);
  float mag_mean = compute_mean(mag_arr, n);
  out_feat->x_mean = x_mean;
  out_feat->y_mean = y_mean;
  out_feat->z_mean = z_mean;
  out_feat->mag_mean = mag_mean;

  // === 2) Standard deviations
  float x_std = compute_std(x_arr, n, x_mean);
  float y_std = compute_std(y_arr, n, y_mean);
  float z_std = compute_std(z_arr, n, z_mean);
  float mag_std = compute_std(mag_arr, n, mag_mean);
  out_feat->x_std = x_std;
  out_feat->y_std = y_std;
  out_feat->z_std = z_std;
  out_feat->mag_std = mag_std;

  // === 3) Min
  out_feat->x_min = compute_min(x_arr, n);
  out_feat->y_min = compute_min(y_arr, n);
  out_feat->z_min = compute_min(z_arr, n);
  out_feat->mag_min = compute_min(mag_arr, n);

  // === 4) Max
  out_feat->x_max = compute_max(x_arr, n);
  out_feat->y_max = compute_max(y_arr, n);
  out_feat->z_max = compute_max(z_arr, n);
  out_feat->mag_max = compute_max(mag_arr, n);

  // === 5) Median
  out_feat->x_median = compute_median_from_data(x_arr, n);
  out_feat->y_median = compute_median_from_data(y_arr, n);
  out_feat->z_median = compute_median_from_data(z_arr, n);
  out_feat->mag_median = compute_median_from_data(mag_arr, n);

  // === 6) 25th percentile (linear interpolation)
  out_feat->x_q25 = compute_quantile_linear(x_arr, n, 0.25f);
  out_feat->y_q25 = compute_quantile_linear(y_arr, n, 0.25f);
  out_feat->z_q25 = compute_quantile_linear(z_arr, n, 0.25f);
  out_feat->mag_q25 = compute_quantile_linear(mag_arr, n, 0.25f);

  // === 7) 75th percentile (linear interpolation)
  out_feat->x_q75 = compute_quantile_linear(x_arr, n, 0.75f);
  out_feat->y_q75 = compute_quantile_linear(y_arr, n, 0.75f);
  out_feat->z_q75 = compute_quantile_linear(z_arr, n, 0.75f);
  out_feat->mag_q75 = compute_quantile_linear(mag_arr, n, 0.75f);

  // === 8) IQR = q75 - q25
  out_feat->x_iqr = out_feat->x_q75 - out_feat->x_q25;
  out_feat->y_iqr = out_feat->y_q75 - out_feat->y_q25;
  out_feat->z_iqr = out_feat->z_q75 - out_feat->z_q25;
  out_feat->mag_iqr = out_feat->mag_q75 - out_feat->mag_q25;

  // === 9) Absolute mean
  out_feat->x_abs_mean = compute_abs_mean(x_arr, n);
  out_feat->y_abs_mean = compute_abs_mean(y_arr, n);
  out_feat->z_abs_mean = compute_abs_mean(z_arr, n);
  out_feat->mag_abs_mean = compute_abs_mean(mag_arr, n);

  // === 10) RMS
  float x_rms = compute_rms(x_arr, n);
  float y_rms = compute_rms(y_arr, n);
  float z_rms = compute_rms(z_arr, n);
  float mag_rms = compute_rms(mag_arr, n);
  out_feat->x_rms = x_rms;
  out_feat->y_rms = y_rms;
  out_feat->z_rms = z_rms;
  out_feat->mag_rms = mag_rms;

  // === 11) Energy
  out_feat->x_energy = compute_energy(x_arr, n);
  out_feat->y_energy = compute_energy(y_arr, n);
  out_feat->z_energy = compute_energy(z_arr, n);
  out_feat->mag_energy = compute_energy(mag_arr, n);

  // === 12) Skewness
  float x_skew = compute_skew(x_arr, n, x_mean, x_std);
  float y_skew = compute_skew(y_arr, n, y_mean, y_std);
  float z_skew = compute_skew(z_arr, n, z_mean, z_std);
  float mag_skew = compute_skew(mag_arr, n, mag_mean, mag_std);
  out_feat->x_skew = x_skew;
  out_feat->y_skew = y_skew;
  out_feat->z_skew = z_skew;
  out_feat->mag_skew = mag_skew;

  // === 13) Kurtosis
  float x_kurt = compute_kurtosis(x_arr, n, x_mean, x_std);
  float y_kurt = compute_kurtosis(y_arr, n, y_mean, y_std);
  float z_kurt = compute_kurtosis(z_arr, n, z_mean, z_std);
  float mag_kurt = compute_kurtosis(mag_arr, n, mag_mean, mag_std);
  out_feat->x_kurtosis = x_kurt;
  out_feat->y_kurtosis = y_kurt;
  out_feat->z_kurtosis = z_kurt;
  out_feat->mag_kurtosis = mag_kurt;

  // === 14) Summed absolute differences
  out_feat->x_summed_abs_diff = compute_summed_abs_diff(x_arr, n);
  out_feat->y_summed_abs_diff = compute_summed_abs_diff(y_arr, n);
  out_feat->z_summed_abs_diff = compute_summed_abs_diff(z_arr, n);
  out_feat->mag_summed_abs_diff = compute_summed_abs_diff(mag_arr, n);

  // === 15) Time from min to max
  out_feat->x_time_min_to_max = compute_time_min_to_max(x_arr, n);
  out_feat->y_time_min_to_max = compute_time_min_to_max(y_arr, n);
  out_feat->z_time_min_to_max = compute_time_min_to_max(z_arr, n);
  out_feat->mag_time_min_to_max = compute_time_min_to_max(mag_arr, n);

  // === 16) Signed max deviation from mean
  out_feat->x_signed_max_dev_from_mean =
      compute_signed_max_dev_from_mean(x_arr, n, x_mean);
  out_feat->y_signed_max_dev_from_mean =
      compute_signed_max_dev_from_mean(y_arr, n, y_mean);
  out_feat->z_signed_max_dev_from_mean =
      compute_signed_max_dev_from_mean(z_arr, n, z_mean);
  out_feat->mag_signed_max_dev_from_mean =
      compute_signed_max_dev_from_mean(mag_arr, n, mag_mean);

  // === 17-19) Longest monotonic runs + down_before_up
  {
    int start_down = -1;
    int len_down = 0;
    int start_up = -1;
    int len_up = 0;

    compute_longest_run_start_and_len(x_arr, n, -1, &start_down, &len_down);
    compute_longest_run_start_and_len(x_arr, n, +1, &start_up, &len_up);
    out_feat->x_longest_down_len = (float)len_down;
    out_feat->x_longest_up_len = (float)len_up;
    out_feat->x_down_before_up = compute_down_before_up(x_arr, n);

    compute_longest_run_start_and_len(y_arr, n, -1, &start_down, &len_down);
    compute_longest_run_start_and_len(y_arr, n, +1, &start_up, &len_up);
    out_feat->y_longest_down_len = (float)len_down;
    out_feat->y_longest_up_len = (float)len_up;
    out_feat->y_down_before_up = compute_down_before_up(y_arr, n);

    compute_longest_run_start_and_len(z_arr, n, -1, &start_down, &len_down);
    compute_longest_run_start_and_len(z_arr, n, +1, &start_up, &len_up);
    out_feat->z_longest_down_len = (float)len_down;
    out_feat->z_longest_up_len = (float)len_up;
    out_feat->z_down_before_up = compute_down_before_up(z_arr, n);

    compute_longest_run_start_and_len(mag_arr, n, -1, &start_down, &len_down);
    compute_longest_run_start_and_len(mag_arr, n, +1, &start_up, &len_up);
    out_feat->mag_longest_down_len = (float)len_down;
    out_feat->mag_longest_up_len = (float)len_up;
    out_feat->mag_down_before_up = compute_down_before_up(mag_arr, n);
  }
}
void features_to_array(const Features_t *f, float out[N_FEATURES]) {
  int i = 0;

  // x-axis (19)
  out[i++] = f->x_mean;
  out[i++] = f->x_std;
  out[i++] = f->x_min;
  out[i++] = f->x_max;
  out[i++] = f->x_median;
  out[i++] = f->x_q25;
  out[i++] = f->x_q75;
  out[i++] = f->x_iqr;
  out[i++] = f->x_abs_mean;
  out[i++] = f->x_rms;
  out[i++] = f->x_energy;
  out[i++] = f->x_skew;
  out[i++] = f->x_kurtosis;
  out[i++] = f->x_summed_abs_diff;
  out[i++] = f->x_time_min_to_max;
  out[i++] = f->x_signed_max_dev_from_mean;
  out[i++] = f->x_longest_down_len;
  out[i++] = f->x_longest_up_len;
  out[i++] = f->x_down_before_up;

  // y-axis (19)
  out[i++] = f->y_mean;
  out[i++] = f->y_std;
  out[i++] = f->y_min;
  out[i++] = f->y_max;
  out[i++] = f->y_median;
  out[i++] = f->y_q25;
  out[i++] = f->y_q75;
  out[i++] = f->y_iqr;
  out[i++] = f->y_abs_mean;
  out[i++] = f->y_rms;
  out[i++] = f->y_energy;
  out[i++] = f->y_skew;
  out[i++] = f->y_kurtosis;
  out[i++] = f->y_summed_abs_diff;
  out[i++] = f->y_time_min_to_max;
  out[i++] = f->y_signed_max_dev_from_mean;
  out[i++] = f->y_longest_down_len;
  out[i++] = f->y_longest_up_len;
  out[i++] = f->y_down_before_up;

  // z-axis (19)
  out[i++] = f->z_mean;
  out[i++] = f->z_std;
  out[i++] = f->z_min;
  out[i++] = f->z_max;
  out[i++] = f->z_median;
  out[i++] = f->z_q25;
  out[i++] = f->z_q75;
  out[i++] = f->z_iqr;
  out[i++] = f->z_abs_mean;
  out[i++] = f->z_rms;
  out[i++] = f->z_energy;
  out[i++] = f->z_skew;
  out[i++] = f->z_kurtosis;
  out[i++] = f->z_summed_abs_diff;
  out[i++] = f->z_time_min_to_max;
  out[i++] = f->z_signed_max_dev_from_mean;
  out[i++] = f->z_longest_down_len;
  out[i++] = f->z_longest_up_len;
  out[i++] = f->z_down_before_up;

  // mag-axis (19)
  out[i++] = f->mag_mean;
  out[i++] = f->mag_std;
  out[i++] = f->mag_min;
  out[i++] = f->mag_max;
  out[i++] = f->mag_median;
  out[i++] = f->mag_q25;
  out[i++] = f->mag_q75;
  out[i++] = f->mag_iqr;
  out[i++] = f->mag_abs_mean;
  out[i++] = f->mag_rms;
  out[i++] = f->mag_energy;
  out[i++] = f->mag_skew;
  out[i++] = f->mag_kurtosis;
  out[i++] = f->mag_summed_abs_diff;
  out[i++] = f->mag_time_min_to_max;
  out[i++] = f->mag_signed_max_dev_from_mean;
  out[i++] = f->mag_longest_down_len;
  out[i++] = f->mag_longest_up_len;
  out[i++] = f->mag_down_before_up;
}

void apply_scaler(const float in[N_FEATURES],
                                  int16_t out[N_FEATURES]) {
  for (int i = 0; i < N_FEATURES; i++) {
    float s = maxabs_inv[i] * in[i]*32767.0f;
    if (!isfinite(s)) {
      s = 0.0f;
    }
    if (s > 32767.0f) s = 32767.0f;
    if (s < -32768.0f) s = -32768.0f;
    out[i] = (int16_t)s;
  }
}
