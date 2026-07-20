import numpy as np
import lumen_core

a = np.array([1.0, 2.0, 3.0], dtype=np.float32)
b = np.array([4.0, 5.0, 6.0], dtype=np.float32)
print(f"L2 distance: {lumen_core.l2_distance(a, b)}")

vec = [1.0, 2.0, 3.0]
scaled = lumen_core.scale_vector(vec, 2.0)
print(f"Scaled: {scaled}")

storage = lumen_core.VectorStorage(dimension=3, capacity=100)
v1 = np.array([1.0, 0.0, 0.0], dtype=np.float32)
v2 = np.array([0.0, 1.0, 0.0], dtype=np.float32)
id1 = storage.append(v1)
id2 = storage.append(v2)
print(f"Inserted vectors: {id1}, {id2}")
print(f"Count: {storage.count()}")
print(f"Vector 1: {storage.get_vector(id1)}")

storage.save("demo_vectors.bin")
loaded = lumen_core.VectorStorage.load("demo_vectors.bin")
print(f"Loaded count: {loaded.count()}")
