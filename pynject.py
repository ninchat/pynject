import ctypes
from collections import defaultdict

_libname = "libpynject.so"

_pylib = ctypes.PyDLL(_libname)
_pylib.pynject_init.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_pylib.pynject_init.restype = ctypes.c_int

_clib = ctypes.CDLL(_libname)
_clib.pynject_set.argtypes = [ctypes.c_int]
_clib.pynject_set.restype = ctypes.c_int

_data = None

def init():
	if _pylib.pynject_init(b"pynject", b"_callback") != 0:
		raise Exception("pynject init failed")

def _set(granularity):
	assert granularity < 1

	if _clib.pynject_set(int(granularity * 1000000000)) != 0:
		raise Exception("pynject set failed")

def start(granularity):
	global _data

	assert granularity > 0

	if _data is None:
		_data = defaultdict(int)
		_set(granularity)
		return True
	else:
		return False

def stop():
	global _data

	if _data is not None:
		_set(0)
		result = _data
		_data = None
		return result
	else:
		return None

def _callback(frame):
	global _data

	if _data is None:
		return

	stack = []

	while frame is not None:
		stack.append("{}({}:{})".format(frame.f_code.co_name, frame.f_globals.get("__name__"), frame.f_code.co_firstlineno))
		frame = frame.f_back

	stack.reverse()

	_data[";".join(stack)] += 1
