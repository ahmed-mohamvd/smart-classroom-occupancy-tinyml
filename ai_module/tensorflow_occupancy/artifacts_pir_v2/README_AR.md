# مخرجات النموذج المختار PIR v2

| الملف | وظيفته |
|---|---|
| `occupancy_model_int8.tflite` | النموذج الصغير المستخدم مع X-CUBE-AI |
| `occupancy_model_float.tflite` | نسخة TFLite عائمة للمقارنة |
| `occupancy_model.keras` | نموذج TensorFlow الأصلي |
| `feature_config.json` | ترتيب الخصائص وثوابت normalization/quantization |
| `metrics.json` | الدقة وF1 ونتائج كل فئة |
| `confusion_matrix_*.csv` | مصفوفات الخلط للتدريب والتحقق والاختبار |
| `split_summary.csv` | عدد النوافذ في كل تقسيم وفئة |
| `test_predictions.csv` | تنبؤات مجموعة الاختبار |
| `training_history.csv` | تغير نتيجة التدريب عبر epochs |

هذه هي النسخة المستخدمة في STM32. لا تستبدلها بموديل آخر دون تحديث
`occupancy_features.c` وإعادة توليد شبكة X-CUBE-AI.
