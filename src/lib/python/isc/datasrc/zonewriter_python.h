// Copyright (C) 2013  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifndef PYTHON_ZONEWRITER_H
#define PYTHON_ZONEWRITER_H 1

#include <Python.h>
#include <datasrc/client_list.h>

namespace isc {
namespace datasrc {
namespace memory {
namespace python {

extern PyTypeObject zonewriter_type;

bool initModulePart_ZoneWriter(PyObject* mod);

/// \brief Create a ZoneWriter python object
///
/// \param source The zone writer pointer to wrap
/// \param base_obj An optional PyObject that this ZoneWriter depends on
///                 Its refcount is increased, and will be decreased when
///                 this zone iterator is destroyed, making sure that the
///                 base object is never destroyed before this ZoneWriter.
PyObject* createZoneWriterObject(
    isc::datasrc::ConfigurableClientList::ZoneWriterPtr source,
    PyObject* base_obj = NULL);

} // namespace python
} // namespace memory
} // namespace datasrc
} // namespace isc

#endif // PYTHON_ZONEWRITER_H

// Local Variables:
// mode: c++
// End:
