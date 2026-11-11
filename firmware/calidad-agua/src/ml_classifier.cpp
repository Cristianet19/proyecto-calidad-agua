#include <stdint.h>
#include <math.h>

#include "esp_log.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/schema/schema_generated.h"

#include "water_quality_model.h"
#include "water_quality_scaler.h"
#include "ml_classifier.h"

static const char *TAG = "ML_CLASSIFIER";

// Memoria reservada para ejecutar el modelo.
constexpr int TENSOR_ARENA_SIZE = 16 * 1024;
alignas(16) static uint8_t tensor_arena[TENSOR_ARENA_SIZE];

// Elementos de TensorFlow Lite Micro.
static const tflite::Model *model = nullptr;
static tflite::MicroInterpreter *interpreter = nullptr;

static TfLiteTensor *input = nullptr;
static TfLiteTensor *output = nullptr;

static bool initialized = false;

// Operaciones que necesita nuestra red neuronal.
static tflite::MicroMutableOpResolver<3> resolver;


extern "C" bool ml_classifier_init(void)
{
    if (initialized) {
        return true;
    }

    // Cargar modelo.
    model = tflite::GetModel(water_quality_model);

    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Incompatible model schema version");
        return false;
    }

    // Registrar operaciones.
    if (resolver.AddFullyConnected() != kTfLiteOk ||
        resolver.AddRelu() != kTfLiteOk ||
        resolver.AddLogistic() != kTfLiteOk) {

        ESP_LOGE(TAG, "Failed to register ops");
        return false;
    }

    // Crear interprete.
    static tflite::MicroInterpreter static_interpreter(
        model,
        resolver,
        tensor_arena,
        TENSOR_ARENA_SIZE
    );

    interpreter = &static_interpreter;

    // Reservar memoria para los tensores.
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors failed");
        return false;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    ESP_LOGI(TAG, "Arena used: %d bytes", (int)interpreter->arena_used_bytes());
    ESP_LOGI(TAG, "Input scale=%f zp=%d",
         input->params.scale, (int)input->params.zero_point);
    ESP_LOGI(TAG, "Output scale=%f zp=%d",
         output->params.scale, (int)output->params.zero_point);

    // Comprobar dimensiones y tipo de entrada.
    if (input->type != kTfLiteInt8 ||
        output->type != kTfLiteInt8 ||
        input->dims->data[1] != 4) {

        ESP_LOGE(TAG, "Unexpected input/output format");
        return false;
    }

    initialized = true;

    ESP_LOGI(TAG, "TinyML model initialized");

    return true;
}


extern "C" float ml_classifier_predict(
    float ph,
    float tds,
    float turbidity,
    float temperature)
{
    if (!initialized) {
        ESP_LOGE(TAG, "Model not initialized");
        return -1.0f;
    }

    // Orden utilizado durante el entrenamiento.
    float values[4] = {
        ph,
        tds,
        turbidity,
        temperature
    };

    // Normalizacion y cuantizacion INT8.
    for (int i = 0; i < 4; i++) {

        float normalized =
            (values[i] - normalization_mean[i]) /
            normalization_scale[i];

        int32_t quantized = (int32_t)roundf(
            normalized / input_quantization_scale
        ) + input_quantization_zero_point;

        // Limitar al rango INT8.
        if (quantized > 127) quantized = 127;
        if (quantized < -128) quantized = -128;

        input->data.int8[i] = (int8_t)quantized;
    }

    // Ejecutar inferencia.
    if (interpreter->Invoke() != kTfLiteOk) {
        ESP_LOGE(TAG, "Inference failed");
        return -1.0f;
    }

    // Descuantizar resultado.
    int8_t result = output->data.int8[0];

    float probability =
        (result - output_quantization_zero_point) *
        output_quantization_scale;

    return probability;
}