import tensorflow as tf
print("TensorFlow version:", tf.__version__)
print("GPUs available:", tf.config.list_physical_devices('GPU'))
print("CUDA enabled:", tf.test.is_built_with_cuda())
print("GPU name:", tf.test.gpu_device_name())