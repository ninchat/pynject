#include <Python.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/timerfd.h>

#include <pthread.h>

static int pynject_fd = -1;

static int pynject_call(void *ptr)
{
	PyObject *callable = ptr;
	PyObject *frame;
	PyObject *args;
	PyObject *result;

	frame = (PyObject *) PyEval_GetFrame();
	if (frame == NULL)
		return 0;

	args = PyTuple_New(1);
	if (args == NULL) {
		PyErr_Print();
		PyErr_Clear();
		return 0;
	}

	Py_INCREF(frame);
	PyTuple_SET_ITEM(args, 0, frame);

	result = PyObject_CallObject(callable, args);
	if (result) {
		Py_XDECREF(result);
	} else {
		PyErr_Print();
		PyErr_Clear();
	}

	Py_XDECREF(args);
	return 0;
}

static void *pynject_loop(void *callable)
{
	while (1) {
		uint64_t count;

		if (read(pynject_fd, &count, sizeof (count)) < 0) {
			if (errno == EINTR)
				continue;

			fprintf(stderr, "pynject: timer fd read failed: %s\n", strerror(errno));
			return NULL; // leak pynject_fd
		}

		if (count > 0)
			Py_AddPendingCall(pynject_call, callable);
	}
}

int pynject_init(const char *module_name, const char *callable_name)
{
	PyObject *module;
	PyObject *callable;
	int fd;
	pthread_attr_t attr;
	pthread_t thread;

	if (pynject_fd >= 0)
		goto already_inited;

	module = PyImport_ImportModule(module_name);
	if (module == NULL)
		goto no_module;

	callable = PyObject_GetAttrString(module, callable_name);
	if (callable == NULL)
		goto no_callable;

	if (!PyCallable_Check(callable))
		goto not_a_callable;

	fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
	if (fd < 0)
		goto no_fd;

	if (pthread_attr_init(&attr) != 0)
		goto no_attr;

	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
		goto no_detach;

	if (pthread_create(&thread, &attr, pynject_loop, callable) != 0)
		goto no_thread;

	pynject_fd = fd;

	pthread_attr_destroy(&attr);
	Py_XDECREF(module);
	return 0;

no_thread:
no_detach:
	pthread_attr_destroy(&attr);
no_attr:
	close(fd);
no_fd:
not_a_callable:
	Py_XDECREF(callable);
no_callable:
	Py_XDECREF(module);
no_module:
already_inited:
	return -1;
}

int pynject_set(int granularity)
{
	struct itimerspec spec = {
		.it_interval = {
			.tv_nsec = granularity,
		},
		.it_value = {
			.tv_nsec = granularity,
		},
	};

	return timerfd_settime(pynject_fd, 0, &spec, NULL);
}
