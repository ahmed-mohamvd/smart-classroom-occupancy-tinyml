# ملفات الشبكة المولدة بواسطة X-CUBE-AI

- `occupancy.c/.h`: واجهة تشغيل الشبكة المولدة.
- `occupancy_data.c/.h`: أوزان وبيانات النموذج.
- `occupancy_data_params.c/.h`: أحجام وترتيب buffers.
- `occupancy_config.h`: إعدادات الشبكة.
- `occupancy_c_info.json`: وصف الشبكة والأحجام.
- `occupancy_generate_report.txt`: تقرير عملية التوليد.
- `LICENSE.txt`: ترخيص runtime المولد.

هذه الملفات مرتبطة بالموديل `artifacts_pir_v2/occupancy_model_int8.tflite`.
إذا أعدت تدريب النموذج، ولّد هذه الملفات من جديد ولا تخلط بين نسختين.
