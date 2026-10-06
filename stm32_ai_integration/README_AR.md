# دمج موديل الإشغال مع STM32Cube.AI

هذه الملفات تجهز preprocessing المطابق لتدريب `artifacts_pir_v2`.
المشروع الفعلي لـSTM32CubeIDE غير موجود داخل مجلد العمل الحالي، لكن مجلد
`generated_network` يحتفظ بنسخة مرجعية من ملفات الشبكة التي ولدها X-CUBE-AI.

## الملفات

- `occupancy_features.h/.c`: نافذة دائرية لآخر 30 قراءة من PIR1 وPIR2،
  حساب الخصائص الست، normalization، وتحويل مدخل INT8.
- `occupancy_ai.h/.c`: غلاف صغير لتهيئة شبكة ST.AI وتشغيل inference.
- `generated_network`: ملفات `occupancy*.c/.h` المولدة من X-CUBE-AI.
- الموديل: `ai_module/tensorflow_occupancy/artifacts_pir_v2/occupancy_model_int8.tflite`.
- إعدادات التدريب: `ai_module/tensorflow_occupancy/artifacts_pir_v2/feature_config.json`.

أداة ST Edge AI 10.2.1 حللت الموديل وولدته بنجاح. النتيجة:

- input: INT8 `[1,6]`
- output: INT8 `[1,3]`
- weights: 356 bytes
- activations RAM: 1,028 bytes
- العمليات: 320 MACC لكل inference
- التطابق بين TFLite وST.AI C-model في اختبار host: 100%

## خطوات CubeMX

1. افتح ملف `.ioc` لمشروع NUCLEO-L476RG.
2. من **Software Packs > Select Components** ثبّت/فعّل X-CUBE-AI.
3. أضف Network جديدة واختر `occupancy_model_int8.tflite`.
4. شغّل **Analyze** وتأكد أن الإدخال `[1,6]` من نوع INT8 والمخرج `[1,3]`.
5. ولّد الكود وافتح المشروع في STM32CubeIDE.
6. انسخ `occupancy_features.c` و`occupancy_ai.c` إلى `Core/Src`، وانسخ
   الملفين `.h` إلى `Core/Inc`.

## منطق التشغيل

استدعِ `OccupancyFeatures_AddSample()` مرة واحدة كل ثانية. أول inference
يحدث بعد اكتمال 30 عينة، ثم كل خمس عينات (خمس ثوانٍ):

```c
static OccupancyFeatureBuffer occupancy_buffer;
static float occupancy_features[OCCUPANCY_FEATURE_COUNT];
static float occupancy_normalized[OCCUPANCY_FEATURE_COUNT];
static int8_t occupancy_input[OCCUPANCY_FEATURE_COUNT];
static int8_t occupancy_output[OCCUPANCY_CLASS_COUNT];

OccupancyFeatures_Init(&occupancy_buffer);
if (!OccupancyAI_Init())
{
  Error_Handler();
}

/* مرة كل ثانية بعد قراءة PIR1 وPIR2 */
if (OccupancyFeatures_AddSample(&occupancy_buffer, pir1_state, pir2_state))
{
  OccupancyFeatures_Compute(&occupancy_buffer, occupancy_features);
  OccupancyFeatures_Normalize(occupancy_features, occupancy_normalized);
  OccupancyFeatures_QuantizeInt8(occupancy_normalized, occupancy_input);

  if (OccupancyAI_Run(occupancy_input, occupancy_output))
  {
    OccupancyClass result = Occupancy_SelectClassInt8(occupancy_output);
    const char *label = Occupancy_ClassName(result);
  }
}
```

الغلاف الحالي يستخدم واجهة `legacy` المولدة باسم `ai_occupancy_*`.

## ترتيب المخرجات

المخرج ذو القيمة الأكبر يحدد الفئة:

- index 0: `EMPTY`
- index 1: `LOW`
- index 2: `HIGH`

تحويل خرج INT8 إلى احتمال تقريبي يتم بواسطة
`Occupancy_OutputProbability()`.

## بقية الحساسات

BH1750 وBME280 وVL53L0X تبقى في البرنامج. يستخدم موديل الإشغال الحالي
حساسي PIR فقط، ثم تُستخدم الإضاءة والحرارة والمسافة الصالحة في قواعد
التنبيه بعد inference.
