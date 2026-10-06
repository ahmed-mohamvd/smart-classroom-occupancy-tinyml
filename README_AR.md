# دليل ملفات مشروع VeCAD

هذا هو المدخل الرئيسي للمشروع بعد تنظيمه في 17 سبتمبر 2026. النسخة النشطة
حالياً تعتمد على حساسي PIR في نموذج الذكاء الاصطناعي، بينما تبقى قراءات
VL53L0X وBME280 وBH1750 متاحة للعرض وقواعد التنبيه.

## ابدأ من هنا

1. لجمع جلسة جديدة افتح `data_collection/README_AR.md`.
2. لتنظيف الداتا وتدريب النموذج افتح
   `ai_module/tensorflow_occupancy/README_AR.md`.
3. لدمج النموذج في STM32 افتح `stm32_ai_integration/README_AR.md`.
4. لمراجعة أكواد الحساسات افتح `firmware/README_AR.md`.
5. للعرض النهائي والمراجع افتح `documentation/README_AR.md`.

## المجلدات الرئيسية

| المجلد | وظيفته |
|---|---|
| `firmware` | نسخة كود STM32 المرجعية وأكواد اختبار الحساسات |
| `data_collection` | تسجيل CSV، الداتا الخام، التنظيف، وخطة الجلسات |
| `ai_module` | تجهيز النوافذ وتدريب نموذج TensorFlow النهائي |
| `stm32_ai_integration` | preprocessing وملفات X-CUBE-AI المولدة |
| `documentation` | العرض النهائي، PDF، الصور، ووثيقة المشروع الأصلية |
| `archive_unused_2026-09-17` | تجارب ونسخ قديمة نُقلت خارج مسار العمل النشط |
| `.venv` | بيئة Python صغيرة لبرنامج التسجيل والتنظيف |
| `.venv-tf` | بيئة TensorFlow المستخدمة في التدريب |
| `node_modules` | رابط لمكتبات Node المشتركة؛ لا يحتوي نسخة مستقلة داخل المشروع |

## الملفات النشطة المعتمدة

- الداتا المنظفة: `data_collection/collected_data/processed/occupancy_training_dataset_v7.csv`.
- خصائص النوافذ: `ai_module/tensorflow_occupancy/window_features_v2.csv`.
- تقسيم الجلسات: `ai_module/tensorflow_occupancy/split_config_v2.csv`.
- النموذج المختار: `ai_module/tensorflow_occupancy/artifacts_pir_v2/occupancy_model_int8.tflite`.
- دقة الاختبار المسجلة: 88.7%، وMacro F1 يساوي 87.5%.
- ملفات دمج STM32: `stm32_ai_integration/occupancy_*.c/.h`.

## تنبيه مهم عن main.c

`firmware/current/main_pir_filtered.c` هو أحدث ملف main محفوظ داخل هذا المجلد،
لكنه نسخة جمع داتا ولا يحتوي دمج AI النهائي الذي أُضيف داخل مشروع CubeIDE.
قبل استبدال `Core/Src/main.c` في CubeIDE، قارن الملفين ولا تنسخ هذه النسخة
فوق النسخة العاملة تلقائياً.

## سياسة الأرشيف

لم تُحذف التجارب القديمة نهائياً. نُقلت إلى
`archive_unused_2026-09-17` حتى لا تختلط بالنسخة النشطة. لا تستخدم أي ملف
من الأرشيف في Build أو تدريب جديد إلا إذا كنت تسترجع تجربة قديمة عن قصد.
