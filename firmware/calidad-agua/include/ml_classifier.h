#include <stdbool.h>
#ifndef ML_CLASSIFIER_H
#define ML_CLASSIFIER_H

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa TensorFlow Lite Micro y carga el modelo.
bool ml_classifier_init(void);

// Devuelve la probabilidad de que el agua sea insegura (0.0 - 1.0).
// Devuelve -1.0 si ocurre algún error.
float ml_classifier_predict(float ph, float tds,
                            float turbidity, float temperature);

#ifdef __cplusplus
}
#endif

#endif