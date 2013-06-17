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

#ifndef CALLOUT_MANAGER_H
#define CALLOUT_MANAGER_H

#include <exceptions/exceptions.h>
#include <util/hooks/library_handle.h>
#include <util/hooks/server_hooks.h>

#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

namespace isc {
namespace util {

/// @brief No such library
///
/// Thrown if an attempt is made to set the current library index to a value
/// that is invalid for the number of loaded libraries.
class NoSuchLibrary : public Exception {
public:
    NoSuchLibrary(const char* file, size_t line, const char* what) :
        isc::Exception(file, line, what) {}
};


/// @brief Callout Manager
///
/// This class manages the registration, deregistration and execution of the
/// library callouts.
///
/// In operation, the class needs to know two items of data:
///
/// - The list of server hooks, which is used in two ways.  Firstly, when a
///   callout registers or deregisters a hook, it does so by name: the
///   @ref isc::util::ServerHooks object supplies the names of registered
///   hooks.  Secondly, when the callouts associated with a hook are called by
///   the server, the server supplies the index of the relevant hook: this is
///   validated by reference to the list of hook.
///
/// - The number of loaded libraries.  Each callout registered by a user
///   library is associated with that library, the callout manager storing both
///   a pointer to the callout and the index of the library in the list of
///   loaded libraries.  Callouts are allowed to dynamically register and
///   deregister callouts in the same library (including themselves): they
///   cannot affect callouts registered by another library.  When calling a
///   callout, the callout manager maintains the idea of a "current library
///   index": if the callout calls one of the callout registration functions 
///   (indirectly via the @ref LibraryHandle object), the registration
///   functions use the "current library index" in their processing.
///
/// These two items of data are supplied when an object of this class is
/// constructed.
///
/// Note that the callout function do not access the library manager: instead,
/// they use a LibraryHandle object.  This contains an internal pointer to
/// the CalloutManager, but provides a restricted interface.  In that way,
/// callouts are unable to affect callouts supplied by other libraries.

class CalloutManager {
private:

    // Private typedefs

    /// Element in the vector of callouts.  The elements in the pair are the
    /// index of the library from which this callout was registered, and a#
    /// pointer to the callout itself.
    typedef std::pair<int, CalloutPtr> CalloutEntry;

    /// An element in the hook vector. Each element is a vector of callouts
    /// associated with a given hook.
    typedef std::vector<CalloutEntry> CalloutVector;

public:

    /// @brief Constructor
    ///
    /// Initializes member variables, in particular sizing the hook vector
    /// (the vector of callout vectors) to the appropriate size.
    ///
    /// @param hooks Collection of known hook names.
    /// @param num_libraries Number of loaded libraries.
    ///
    /// @throw isc::BadValue if the number of libraries is less than or equal
    ///        to 0, or if the pointer to the server hooks object is empty.
    CalloutManager(const boost::shared_ptr<ServerHooks>& hooks,
                   int num_libraries)
        : current_library_(-1), hooks_(hooks), hook_vector_(),
          library_handle_(this), num_libraries_(num_libraries)
    {
        if (!hooks) {
            isc_throw(isc::BadValue, "must pass a pointer to a valid server "
                      "hooks object to the CalloutManager");
        } else if (num_libraries <= 0) {
            isc_throw(isc::BadValue, "number of libraries passed to the "
                      "CalloutManager must be >= 0");
        }

        // Parameters OK, do operations that depend on them.
        hook_vector_.resize(hooks_->getCount());
    }

    /// @brief Register a callout on a hook for the current library
    ///
    /// Registers a callout function for the current library with a given hook
    /// (the index of the "current library" being given by the current_library_
    /// member).  The callout is added to the end of the callouts for this
    /// library that are associated with that hook.
    ///
    /// @param name Name of the hook to which the callout is added.
    /// @param callout Pointer to the callout function to be registered.
    ///
    /// @throw NoSuchHook The hook name is unrecognised.
    /// @throw Unexpected The hook name is valid but an internal data structure
    ///        is of the wrong size.
    void registerCallout(const std::string& name, CalloutPtr callout);

    /// @brief De-Register a callout on a hook for the current library
    ///
    /// Searches through the functions registered by the the current library
    /// (the index of the "current library" being given by the current_library_
    /// member) with the named hook and removes all entries matching the
    /// callout.
    ///
    /// @param name Name of the hook from which the callout is removed.
    /// @param callout Pointer to the callout function to be removed.
    ///
    /// @return true if a one or more callouts were deregistered.
    ///
    /// @throw NoSuchHook The hook name is unrecognised.
    /// @throw Unexpected The hook name is valid but an internal data structure
    ///        is of the wrong size.
    bool deregisterCallout(const std::string& name, CalloutPtr callout);

