#pragma once

namespace Kalman {
static const int KF_DIM = 6;

class Kalman {
   public:
    Kalman() {
        for (int i = 0; i < KF_DIM; i++) {
            x[i] = 1.0f;
            z[i] = 1.05f;
            for (int j = 0; j < KF_DIM; j++) {
                F[i][j] = (i == j) ? 1.0f : 0.1f;
                P[i][j] = (i == j) ? 1.0f : 0.0f;
                H[i][j] = (i == j) ? 1.0f : 0.0f;
                Q[i][j] = (i == j) ? 0.01f : 0.0f;
                R[i][j] = (i == j) ? 0.1f : 0.0f;
            }
        }
    }

    ~Kalman() = default;

    int run() {
        // x = F * x
        mat_mult_Nx1(F, x, temp_N);
        for (int i = 0; i < KF_DIM; i++) x[i] = temp_N[i];

        // P = F * P * F^T + Q
        mat_transpose(F, temp_NxN_1);                      // F^T
        mat_mult_NxN(F, P, temp_NxN_2);                    // F * P
        mat_mult_NxN(temp_NxN_2, temp_NxN_1, temp_NxN_3);  // (F * P) * F^T
        mat_add_NxN(temp_NxN_3, Q, P);                     // + Q

        // y = z - H * x
        mat_mult_Nx1(H, x, temp_N);
        mat_sub_Nx1(z, temp_N, temp_N);  // temp_N is now 'y'

        // S = H * P * H^T + R
        mat_transpose(H, temp_NxN_1);                      // H^T
        mat_mult_NxN(H, P, temp_NxN_2);                    // H * P
        mat_mult_NxN(temp_NxN_2, temp_NxN_1, temp_NxN_3);  // (H * P) * H^T
        mat_add_NxN(temp_NxN_3, R, temp_NxN_2);            // + R (temp_NxN_2 is now S)

        // K = P * H^T * S^-1
        mat_inv(temp_NxN_2, temp_NxN_3);          // S^-1
        mat_mult_NxN(P, temp_NxN_1, temp_NxN_2);  // P * H^T
        mat_mult_NxN(temp_NxN_2, temp_NxN_3, K);  // K

        // x = x + K * y
        mat_mult_Nx1(K, temp_N, temp_N);  // K * y
        for (int i = 0; i < KF_DIM; i++) x[i] += temp_N[i];

        // P = (I - K * H) * P
        float I[KF_DIM][KF_DIM];
        for (int i = 0; i < KF_DIM; i++)
            for (int j = 0; j < KF_DIM; j++) I[i][j] = (i == j) ? 1.0f : 0.0f;

        mat_mult_NxN(K, H, temp_NxN_1);           // K * H
        mat_sub_NxN(I, temp_NxN_1, temp_NxN_2);   // I - K * H
        mat_mult_NxN(temp_NxN_2, P, temp_NxN_1);  // (I - K * H) * P

        // Write back to P
        for (int i = 0; i < KF_DIM; i++)
            for (int j = 0; j < KF_DIM; j++) P[i][j] = temp_NxN_1[i][j];

        return 0;
    }

   private:
    static void mat_mult_NxN(float A[KF_DIM][KF_DIM], float B[KF_DIM][KF_DIM], float C[KF_DIM][KF_DIM]) {
        for (int i = 0; i < KF_DIM; i++) {
            for (int j = 0; j < KF_DIM; j++) {
                C[i][j] = 0.0f;
                for (int k = 0; k < KF_DIM; k++) {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }
    }

    static void mat_mult_Nx1(float A[KF_DIM][KF_DIM], float B[KF_DIM], float C[KF_DIM]) {
        for (int i = 0; i < KF_DIM; i++) {
            C[i] = 0.0f;
            for (int j = 0; j < KF_DIM; j++) {
                C[i] += A[i][j] * B[j];
            }
        }
    }

    static void mat_add_NxN(float A[KF_DIM][KF_DIM], float B[KF_DIM][KF_DIM], float C[KF_DIM][KF_DIM]) {
        for (int i = 0; i < KF_DIM; i++) {
            for (int j = 0; j < KF_DIM; j++) {
                C[i][j] = A[i][j] + B[i][j];
            }
        }
    }

    static void mat_sub_NxN(float A[KF_DIM][KF_DIM], float B[KF_DIM][KF_DIM], float C[KF_DIM][KF_DIM]) {
        for (int i = 0; i < KF_DIM; i++) {
            for (int j = 0; j < KF_DIM; j++) {
                C[i][j] = A[i][j] - B[i][j];
            }
        }
    }

    static void mat_sub_Nx1(float A[KF_DIM], float B[KF_DIM], float C[KF_DIM]) {
        for (int i = 0; i < KF_DIM; i++) {
            C[i] = A[i] - B[i];
        }
    }

    static void mat_transpose(float A[KF_DIM][KF_DIM], float C[KF_DIM][KF_DIM]) {
        for (int i = 0; i < KF_DIM; i++) {
            for (int j = 0; j < KF_DIM; j++) {
                C[i][j] = A[j][i];
            }
        }
    }

    static void mat_inv(float A[KF_DIM][KF_DIM], float inv[KF_DIM][KF_DIM]) {
        float temp[KF_DIM][KF_DIM];

        for (int i = 0; i < KF_DIM; i++) {
            for (int j = 0; j < KF_DIM; j++) {
                temp[i][j] = A[i][j];
                inv[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }

        for (int i = 0; i < KF_DIM; i++) {
            float pivot = temp[i][i];
            if (pivot > -1e-5f && pivot < 1e-5f) pivot = 1e-5f;

            for (int j = 0; j < KF_DIM; j++) {
                temp[i][j] /= pivot;
                inv[i][j] /= pivot;
            }
            for (int k = 0; k < KF_DIM; k++) {
                if (k != i) {
                    float factor = temp[k][i];
                    for (int j = 0; j < KF_DIM; j++) {
                        temp[k][j] -= factor * temp[i][j];
                        inv[k][j] -= factor * inv[i][j];
                    }
                }
            }
        }
    }

   private:
    float x[KF_DIM];          // State vector
    float P[KF_DIM][KF_DIM];  // Covariance matrix
    float F[KF_DIM][KF_DIM];  // State transition matrix
    float Q[KF_DIM][KF_DIM];  // Process noise covariance
    float H[KF_DIM][KF_DIM];  // Observation matrix
    float R[KF_DIM][KF_DIM];  // Measurement noise covariance
    float z[KF_DIM];          // Measurement vector

    float temp_NxN_1[KF_DIM][KF_DIM];
    float temp_NxN_2[KF_DIM][KF_DIM];
    float temp_NxN_3[KF_DIM][KF_DIM];
    float temp_N[KF_DIM];
    float K[KF_DIM][KF_DIM];
};

}  // namespace Kalman
