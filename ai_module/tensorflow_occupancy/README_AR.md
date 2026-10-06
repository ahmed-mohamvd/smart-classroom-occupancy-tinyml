# تدريب موديل إشغال الغرفة باستخدام TensorFlow

هذه هي سلسلة التدريب النشطة الوحيدة في المشروع. النموذج المعتمد هو
`artifacts_pir_v2`؛ بقية التجارب القديمة نُقلت إلى الأرشيف.

هذا المسار يحول القراءات المتزامنة إلى نوافذ من 30 قراءة، ثم يدرب شبكة
عصبية صغيرة لتصنيف `EMPTY` و`LOW` و`HIGH`.

## 1. إنشاء بيانات النوافذ

```powershell
.\.venv\Scripts\python.exe .\ai_module\tensorflow_occupancy\build_window_dataset.py `
  .\data_collection\collected_data\processed\occupancy_training_dataset_v7.csv `
  --output .\ai_module\tensorflow_occupancy\window_features_v2.csv `
  --window-size 30 `
  --step-size 5
```

النافذة تجمع آخر 30 ثانية تقريباً، وتبدأ نافذة جديدة كل 5 ثوانٍ.

## 2. إنشاء بيئة TensorFlow منفصلة

```powershell
py -3.12 -m venv .venv-tf
.\.venv-tf\Scripts\python.exe -m pip install --upgrade pip
.\.venv-tf\Scripts\python.exe -m pip install -r .\ai_module\tensorflow_occupancy\requirements.txt
```

نستخدم بيئة منفصلة حتى لا تتغير مكتبات برنامج تسجيل الداتا.

## 3. تدريب الموديل

```powershell
.\.venv-tf\Scripts\python.exe .\ai_module\tensorflow_occupancy\train_tensorflow.py `
  .\ai_module\tensorflow_occupancy\window_features_v2.csv `
  --artifacts .\ai_module\tensorflow_occupancy\artifacts_pir_v2 `
  --feature-set pir `
  --split-config .\ai_module\tensorflow_occupancy\split_config_v2.csv
```

يمكن لاحقاً تشغيل نسخة ثانية باستخدام `--feature-set all` لمقارنة إضافة
الحرارة والرطوبة والضغط والإضاءة.

## الملفات المهمة بعد التدريب

- `occupancy_model.keras`: موديل TensorFlow الأصلي.
- `occupancy_model_float.tflite`: نسخة TensorFlow Lite العادية.
- `occupancy_model_int8.tflite`: النسخة الصغيرة المناسبة لـSTM32Cube.AI.
- `metrics.json`: دقة الموديل ونتائج كل تصنيف.
- `confusion_matrix_test_int8.csv`: أماكن الخلط بين EMPTY وLOW وHIGH.

## تقسيم الجلسات في التجربة الثانية

يحدد `split_config_v2.csv` الجلسات صراحة حتى لا تختلط نوافذ الجلسة نفسها
بين التدريب والاختبار. دخلت `S19_LOW_ACON_LIGHTON_LOC2` في التدريب، بينما
بقيت `S21_LOW_ACON_LIGHTON_LOC2_R2` اختباراً جديداً لم يره الموديل أثناء
التدريب. جلسات الاختبار الثلاث هي S20 لـEMPTY وS21 لـLOW وS18 لـHIGH.

## نتيجة التجربة الثانية

بعد إضافة جلسة S21 وتنظيف أول وآخر خمس ثوانٍ، أصبحت البيانات 6,479 قراءة
و1,221 نافذة. أفضل نتيجة على جلسات الاختبار الجديدة كانت خصائص PIR فقط:
دقة 88.7% وMacro F1 يساوي 87.5% للموديل العادي ولنسخة INT8. استرجاع LOW
وصل إلى 100% في جلسة S21، بينما كان استرجاع HIGH يساوي 70.1%.

إضافة ToF خفضت الدقة قليلاً إلى 87.9% لأن حساس المسافة لا يرى معظم الغرفة
من موقع الجهاز الحالي. إضافة القيم البيئية المطلقة خفضت التعميم بشدة، لذلك
لا تستخدم الحرارة والرطوبة والضغط والإضاءة كمدخلات مطلقة لموديل الإشغال
الحالي. يمكن إبقاء هذه الحساسات للعرض والتحكم وجمع البيانات.

## نتيجة التجربة الأولى للمقارنة

تمت مقارنة أربع مجموعات خصائص على جلسات كاملة غير مستخدمة في التدريب.
أفضل نتيجة حالياً هي خصائص PIR فقط: دقة 81.5% للموديل العادي و81.1%
لنسخة INT8. ما زالت جلسة LOW الهادئة هي الحالة الأصعب، لذلك نحتاج جلسة LOW
هادئة إضافية قبل اختيار الموديل النهائي. بعد تسجيل S21 وإدخال S19 في
التدريب تحسن اختبار LOW الجديد كما هو موضح أعلاه. المقارنة ليست قياساً
وحيداً لنفس جلسة الاختبار لأن جلسة LOW المحجوزة تغيرت من S19 إلى S21.
