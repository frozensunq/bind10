// Copyright (C) 2013 Internet Systems Consortium, Inc. ("ISC")
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

#ifndef NCR_IO_H
#define NCR_IO_H

/// @file ncr_io.h
/// @brief This file defines abstract classes for exchanging NameChangeRequests.
///
/// These classes are used for sending and receiving requests to update DNS
/// information for FQDNs embodied as NameChangeRequests (aka NCRs). Ultimately,
/// NCRs must move through the following three layers in order to implement
/// DHCP-DDNS:
///
///    * Application layer - the business layer which needs to
///    transport NameChangeRequests, and is unaware of the means by which
///    they are transported.
///
///    * NameChangeRequest layer - This is the layer which acts as the
///    intermediary between the Application layer and the IO layer.  It must
///    be able to move NameChangeRequests to the IO layer as raw data and move
///    raw data from the IO layer in the Application layer as
///    NameChangeRequests.
///
///    * IO layer - the low-level layer that is directly responsible for
///    sending and receiving data asynchronously and is supplied through
///    other libraries.  This layer is largely unaware of the nature of the
///    data being transmitted.  In other words, it doesn't know beans about
///    NCRs.
///
/// The abstract classes defined here implement the latter, middle layer,
/// the NameChangeRequest layer.  There are two types of participants in this
/// middle ground:
///
///    * listeners - Receive NCRs from one or more sources. The DHCP-DDNS
///   application, (aka D2), is a listener. Listeners are embodied by the
///   class, NameChangeListener.
///
///    * senders - sends NCRs to a given target.  DHCP servers are senders.
///   Senders are embodied by the class, NameChangeListener.
///
/// These two classes present a public interface for asynchronous
/// communications that is independent of the IO layer mechanisms.  While the
/// type and details of the IO mechanism are not relevant to either class, it
/// is presumed to use isc::asiolink library for asynchronous event processing.
///

#include <asiolink/io_address.h>
#include <asiolink/io_service.h>
#include <d2/ncr_msg.h>
#include <exceptions/exceptions.h>

#include <deque>

namespace isc {
namespace d2 {

/// @brief Exception thrown if an NcrListenerError encounters a general error.
class NcrListenerError : public isc::Exception {
public:
    NcrListenerError(const char* file, size_t line, const char* what) :
        isc::Exception(file, line, what) { };
};

/// @brief Exception thrown if an error occurs during IO source open.
class NcrListenerOpenError : public isc::Exception {
public:
    NcrListenerOpenError(const char* file, size_t line, const char* what) :
        isc::Exception(file, line, what) { };
};

/// @brief Exception thrown if an error occurs initiating an IO receive.
class NcrListenerReceiveError : public isc::Exception {
public:
    NcrListenerReceiveError(const char* file, size_t line, const char* what) :
        isc::Exception(file, line, what) { };
};


/// @brief Abstract interface for receiving NameChangeRequests.
///
/// NameChangeListener provides the means to:
/// -  Supply a callback to invoke upon receipt of an NCR or a listening
/// error
/// -  Start listening using a given IOService instance to process events
/// -  Stop listening
///
/// It implements the high level logic flow to listen until a request arrives,
/// invoke the implementation's handler and return to listening for the next
/// request.
///
/// It provides virtual methods that allow derivations supply implementations
/// to open the appropriate IO source, perform a listen, and close the IO
/// source.
///
/// The overall design is based on a callback chain. The listener's caller (the
/// application) supplies an "application" layer callback through which it will
/// receive inbound NameChangeRequests.  The listener derivation will supply
/// its own callback to the IO layer to process receive events from its IO
/// source.  This is referred to as the NameChangeRequest completion handler.
/// It is through this handler that the NameChangeRequest layer gains access
/// to the low level IO read service results.  It is expected to assemble
/// NameChangeRequests from the inbound data and forward them to the
/// application layer by invoking the application layer callback registered
/// with the listener.
///
/// The application layer callback is structured around a nested class,
/// RequestReceiveHandler.  It consists of single, abstract operator() which
/// accepts a result code and a pointer to a NameChangeRequest as parameters.
/// In order to receive inbound NCRs, a caller implements a derivation of the
/// RequestReceiveHandler and supplies an instance of this derivation to the
/// NameChangeListener constructor. This "registers" the handler with the
/// listener.
///
/// To begin listening, the caller invokes the listener's startListener()
/// method, passing in an IOService instance.  This in turn will pass the
/// IOService into the virtual method, open().  The open method is where the
/// listener derivation performs the steps necessary to prepare its IO source
/// for reception (e.g. opening a socket, connecting to a database).
///
/// Assuming the open is successful, startListener will call the virtual
/// doReceive() method.  The listener derivation uses this method to
/// instigate an IO layer asynchronous passing in its IO layer callback to
/// handle receive events from its IO source.
///
/// As stated earlier, the derivation's NameChangeRequest completion handler
/// MUST invoke the application layer handler registered with the listener.
/// This is done by passing in either a success status and a populated
/// NameChangeRequest or an error status and an empty request into the
/// listener's invokeRecvHandler method. This is the mechanism by which the
/// listener's caller is handed inbound NCRs.
class NameChangeListener {
public:

