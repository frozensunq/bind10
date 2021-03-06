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

#ifndef HOOKS_H
#define HOOKS_H

#include <config.h>
#include <hooks/callout_handle.h>
#include <hooks/library_handle.h>

namespace {

// Version 1 of the hooks framework.
const int BIND10_HOOKS_VERSION = 1;

// Names of the framework functions.
const char* const LOAD_FUNCTION_NAME = "load";
const char* const UNLOAD_FUNCTION_NAME = "unload";
const char* const VERSION_FUNCTION_NAME = "version";

// Typedefs for pointers to the framework functions.
typedef int (*version_function_ptr)();
typedef int (*load_function_ptr)(isc::hooks::LibraryHandle&);
typedef int (*unload_function_ptr)();

} // Anonymous namespace

namespace isc {
namespace hooks {

/// @brief User-Library Initialization for Statically-Linked BIND 10
///
/// If BIND 10 is statically-linked, a user-created hooks library will not be
/// able to access symbols in it.  In particular, it will not be able to access
/// singleton objects.
///
/// The hooks framework handles some of this.  For example, although there is
/// a singleton ServerHooks object, hooks framework objects store a reference
/// to it when they are created.  When the user library needs to register a
/// callout (which requires access to the ServerHooks information), it accesses
/// the ServerHooks object through a pointer passed from the BIND 10 image.
///
/// The logging framework is more problematical. Here the code is partly
/// statically linked (the BIND 10 logging library) and partly shared (the
/// log4cplus).  The state of the former is not accessible to the user library,
/// but the state of the latter is.  So within the user library, we need to
/// initialize the BIND 10 logging library but not initialize the log4cplus
/// code.  Some of the initialization is done when the library is loaded, but
/// other parts are done at run-time.
///
/// This function - to be called by the user library code in its load() function
/// when running against a statically linked BIND 10 - initializes the BIND 10
/// logging library.  In particular, it loads the message dictionary with the
/// text of the BIND 10 messages.
///
/// @note This means that the virtual address space is loaded with two copies
/// of the message dictionary.  Depending on how the user libraries are linked,
/// loading multiple user libraries may involve loading one message dictionary
/// per library.

void hooksStaticLinkInit();

} // namespace hooks
} // namespace isc

#endif  // HOOKS_H