    /// @brief Removes all callouts on a hook for the current library
    ///
    /// Removes all callouts associated with a given hook that were registered
    /// by the current library (the index of the "current library" being given
    /// by the current_library_ member).
    ///
    /// @param name Name of the hook from which the callouts are removed.
    ///
    /// @return true if one or more callouts were deregistered.
    ///
    /// @throw NoSuchHook Thrown if the hook name is unrecognised.
    bool deregisterAllCallouts(const std::string& name);

    /// @brief Checks if callouts are present on a hook
    ///
    /// Checks all loaded libraries and returns true if at least one callout
    /// has been registered by any of them for the given hook.
    ///
    /// @param hook_index Hook index for which callouts are checked.
    ///
    /// @return true if callouts are present, false if not.
    ///
    /// @throw NoSuchHook Given index does not correspond to a valid hook.
    bool calloutsPresent(int hook_index) const;

    /// @brief Calls the callouts for a given hook
    ///
    /// Iterates through the libray handles and calls the callouts associated
    /// with the given hook index.
    ///
    /// @note This method invalidates the current library index set with
    ///       setLibraryIndex().
    ///
    /// @param hook_index Index of the hook to call.
    /// @param callout_handle Reference to the CalloutHandle object for the
    ///        current object being processed.
    void callCallouts(int hook_index, CalloutHandle& callout_handle);

    /// @brief Get number of libraries
    ///
    /// Returns the number of libraries that this CalloutManager is expected
    /// to serve.  This is the number passed to its constructor.
    ///
    /// @return Number of libraries server by this CalloutManager.
    int getNumLibraries() const {
        return (num_libraries_);
    }

    /// @brief Get current library index
    ///
    /// Returns the index of the "current" library.  This the index associated
    /// with the currently executing callout when callCallouts is executing.
    /// When callCallouts() is not executing (as is the case when the load()
    /// function in a user-library is called during the library load process),
    /// the index can be set by setLibraryIndex().
    ///
    /// @note The value set by this method is lost after a call to
    ///       callCallouts.
    ///
    /// @return Current library index.
    int getLibraryIndex() const {
        return (current_library_);
    }

    /// @brief Set current library index
    ///
    /// Sets the current library index.  This must be in the range 0 to
    /// (numlib - 1), where "numlib" is the number of libraries loaded in the
    /// system, this figure being passed to this object at construction time.
    ///
    /// @param library_index New library index.
    ///
    /// @throw NoSuchLibrary if the index is not valid.
    void setLibraryIndex(int library_index) {
        checkLibraryIndex(library_index);
        current_library_ = library_index;
    }

    /// @brief Return library handle
    ///
    /// The library handle is available to the user callout via the callout
    /// handle object.  It provides a cut-down view of the CalloutManager,
    /// allowing the callout to register and deregister callouts in the
    /// library of which it is part, whilst denying access to anything that
    /// may affect other libraries.
    ///
    /// @return Reference to callout handle for this manager
    LibraryHandle& getLibraryHandle() {
        return (library_handle_);
    }

private:
    /// @brief Check library index
    ///
    /// Ensures that the current library index is valid.  This is called by
    /// the hook registration functions.
    ///
    /// @param library_index Value to check for validity as a library index.
    ///
    /// @throw NoSuchLibrary Library index is not valid.
    void checkLibraryIndex(int library_index) const {
        if ((library_index < 0) || (library_index >= num_libraries_)) {
            isc_throw(NoSuchLibrary, "library index " << library_index <<
                      " is not valid for the number of loaded libraries (" <<
                      num_libraries_ << ")");
        }
    }

    /// @brief Compare two callout entries for library equality
    ///
    /// This is used in callout removal code when all callouts on a hook for a
    /// given library are being removed.  It checks whether two callout entries
    /// have the same library index.
    ///
    /// @param ent1 First callout entry to check
    /// @param ent2 Second callout entry to check
    ///
    /// @return bool true if the library entries are the same
    class CalloutLibraryEqual :
        public std::binary_function<CalloutEntry, CalloutEntry, bool> {
    public:
        bool operator()(const CalloutEntry& ent1,
                        const CalloutEntry& ent2) const {
            return (ent1.first == ent2.first);
        }
    };

    /// Current library index.  When a call is made to any of the callout
    /// registration methods, this variable indicates the index of the user
    /// library that should be associated with the call.
    int current_library_;

    /// List of server hooks.
    boost::shared_ptr<ServerHooks>  hooks_;

    /// Vector of callout vectors.  There is one entry in this outer vector for
    /// each hook. Each element is itself a vector, with one entry for each
    /// callout registered for that hook.
    std::vector<CalloutVector>  hook_vector_;

    /// LibraryHandle object user by the callout to access the callout
    /// registration methods on this CalloutManager object.
    LibraryHandle library_handle_;

    /// Number of libraries.
    int num_libraries_;

};

} // namespace util
} // namespace isc

#endif // CALLOUT_MANAGER_H