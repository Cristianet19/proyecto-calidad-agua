# Modelo de ML (TinyML)

- `water_quality_model.tflite`: modelo int8 que lleva el firmware. Red 4-16-8-1 con entradas pH, TDS, turbidez y temperatura (en ese orden) y salida: probabilidad de que el agua "requiera tratamiento". Es el mismo modelo que está en `firmware/calidad-agua/include/water_quality_model.h`.
- `EI1058_Dataset.ipynb`: cuaderno de Colab con el entrenamiento y la exportación a `.tflite`, `water_quality_model.h` y `water_quality_scaler.h`.
- Dataset: "Water Quality Dataset from Water Sources in Kenya" (Mendeley Data, DOI 10.17632/575ctg3nnr, CC BY 4.0). No se incluye en el repositorio.

## Estados

- Probabilidad < 0,40: CLEAN
- Entre 0,40 y 0,60: DOUBTFUL
- Mayor de 0,60: CONTAMINATED

## Notas

- El cuaderno no fija la semilla aleatoria: reentrenarlo produce un modelo distinto al del firmware. Si se cambia de modelo, hay que regenerar y sustituir juntos `water_quality_model.h` y `water_quality_scaler.h`.
- La etiqueta del dataset depende del tipo de fuente, no de una medición de laboratorio. El modelo reconoce perfiles de agua, no contaminantes concretos.
- La turbidez del dataset está en NTU y la del sensor del proyecto es un valor relativo: hay que reescalarla cuando se use el sensor real.