/**
 * Copyright (C) 2007-2016 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or any later version.
 *
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hypertable. If not, see <http://www.gnu.org/licenses/>
 */
#include "Common/Compat.h"

#include <ThriftBroker/SerializedCellsReader.h>
#include <ThriftBroker/SerializedCellsWriter.h>

#include <Python.h>
#include <sys/resource.h>

using namespace Hypertable;



static PyObject *
scw_init22(PyObject *self, PyObject *args)
{
	int sz = 0;
	int grow = 0;
	if (!PyArg_ParseTuple(args, "ip", &sz, &grow))
		return NULL;
	auto scw = std::unique_ptr<SerializedCellsWriter>(new SerializedCellsWriter(sz, grow));
	return PyLong_FromVoidPtr((void *)scw.get());
}
static PyObject *
scw_add22(PyObject *self, PyObject *args)
{
	PyObject *scw_ptr;
	int grow = 0;
	if (!PyArg_ParseTuple(args, "Oi", &scw_ptr, &grow))
		return NULL;
	
	std::unique_ptr<SerializedCellsWriter> scw(static_cast<SerializedCellsWriter*>(PyLong_AsVoidPtr(scw_ptr)));

	return Py_None;
}

static PyObject *
scr_init(PyObject *self, PyObject *args)
{
	uint32_t len = 0;
	std::string b;
	if (!PyArg_ParseTuple(args, "sk", &b, &len))
		return NULL;
	auto scr = std::unique_ptr<SerializedCellsReader>(new SerializedCellsReader(b, len));
	return PyLong_FromVoidPtr((void *)scr.get());
}



static PyObject *scw_add (PyObject *dummy, PyObject *args)
{
    PyObject *pymodel = NULL;
	const char* s;
    if (!PyArg_ParseTuple(args, "Os", &pymodel, &s))
        return NULL;

    std::unique_ptr<SerializedCellsWriter> scw(static_cast<SerializedCellsWriter*>(PyCapsule_GetPointer(pymodel, "SerializedCellsWriter")));
    printf("scw Pointer %x \n", scw.get());
	
    return Py_BuildValue("");
}

static PyObject* scw_init (PyObject *dummy, PyObject *args)
{
	int sz = 0;
	int grow = 0;
	if (!PyArg_ParseTuple(args, "ip", &sz, &grow))
		return NULL;

	auto scw = std::unique_ptr<SerializedCellsWriter>(new SerializedCellsWriter(sz, grow));
    printf("scw Pointer %x \n", scw.get());

    PyObject * ret = PyCapsule_New((void *)scw.get(), "SerializedCellsWriter", NULL);
    return ret;
}
// https://stackoverflow.com/questions/40166645/a-c-pointer-passed-between-c-and-python-using-capsule-is-empty











static PyMethodDef serialized_cells_Methods[] =
{
	{"SerializedCellsWriter", scw_init, METH_VARARGS, "SerializedCellsWriter(sz, grow(bool)) "},
	{"scw_add", scw_add, METH_VARARGS, "scw_add(sz, grow(bool)) "},
	{"SerializedCellsReader", scr_init, METH_VARARGS," SerializedCellsReader(buff, len) "},
	{NULL, NULL, 0, NULL}
};









struct module_state {
	PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif


#if PY_MAJOR_VERSION >= 3

static int serialized_cells_traverse(PyObject *m, visitproc visit, void *arg) {
	Py_VISIT(GETSTATE(m)->error);
	return 0;
}
static int serialized_cells_clear(PyObject *m) {
	Py_CLEAR(GETSTATE(m)->error);
	return 0;
}

static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"serialized_cells",
	NULL,
	sizeof(struct module_state),
	serialized_cells_Methods,
	NULL,
	serialized_cells_traverse,
	serialized_cells_clear,
	NULL
};

PyMODINIT_FUNC
PyInit_serialized_cells(void)

#else

void
initserialized_cells(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
	PyObject *module = PyModule_Create(&moduledef);
#else
	PyObject *module = Py_InitModule("serialized_cells", serialized_cells_Methods);
#endif

	/* no core dump file */
	struct rlimit rlp;
	rlp.rlim_cur = 0;
	setrlimit(RLIMIT_CORE, &rlp);

#if PY_MAJOR_VERSION >= 3
	return module;
#endif
}