    /// @brief Defines the outcome of an asynchronous NCR receive
    enum Result {
      SUCCESS,
      TIME_OUT,
      STOPPED,
      ERROR
    };

    /// @brief Abstract class for defining application layer receive callbacks.
    ///
    /// Applications which will receive NameChangeRequests must provide a
    /// derivation of this class to the listener constructor in order to
    /// receive NameChangeRequests.
    class RequestReceiveHandler {
      public:
        /// @brief Function operator implementing a NCR receive callback.
        ///
        /// This method allows the application to receive the inbound
        /// NameChangeRequests. It is intended to function as a hand off of
        /// information and should probably not be time-consuming.
        ///
        /// @param result contains that receive outcome status.
        /// @param ncr is a pointer to the newly received NameChangeRequest if
        /// result is NameChangeListener::SUCCESS.  It is indeterminate other
        /// wise.
        /// @throw This method MUST NOT throw.
        virtual void operator ()(const Result result,
                                 NameChangeRequestPtr& ncr) = 0;

        virtual ~RequestReceiveHandler() {
        }
    };

    /// @brief Constructor
    ///
    /// @param recv_handler is a pointer the application layer handler to be
    /// invoked each time a NCR is received or a receive error occurs.
    NameChangeListener(RequestReceiveHandler& recv_handler);

    /// @brief Destructor
    virtual ~NameChangeListener() {
    };

    /// @brief Prepares the IO for reception and initiates the first receive.
    ///
    /// Calls the derivation's open implementation to initialize the IO layer
    /// source for receiving inbound requests.  If successful, it issues first
    /// asynchronous read by calling the derivation's doReceive implementation.
    ///
    /// @param io_service is the IOService that will handle IO event processing.
    ///
    /// @throw NcrListenError if the listener is already "listening" or
    /// in the event the open or doReceive methods fail.
    void startListening(isc::asiolink::IOService& io_service);

    /// @brief Closes the IO source and stops listen logic.
    ///
    /// Calls the derivation's implementation of close and marks the state
    /// as not listening.
    void stopListening();

    /// @brief Calls the NCR receive handler registered with the listener.
    ///
    /// This is the hook by which the listener's caller's NCR receive handler
    /// is called.  This method MUST be invoked by the derivation's
    /// implementation of doReceive.
    ///
    /// NOTE:
    /// The handler invoked by this method MUST NOT THROW. The handler is
    /// at application level and should trap and handle any errors at
    /// that level, rather than throw exceptions.  If an error has occurred
    /// prior to invoking the handler, it will be expressed in terms a failed
    /// result being passed to the handler, not a throw.  Therefore any
    /// exceptions at the handler level are application issues and should be
    /// dealt with at that level.
    ///
    /// This method does wrap the handler invocation within a try-catch
    /// block as a fail-safe.  The exception will be logged but the
    /// receive logic will continue.  What this implies is that continued
    /// operation may or may not succeed as the application has violated
    /// the interface contract.
    ///
    /// @param result contains that receive outcome status.
    /// @param ncr is a pointer to the newly received NameChangeRequest if
    /// result is NameChangeListener::SUCCESS.  It is indeterminate other
    /// wise.
    void invokeRecvHandler(const Result result, NameChangeRequestPtr& ncr);

    /// @brief Abstract method which opens the IO source for reception.
    ///
    /// The derivation uses this method to perform the steps needed to
    /// prepare the IO source to receive requests.
    ///
    /// @param io_service is the IOService that process IO events.
    ///
    /// @throw If the implementation encounters an error it MUST
    /// throw it as an isc::Exception or derivative.
    virtual void open(isc::asiolink::IOService& io_service) = 0;

    /// @brief Abstract method which closes the IO source.
    ///
    /// The derivation uses this method to perform the steps needed to
    /// "close" the IO source.
    ///
    /// @throw If the implementation encounters an error it  MUST
    /// throw it as an isc::Exception or derivative.
    virtual void close() = 0;

    /// @brief Initiates an IO layer asynchronous read.
    ///
    /// The derivation uses this method to perform the steps needed to
    /// initiate an asynchronous read of the IO source with the
    /// derivation's IO layer handler as the IO completion callback.
    ///
    /// @throw If the implementation encounters an error it  MUST
    /// throw it as an isc::Exception or derivative.
    virtual void doReceive() = 0;

    /// @brief Returns true if the listener is listening, false otherwise.
    ///
    /// A true value indicates that the IO source has been opened successfully,
    /// and that receive loop logic is active.
    bool amListening() const {
        return (listening_);
    }

private:
    /// @brief Sets the listening indicator to the given value.
    ///
    /// Note, this method is private as it is used the base class is solely
    /// responsible for managing the state.
    ///
    /// @param value is the new value to assign to the indicator.
    void setListening(bool value) {
        listening_ = value;
    }

    /// @brief Indicates if the listener is listening.
    bool listening_;

    /// @brief Application level NCR receive completion handler.
    RequestReceiveHandler& recv_handler_;
};

/// @brief Defines a smart pointer to an instance of a listener.
typedef boost::shared_ptr<NameChangeListener> NameChangeListenerPtr;


/// @brief Thrown when a NameChangeSender encounters an error.
class NcrSenderError : public isc::Exception {
public:
    NcrSenderError(const char* file, size_t line, const char* what) :
        isc::Exception(file, line, what) { };
};

/// @brief Exception thrown if an error occurs during IO source open.
class NcrSenderOpenError : public isc::Exception {
public:
    NcrSenderOpenError(const char* file, size_t line, const char* what) :
        isc::Exception(file, line, what) { };
};

/// @brief Exception thrown if an error occurs initiating an IO send.
class NcrSenderQueueFull : public isc::Exception {
public:
    NcrSenderQueueFull(const char* file, size_t line, const char* what) :
        isc::Exception(file, line, what) { };
};

/// @brief Exception thrown if an error occurs initiating an IO send.
class NcrSenderSendError : public isc::Exception {
public:
    NcrSenderSendError(const char* file, size_t line, const char* what) :
        isc::Exception(file, line, what) { };
};


/// @brief Abstract interface for sending NameChangeRequests.
///
/// NameChangeSender provides the means to:
/// - Supply a callback to invoke upon completing the delivery of an NCR or a
/// send error
/// - Start sending using a given IOService instance to process events
/// - Queue NCRs for delivery
/// - Stop sending
///
/// It implements the high level logic flow to queue requests for delivery,
/// and ship them one at a time, waiting for the send to complete prior to
/// sending the next request in the queue.  If a send fails, the request
/// will remain at the front of queue and will be the send will be retried
/// endlessly unless the caller dequeues the request.  Note, it is presumed that
/// a send failure is some form of IO error such as loss of connectivity and
/// not a message content error.  It should not be possible to queue an invalid
/// message.
///
/// It should be noted that once a request is placed onto the send queue it
/// will remain there until one of three things occur:
///     * It is successfully delivered
///     * @c NameChangeSender::skipNext() is called
///     * @c NameChangeSender::clearSendQueue() is called
///
/// The queue contents are preserved across start and stop listening
/// transitions. This is to provide for error recovery without losing
/// undelivered requests.

/// It provides virtual methods so derivations may supply implementations to
/// open the appropriate IO sink, perform a send, and close the IO sink.
///
/// The overall design is based on a callback chain.  The sender's caller (the
/// application) supplies an "application" layer callback through which it will
/// be given send completion notifications. The sender derivation will employ
/// its own callback at the IO layer to process send events from its IO sink.
/// This callback is expected to forward the outcome of each asynchronous send
/// to the application layer by invoking the application layer callback
/// registered with the sender.
///
/// The application layer callback is structured around a nested class,
/// RequestSendHandler.  It consists of single, abstract operator() which
/// accepts a result code and a pointer to a NameChangeRequest as parameters.
/// In order to receive send completion notifications, a caller implements a
/// derivation of the RequestSendHandler and supplies an instance of this
/// derivation to the NameChangeSender constructor. This "registers" the
/// handler with the sender.
///
/// To begin sending, the caller invokes the listener's startSending()
/// method, passing in an IOService instance.  This in turn will pass the
/// IOService into the virtual method, open().  The open method is where the
/// sender derivation performs the steps necessary to prepare its IO sink for
/// output (e.g. opening a socket, connecting to a database).  At this point,
/// the sender is ready to send messages.
///
/// In order to send a request, the application layer invokes the sender
/// method, sendRequest(), passing in the NameChangeRequest to send.  This
/// method places the request onto the back of the send queue, and then invokes
/// the sender method, sendNext().
///
/// If there is already a send in progress when sendNext() is called, the method
/// will return immediately rather than initiate the next send.  This is to
/// ensure that sends are processed sequentially.
///
/// If there is not a send in progress and the send queue is not empty,
/// the sendNext method will pass the NCR at the front of the send queue into
/// the virtual doSend() method.
///
/// The sender derivation uses this doSend() method to instigate an IO layer
/// asynchronous send with its IO layer callback to handle send events from its
/// IO sink.
///
/// As stated earlier, the derivation's IO layer callback MUST invoke the
/// application layer handler registered with the sender.  This is done by
/// passing in  a status indicating the outcome of the send into the sender's
/// invokeSendHandler method. This is the mechanism by which the sender's
/// caller is handed outbound notifications.

/// After invoking the application layer handler, the invokeSendHandler method
/// will call the sendNext() method to initiate the next send.  This ensures
/// that requests continue to dequeue and ship.
///
class NameChangeSender {
public:

    /// @brief Defines the type used for the request send queue.
    typedef std::deque<NameChangeRequestPtr> SendQueue;

    /// @brief Defines a default maximum number of entries in the send queue.
    static const size_t MAX_QUEUE_DEFAULT = 1024;

    /// @brief Defines the outcome of an asynchronous NCR send.
    enum Result {
        SUCCESS,
        TIME_OUT,
        STOPPED,
        ERROR
    };

    /// @brief Abstract class for defining application layer send callbacks.
    ///
    /// Applications which will send NameChangeRequests must provide a
    /// derivation of this class to the sender constructor in order to
    /// receive send outcome notifications.
    class RequestSendHandler {
      public:
        /// @brief Function operator implementing a NCR send callback.
        ///
        /// This method allows the application to receive the outcome of
        /// each send.  It is intended to function as a hand off of information
        /// and should probably not be time-consuming.
        ///
        /// @param result contains that send outcome status.
        /// @param ncr is a pointer to the NameChangeRequest that was
        /// delivered (or attempted).
        ///
        /// @throw This method MUST NOT throw.
        virtual void operator ()(const Result result,
                                 NameChangeRequestPtr& ncr) = 0;

        virtual ~RequestSendHandler(){
        }
    };

    /// @brief Constructor
    ///
    /// @param send_handler is a pointer the application layer handler to be
    /// invoked each time a NCR send attempt completes.
    /// @param send_queue_max is the maximum number of entries allowed in the
    /// send queue.  Once the maximum number is reached, all calls to
    /// sendRequest will fail with an exception.
    NameChangeSender(RequestSendHandler& send_handler,
            size_t send_queue_max = MAX_QUEUE_DEFAULT);

    /// @brief Destructor
    virtual ~NameChangeSender() {
    }

    /// @brief Prepares the IO for transmission.
    ///
    /// Calls the derivation's open implementation to initialize the IO layer
    /// sink for sending outbound requests.
    ///
    /// @param io_service is the IOService that will handle IO event processing.
    ///
    /// @throw NcrSenderError if the sender is already "sending" or
    /// NcrSenderOpenError if the open fails.
    void startSending(isc::asiolink::IOService & io_service);

    /// @brief Closes the IO sink and stops send logic.
    ///
    /// Calls the derivation's implementation of close and marks the state
    /// as not sending.
    void stopSending();

    /// @brief Queues the given request to be sent.
    ///
    /// The given request is placed at the back of the send queue and then
    /// sendNext is invoked.

    ///
    /// @param ncr is the NameChangeRequest to send.
    ///
    /// @throw NcrSenderError if the sender is not in sending state or
    /// the request is empty; NcrSenderQueueFull if the send queue has reached
    /// capacity.
    void sendRequest(NameChangeRequestPtr& ncr);

    /// @brief Dequeues and sends the next request on the send queue.
    ///
    /// If there is already a send in progress just return. If there is not
    /// a send in progress and the send queue is not empty the grab the next
    /// message on the front of the queue and call doSend().
    ///
    void sendNext();

