# AI Module

The active machine-learning pipeline is located in `tensorflow_occupancy/`. It creates time-window features and trains a three-class `EMPTY` / `LOW` / `HIGH` occupancy model.

The selected final model uses six features derived from PIR1 and PIR2. Environmental sensors are retained for monitoring and rule-based context rather than as direct inputs to the final occupancy classifier.
