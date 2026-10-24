"""Bounded native batch concurrency; preserves input ordering and scalar physics."""
from concurrent.futures import ThreadPoolExecutor
import os
import numpy as np

available=len(os.sched_getaffinity(0)) if hasattr(os,"sched_getaffinity") else (os.cpu_count() or 1)
workers=int(os.environ.get("ATLAS_CORRECTIONLIB_THREADS",str(min(4,available))))
if workers < 1:
    raise ValueError("ATLAS_CORRECTIONLIB_THREADS must be positive")
_pool=ThreadPoolExecutor(max_workers=workers,thread_name_prefix="corrections") if workers>1 else None

def batch(evaluate, rows):
    if _pool is None or len(rows)<4096:
        return evaluate(rows)
    pieces=np.array_split(rows,workers,axis=0)
    return np.concatenate(list(_pool.map(evaluate,pieces)),axis=0)
