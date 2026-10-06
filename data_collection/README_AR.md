# نظام جمع داتا المشروع

الملفات هنا تجهز السلسلة كاملة من قراءة الحساسات إلى ملف صالح لتدريب نموذج
الـAI. ابدأ بتجربة قصيرة عبر USB، وبعد نجاحها فعّل HC-06 للجلسات الفعلية.

## 1. مكونات الكود ووظائفها

- `STM32_DATA_COLLECTION_MAIN.c.txt`: كود STM32 لجميع الحساسات والشاشة
  والزرارين والـLEDs والـactive buzzer.
- `serial_data_logger.py`: يستقبل الداتا من منفذ COM ويحفظها فورًا في CSV.
- `prepare_training_data.py`: ينظف الملفات الخام ويخرج ملف التدريب النهائي.
- `planning/data_collection_plan.xlsx`: خطة الجلسات وتعريف كل عمود وأوامر التشغيل.

كل صف يجمع القراءات في لحظة واحدة كل ثانية. الداتا الخام لا تُعدل، وملف
التدريب النظيف يُنشأ في خطوة منفصلة.

برنامج الحفظ يربط أول وقت يصل من STM32 بساعة اللابتوب، ثم يستخدم
`mcu_time_ms` لبقية الصفوف. بهذه الطريقة لا تُفسد تأخيرات البلوتوث ترتيب
العينات أو تجعل عدة صفوف تحمل نفس الوقت.

## 2. توصيل الحساسات

وصّل OLED وBME280 وVL53L0X وBH1750 على نفس I2C1:

| الإشارة | NUCLEO-L476RG | الأجهزة |
|---|---|---|
| SCL | PB8 / D15 | SCL لجميع أجهزة I2C |
| SDA | PB9 / D14 | SDA لجميع أجهزة I2C |
| GND | GND مشترك | GND لجميع المكونات |
| BH1750 ADDR | GND | يجعل العنوان `0x23` |

استخدم التغذية التي أثبتت نجاح كل module في اختباره المنفصل. خطوط I2C نفسها
يجب أن تبقى 3.3V. نتيجة الـscanner الطبيعية هي OLED `0x3C`، VL53L0X
`0x29`، BH1750 `0x23`، وBME280 `0x76` أو `0x77`.

أسماء الأرجل المطلوبة في CubeMX هي نفس الأسماء الموجودة في اختباراتك:
`PIR_1`, `PIR_2`, `LED_Red`, `LED_Green`, `LD2`, `Push_button_1`,
`Push_button_2`, و`Buzzer`. الزراران input pull-up، ولذلك كل زر يُوصل بين
الـpin وGND. الـPIR inputs بدون pull، والـLEDs والـbuzzer outputs.
موديول الـactive buzzer المستخدم active-low: ‏`PB4 LOW` يصدر الصوت
و`PB4 HIGH` يجعله ساكتًا، ولذلك الكود يضعه HIGH في وضع الخمول.

## 3. أول اختبار عبر USB

1. انسخ محتوى `STM32_DATA_COLLECTION_MAIN.c.txt` إلى `Core/Src/main.c`.
2. اترك `DATA_LINK_USE_HC06` بقيمة `0`.
3. تأكد أن USART2 مضبوط على `115200, 8-N-1`.
4. في إعدادات linker فعّل `-u _printf_float` لأن صف CSV يحتوي قيم BME280
   العشرية.
5. اعمل Build وFlash.
6. ثبّت المكتبات على اللابتوب:

```powershell
python -m pip install -r data_collection\requirements.txt
```

7. شغّل برنامج الحفظ، مع استبدال COM8 بالمنفذ الصحيح:

```powershell
python data_collection\serial_data_logger.py --port COM8 --baud 115200
```

بعد تجهيز البيئة أول مرة، يمكن تشغيل `START_DATA_LOGGER.bat` بنقرتين بدل
كتابة الأمر. الملف مضبوط على `COM8` و`115200`. أوقف جلسة STM32 بالزر الثاني
ثم اكتب `Q` لإغلاق برنامج اللابتوب بأمان.

من نافذة البرنامج اكتب `E` أو `L` أو `H` لتحديد الحقيقة الأرضية، ثم `S`
لبدء الجلسة و`X` لإيقافها. يمكن استعمال الزرارين بدل لوحة المفاتيح.

## 4. وظيفة الأزرار والإشارات

- Button 1: يغير `EMPTY -> LOW -> HIGH` قبل بدء التسجيل.
- Button 2: يبدأ أو يوقف الجلسة.
- الأخضر: EMPTY.
- الأخضر والأحمر معًا: LOW، أي شخص واحد في الداتا الحالية.
- الأحمر: HIGH، أي شخصان في الداتا الحالية.
- LD2: مضاء أثناء التسجيل ويومض عند إرسال كل صف.
- Buzzer: صفارة واحدة للتأكيد، صفارتان عند الإيقاف، وثلاث صفارات للخطأ.

