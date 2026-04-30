#ifndef _FEATURES_H
#define _FEATURES_H

#include <stdint.h>
#include "scaler.h"

#define MAX_LINE_LENGTH 1024
#define MAX_SAMPLES 150
#define N_FEATURES 76

typedef struct {
  float x_mean;
  float y_mean;
  float z_mean;
  float mag_mean;

  float x_std; // Standard deviation of X
  float y_std;
  float z_std;
  float mag_std;

  float x_min;
  float y_min;
  float z_min;
  float mag_min;

  float x_max;
  float y_max;
  float z_max;
  float mag_max;

  float x_median;
  float y_median;
  float z_median;
  float mag_median;

  float x_q25;
  float y_q25;
  float z_q25;
  float mag_q25;

  float x_q75;
  float y_q75;
  float z_q75;
  float mag_q75;

  float x_iqr;
  float y_iqr;
  float z_iqr;
  float mag_iqr;

  float x_abs_mean;
  float y_abs_mean;
  float z_abs_mean;
  float mag_abs_mean;

  float x_rms; // Root Mean Square of X
  float y_rms;
  float z_rms;
  float mag_rms;

  float x_energy;
  float y_energy;
  float z_energy;
  float mag_energy;

  float x_skew; // Skewness of X
  float y_skew;
  float z_skew;
  float mag_skew;

  float x_kurtosis; // Kurtosis of X
  float y_kurtosis;
  float z_kurtosis;
  float mag_kurtosis;

  float x_summed_abs_diff; // sum(abs(diff(x)))
  float y_summed_abs_diff;
  float z_summed_abs_diff;
  float mag_summed_abs_diff;

  float x_time_min_to_max; // index(max) - index(min)
  float y_time_min_to_max;
  float z_time_min_to_max;
  float mag_time_min_to_max;

  float x_signed_max_dev_from_mean;
  float y_signed_max_dev_from_mean;
  float z_signed_max_dev_from_mean;
  float mag_signed_max_dev_from_mean;

  float x_longest_down_len;
  float y_longest_down_len;
  float z_longest_down_len;
  float mag_longest_down_len;

  float x_longest_up_len;
  float y_longest_up_len;
  float z_longest_up_len;
  float mag_longest_up_len;

  float x_down_before_up;
  float y_down_before_up;
  float z_down_before_up;
  float mag_down_before_up;
} Features_t;

// type as above, same names, but int16_t
typedef struct {
  int16_t x_mean;
  int16_t y_mean;
  int16_t z_mean;
  int16_t mag_mean;

  int16_t x_std; // Standard deviation of X
  int16_t y_std;
  int16_t z_std;
  int16_t mag_std;

  int16_t x_min;
  int16_t y_min;
  int16_t z_min;
  int16_t mag_min;

  int16_t x_max;
  int16_t y_max;
  int16_t z_max;
  int16_t mag_max;

  int16_t x_median;
  int16_t y_median;
  int16_t z_median;
  int16_t mag_median;

  int16_t x_q25;
  int16_t y_q25;
  int16_t z_q25;
  int16_t mag_q25;

  int16_t x_q75;
  int16_t y_q75;
  int16_t z_q75;
  int16_t mag_q75;

  int16_t x_iqr;
  int16_t y_iqr;
  int16_t z_iqr;
  int16_t mag_iqr;

  int16_t x_abs_mean;
  int16_t y_abs_mean;
  int16_t z_abs_mean;
  int16_t mag_abs_mean;

  int16_t x_rms; // Root Mean Square of X
  int16_t y_rms;
  int16_t z_rms;
  int16_t mag_rms;

  int16_t x_energy;
  int16_t y_energy;
  int16_t z_energy;
  int16_t mag_energy;

  int16_t x_skew; // Skewness of X
  int16_t y_skew;
  int16_t z_skew;
  int16_t mag_skew;

  int16_t x_kurtosis; // Kurtosis of X
  int16_t y_kurtosis;
  int16_t z_kurtosis;
  int16_t mag_kurtosis;

  int16_t x_summed_abs_diff; // sum(abs(diff(x)))
  int16_t y_summed_abs_diff;
  int16_t z_summed_abs_diff;
  int16_t mag_summed_abs_diff;

  int16_t x_time_min_to_max; // index(max) - index(min)
  int16_t y_time_min_to_max;
  int16_t z_time_min_to_max;
  int16_t mag_time_min_to_max;

  int16_t x_signed_max_dev_from_mean;
  int16_t y_signed_max_dev_from_mean;
  int16_t z_signed_max_dev_from_mean;
  int16_t mag_signed_max_dev_from_mean;

  int16_t x_longest_down_len;
  int16_t y_longest_down_len;
  int16_t z_longest_down_len;
  int16_t mag_longest_down_len;

  int16_t x_longest_up_len;
  int16_t y_longest_up_len;
  int16_t z_longest_up_len;
  int16_t mag_longest_up_len;

  int16_t x_down_before_up;
  int16_t y_down_before_up;
  int16_t z_down_before_up;
  int16_t mag_down_before_up;
} Features_q_t;

/**
 * @brief Extract selected features from x/y/z and magnitude arrays.
 * @param x_arr Array of x-axis samples
 * @param y_arr Array of y-axis samples
 * @param z_arr Array of z-axis samples
 * @param mag_arr Array of magnitude samples
 * @param n     Number of samples
 * @param out_feat Pointer to MyFeatures_t struct to store results
 */
void extract_features(const float *x_arr, const float *y_arr,
                      const float *z_arr, const float *mag_arr, int n,
                      Features_t *out_feat);
void features_to_array(const Features_t *f, float out[N_FEATURES]);

void apply_scaler(const float in[N_FEATURES],
                                  int16_t out[N_FEATURES]);

#endif // MY_FEATURES_H