    /// @brief Calls the NCR send completion handler registered with the
    /// sender.
    ///
    /// This is the hook by which the sender's caller's NCR send completion
    /// handler is called.  This method MUST be invoked by the derivation's
    /// implementation of doSend.   Note that if the send was a success,
    /// the entry at the front of the queue is removed from the queue.
    /// If not we leave it there so we can retry it.  After we invoke the
    /// handler we clear the pending ncr value and queue up the next send.
    ///
    /// NOTE:
    /// The handler invoked by this method MUST NOT THROW. The handler is
    /// application level logic and should trap and handle any errors at
    /// that level, rather than throw exceptions.  If IO errors have occurred
    /// prior to invoking the handler, they are expressed in terms a failed
    /// result being passed to the handler.  Therefore any exceptions at the
    /// handler level are application issues and should be dealt with at that
    /// level.
    ///
    /// This method does wrap the handler invocation within a try-catch
    /// block as a fail-safe.  The exception will be logged but the
    /// send logic will continue.  What this implies is that continued
    /// operation may or may not succeed as the application has violated
    /// the interface contract.
    ///
    /// @param result contains that send outcome status.
    void invokeSendHandler(const NameChangeSender::Result result);

    /// @brief Removes the request at the front of the send queue
    ///
    /// This method can be used to avoid further retries of a failed
    /// send. It is provided primarily as a just-in-case measure. Since
    /// a failed send results in the same request being retried continuously
    /// this method makes it possible to remove that entry, causing the
    /// subsequent entry in the queue to be attempted on the next send.
    /// It is presumed that sends will only fail due to some sort of
    /// communications issue. In the unlikely event that a request is
    /// somehow tainted and causes an send failure based on its content,
    /// this method provides a means to remove th message.
    void skipNext();

    /// @brief Flushes all entries in the send queue
    ///
    /// This method can be used to discard all of the NCRs currently in the
    /// the send queue.  Note it may not be called while the sender is in
    /// the sending state.
    /// @throw NcrSenderError if called and sender is in sending state.
    void clearSendQueue();

    /// @brief Abstract method which opens the IO sink for transmission.
    ///
    /// The derivation uses this method to perform the steps needed to
    /// prepare the IO sink to send requests.
    ///
    /// @param io_service is the IOService that process IO events.
    ///
    /// @throw If the implementation encounters an error it MUST
    /// throw it as an isc::Exception or derivative.
    virtual void open(isc::asiolink::IOService& io_service) = 0;

    /// @brief Abstract method which closes the IO sink.
    ///
    /// The derivation uses this method to perform the steps needed to
    /// "close" the IO sink.
    ///
    /// @throw If the implementation encounters an error it MUST
    /// throw it as an isc::Exception or derivative.
    virtual void close() = 0;

    /// @brief Initiates an IO layer asynchronous send
    ///
    /// The derivation uses this method to perform the steps needed to
    /// initiate an asynchronous send through the IO sink of the given NCR.
    ///
    /// @param ncr is a pointer to the NameChangeRequest to send.
    /// derivation's IO layer handler as the IO completion callback.
    ///
    /// @throw If the implementation encounters an error it MUST
    /// throw it as an isc::Exception or derivative.
    virtual void doSend(NameChangeRequestPtr& ncr) = 0;

    /// @brief Returns true if the sender is in send mode, false otherwise.
    ///
    /// A true value indicates that the IO sink has been opened successfully,
    /// and that send loop logic is active.
    bool amSending() const {
        return (sending_);
    }

    /// @brief Returns true when a send is in progress.
    ///
    /// A true value indicates that a request is actively in the process of
    /// being delivered.
    bool isSendInProgress() const {
        return ((ncr_to_send_) ? true : false);
    }

    /// @brief Returns the maximum number of entries allowed in the send queue.
    size_t getQueueMaxSize() const  {
        return (send_queue_max_);
    }

    /// @brief Returns the number of entries currently in the send queue.
    size_t getQueueSize() const {
        return (send_queue_.size());
    }

private:
    /// @brief Sets the sending indicator to the given value.
    ///
    /// Note, this method is private as it is used the base class is solely
    /// responsible for managing the state.
    ///
    /// @param value is the new value to assign to the indicator.
    void setSending(bool value) {
            sending_ = value;
    }

    /// @brief Boolean indicator which tracks sending status.
    bool sending_;

    /// @brief A pointer to regisetered send completion handler.
    RequestSendHandler& send_handler_;

    /// @brief Maximum number of entries permitted in the send queue.
    size_t send_queue_max_;

    /// @brief Queue of the requests waiting to be sent.
    SendQueue send_queue_;

    /// @brief Pointer to the request which is in the process of being sent.
    NameChangeRequestPtr ncr_to_send_;
};

/// @brief Defines a smart pointer to an instance of a sender.
typedef boost::shared_ptr<NameChangeSender> NameChangeSenderPtr;

} // namespace isc::d2
} // namespace isc

#endif