الكود يمنع تغيير الـlabel أثناء الجلسة. أوقف التسجيل أولًا إذا تغير عدد
الأشخاص.

## 5. تشغيل HC-06 بدون حمل اللابتوب

اختبر USB أولًا. بعد ذلك في CubeMX فعّل UART إضافي، مثل USART1، بوضع
Asynchronous واضبطه على `9600, 8-N-1`. استعمل الأرجل الحرة التي يعرضها
CubeMX، ثم وصّل:

| HC-06 | STM32 |
|---|---|
| TXD | RX للـUART المختار |
| RXD | TX للـUART المختار |
| GND | GND مشترك |
| VCC | حسب لوحة HC-06 نفسها؛ كثير من breakout boards تقبل 5V |

بعد توليد كود USART1، غيّر `DATA_LINK_USE_HC06` إلى `1`. شغّل STM32 من
power bank، اعمل pairing للـHC-06، واعرف رقم Bluetooth COM في Windows، ثم:

```powershell
python data_collection\serial_data_logger.py --port COM_NUMBER --baud 9600
```

ضع اللابتوب في مكان آمن داخل مدى البلوتوث؛ لا تحتاج أن تمسكه أثناء الجلسة.

## 6. بروتوكول جمع الداتا

ثبّت الجهاز في موضعه النهائي واتجاهه النهائي قبل الجلسات، ولا تحركه بين
الجلسات العادية. غيّر الإضاءة ومواضع الأشخاص وحركتهم، لكن نفذ هذه التغييرات
بصورة متوازنة في EMPTY وLOW وHIGH. إذا أردت تجربة موضع جهاز مختلف، اعتبره
إعدادًا جديدًا وكرر الفئات الثلاث تحته.

ابدأ بـpilot مدته دقيقة واحدة لكل فئة. بعد نجاح الملفات والتنظيف، نفذ 15
جلسة: خمس EMPTY، خمس LOW، وخمس HIGH. مدة البداية المقترحة 10 دقائق للجلسة
وبمعدل 1 Hz، أي نحو 9000 صف إجمالًا.

في EMPTY جرّب الضوء مطفأ وعادي وضوء نهار، لكن الغرفة تبقى بلا أشخاص. لا
تغيّر مكان الجهاز لمجرد بدء جلسة EMPTY جديدة.

## 7. تنظيف الداتا والتحقق منها

مرر كل ملفات raw معًا:

```powershell
python data_collection\prepare_training_data.py data_collection\collected_data\raw\*.csv
```

المنظف يحافظ على الملفات الخام، يرفض قراءات الحساسات غير الصالحة، ويعامل
`VL53L0X RangeStatus != 0` و`8190/8191 mm` كعدم وجود هدف صالح. يحتفظ بالصف
ويحوّل المسافة إلى 2000 mm مع عمود `tof_valid=0` حتى يستطيع STM32 تطبيق
نفس المعالجة أثناء inference. كما يحذف أول وآخر 5 ثوانٍ من كل جلسة، ويبدأ
`segment_id` جديدًا بعد أي فجوة زمنية كبيرة حتى لا تعبرها ميزات النافذة.
راجع دائمًا ملف `training_dataset.report.json` وملف الصفوف المرفوضة.

القيمة الافتراضية لا تنعم القراءات السليمة. لتجربة median filter سببي من
ثلاث عينات استخدم `--median-window 3`. لا تعتمد هذا الخيار في النموذج النهائي
إلا إذا طبقت نفس الفلتر داخل STM32 وقت inference؛ اختلاف preprocessing بين
التدريب والجهاز يعطي نتيجة مضللة.

بعد تجهيز `occupancy_training_dataset_v7.csv`، ابنِ نوافذ TensorFlow:

```powershell
.\.venv-tf\Scripts\python.exe .\ai_module\tensorflow_occupancy\build_window_dataset.py `
  .\data_collection\collected_data\processed\occupancy_training_dataset_v7.csv `
  --output .\ai_module\tensorflow_occupancy\window_features_v2.csv `
  --window-size 30 --step-size 5
```

ثم أعد تدريب النسخة المعتمدة:

```powershell
.\.venv-tf\Scripts\python.exe .\ai_module\tensorflow_occupancy\train_tensorflow.py `
  .\ai_module\tensorflow_occupancy\window_features_v2.csv `
  --artifacts .\ai_module\tensorflow_occupancy\artifacts_pir_v2 `
  --feature-set pir `
  --split-config .\ai_module\tensorflow_occupancy\split_config_v2.csv
```

مهم: لأن مشروع CubeIDE الفعلي غير موجود داخل هذا المجلد، لا يمكن إجراء Build
حقيقي للكود هنا. إذا ظهرت أخطاء بعد النسخ، أرسل `main.h` و`usart.c` ونافذة
الأخطاء، وسنطابق أسماء الأرجل والـUART مع مشروعك مباشرة.
