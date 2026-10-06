# Notebook شرح وتدريب الموديل

الملف `training_walkthrough.ipynb` يحتوي على تدريب الموديل خطوة بخطوة من
بيانات المشروع الحقيقية. تم تشغيل جميع الخلايا مسبقاً وحفظ النتائج داخل
الملف، لذلك يمكن فتحه وتصوير النتائج مباشرة.

## طريقة الفتح

شغّل الملف:

`OPEN_TRAINING_NOTEBOOK.bat`

سيفتح JupyterLab في المتصفح. افتح `training_walkthrough.ipynb` إذا لم يفتح
تلقائياً. لإعادة التدريب اختر من القائمة `Run` ثم `Run All Cells`.

التشغيل يكتب داخل `notebook_run` فقط ولا يغيّر الموديل النهائي الموجود في
`artifacts_pir_v2`.

## أفضل صور للعرض

1. جدول توزيع Train وValidation وTest.
2. `model.summary()` الذي يعرض الشبكة `6 → 16 → 8 → 3` وعدد 275 parameter.
3. آخر Epochs من خلية التدريب.
4. منحنيات Accuracy وLoss.
5. Confusion Matrix ودقة 88.67%.
6. الخلية النهائية التي تعرض حجم موديل INT8 وهو 3,616 bytes.

نسخ PNG الجاهزة موجودة أيضاً في مجلد `notebook_run`.
