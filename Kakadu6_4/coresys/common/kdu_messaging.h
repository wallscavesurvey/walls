/*****************************************************************************/
// File: kdu_messaging.h [scope = CORESYS/COMMON]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/*****************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/*****************************************************************************/
// Licensee: Mr David McKenzie
// License number: 00590
// The licensee has been granted a NON-COMMERCIAL license to the contents of
// this source file.  A brief summary of this license appears below.  This
// summary is not to be relied upon in preference to the full text of the
// license agreement, accepted at purchase of the license.
// 1. The Licensee has the right to install and use the Kakadu software and
//    to develop Applications for the Licensee's own use.
// 2. The Licensee has the right to Deploy Applications built using the
//    Kakadu software to Third Parties, so long as such Deployment does not
//    result in any direct or indirect financial return to the Licensee or
//    any other Third Party, which further supplies or otherwise uses such
//    Applications.
// 3. The Licensee has the right to distribute Reusable Code (including
//    source code and dynamically or statically linked libraries) to a Third
//    Party, provided the Third Party possesses a license to use the Kakadu
//    software, and provided such distribution does not result in any direct
//    or indirect financial return to the Licensee.
/******************************************************************************
Description:
   Defines formatted error and warning message services, which alleviate
other parts of the implementation from concerns regarding formatting of text
messages, appropriate termination methods (e.g. process exit() or exception
throwing) in the event of a fatal error, graphical user interface
considerations and so forth.  These reduce or eliminate the effort required
to port the system to different application environments.
******************************************************************************/

#ifndef KDU_MESSAGING_H
#define KDU_MESSAGING_H

#include <stdlib.h>
#include <stdio.h> // For access to `sprintf'.
#include <assert.h>
#include "kdu_elementary.h"

// Defined here:
class kdu_message;
class kdu_thread_safe_message;
class kdu_message_formatter;
class kdu_error;
class kdu_warning;


/* ========================================================================= */
/*                       Messaging Setup Functions                           */
/* ========================================================================= */

extern KDU_EXPORT void
kdu_customize_warnings(kdu_message *handler);
/* [SYNOPSIS]
	 This function can and usually should be used to customize the behaviour
	 of warning messages produced using the `kdu_warning' object.  By
	 default, warning messages will simply be discarded.
   [ARG: handler]
	 All text delivered to a `kdu_warning' object will be passed to
	 `handler->put_text' (unless `handler' is NULL) and all calls to
	 `kdu_warning::flush' will be generate calls to `handler->flush',
	 with the `end_of_message' argument set to false (regardless of the
	 value of this argument in the call to `kdu_warning::flush').  When
	 any `kdu_warning' object is destroyed, `handler->flush' will be called
	 with an `end_of_message' argument equal to true.
*/
extern KDU_EXPORT void
kdu_customize_errors(kdu_message *handler);
/* [SYNOPSIS]
	 This function can and usually should be used to customize the behaviour
	 of error messages produced using the `kdu_error' object.  By
	 default, error message text will be discarded, and the process will
	 be terminated through `exit' whenever a `kdu_error' object is destroyed.
   [ARG: handler]
	 All text delivered to a `kdu_error' object will be passed to
	 `handler->put_text' (unless `handler' is NULL) and all calls to
	 `kdu_error::flush' will be generate calls to `handler->flush',
	 with the `end_of_message' argument set to false (regardless of the
	 value of this argument in the call to `kdu_error::flush').  If
	 a `kdu_error' object is destroyed, `handler->flush' will be called
	 with an `end_of_message' argument equal to true and the process will
	 subsequently be terminated through `exit'.  The termination may be
	 avoided, however, by throwing an exception from within the message
	 terminating `handler->flush' call.
	 [//]
	 For consistency, and especially to ensure correct behaviour of
	 Kakadu's multi-threaded processing machinery, you should only throw
	 exceptions of type `kdu_exception'.
*/
extern KDU_EXPORT void
kdu_customize_text(const char *context, kdu_uint32 id,
	const char *lead_in, const char *text);
/* [SYNOPSIS]
	 This function is used to register text (usually for internalization
	 purposes) to be rendered by `kdu_error' or `kdu_warning' when
	 constructed with the `context'/`id' pair supplied here.
	 [//]
	 Note that there is a second (overloaded) form of this function which
	 you can use to register unicode text.  If you invoke either of
	 these functions multiple times with the same `context' and `id'
	 values, the effects of all but the last invocation will be lost.
   [ARG: context]
	 String which is matched against a corresponding string which may
	 be supplied in the constructor for `kdu_error' or `kdu_warning'.
	 Note carefully that the string must be a constant resource.  Only
	 its address will be registered; the string itself will not be copied.
   [ARG: id]
	 Integer identifier which is matched against the value supplied in
	 the constructor for `kdu_error' or `kdu_warning'.
   [ARG: lead_in]
	 Holds the lead-in string to be printed first.  All other text is
	 printed only in response to interface functions invoked on the
	 `kdu_error' or `kdu_warning' object, as appropriate.
   [ARG: text]
	 Holds one or more text strings which are to replace instances of
	 a special 3-character PATTERN, when it is supplied to
	 `kdu_error::put_text' or `kdu_warning::put_text', as appropriate.
	 The special PATTERN consists of a "#" character, surrounded by "<"
	 and ">" delimiters.  Most applications of `kdu_error' or `kdu_warning'
	 involve only a single string, which means that a single instance of
	 the 3-character PATTERN will be translated.   However, multiple
	 translations may be required.  In general, if there are N instances
	 of the pattern, `text' is assumed to point to N null-terminated
	 strings concatenated together.  If any of these strings is found to
	 be empty, no further parsing into the `text' array will occur, meaning
	 that no further instances of the 3-character PATTERN will be replaced.
	 This means that you can avoid the risk of accidentally reading past
	 the end of a `text' array, by terminating the last string in the
	 array by two null ('\0') characters.  However, there is no
	 requirement for you to do this.
	 [//]
	 In practice, the special 3-character PATTERNs and the `text' strings
	 themselves are generated with the aid of macros (these have the
	 names `KDU_ERROR', `KDU_WARNING', `KDU_ERROR_DEV' and `KDU_WARNING_DEV'
	 in the licensed Kakadu source files), together with a processing
	 tool named "kdu_text_extractor".
	 [//]
	 Although many programs might assume that `text' is an ASCII text
	 string, it might also be a UTF-8 encoding of unicode characters.
	 Where possible, application message handlers should be capable of
	 handling UTF-8 rather than just ASCII.
*/
extern KDU_EXPORT void
kdu_customize_text(const char *context, kdu_uint32 id,
	const kdu_uint16 *lead_in, const kdu_uint16 *text);
/* [SYNOPSIS]
	 Same as the first form of the overloaded `kdu_customize_text'
	 function, except that this version registers unicode `lead_in'
	 and `text' strings.  Strings must be terminated with the 16-bit
	 unicode null character (0x0000).  Where multiple strings are
	 embedded in the `text' array, they must each be separated by the
	 unicode null character.  Reading into the `text' array will be
	 safely terminated once two consecutive unicode null characters have
	 been encountered.
*/


/* ========================================================================= */
/*                           Messaging Classes                               */
/* ========================================================================= */

/*****************************************************************************/
/*                               kdu_message                                 */
/*****************************************************************************/

class kdu_message {
	/* [BIND: reference]
	   [SYNOPSIS]
		 Objects of this class form a generic platform on which text messaging
		 services are built in Kakadu.  These services allow the Kakadu core
		 system to avoid any dependence on C++ I/O streams, which
		 are not always available on some platforms (e.g., WinCE).
		 For meaningful messaging, this object must be derived.  The
		 architecture is carefully devised to allow advanced messaging
		 extensions to be implemented in other language bindings (e.g., Java),
		 and to then be available for rendering all Kakadu core system
		 messages, along with other application-specific messages.
		 [//]
		 If you need to ensure that a single message service will behave
		 correctly when used from multiple threads, consider the
		 `kdu_thread_safe_message' object as a base for your derived
		 message classes.
		 [//]
		 For applications involving multiple threads and interactive
		 display of messages (e.g., with pop up windows), you should
		 almost certainly base your message handling services on
		 `kdu_message_queue', which derives from `kdu_thread_safe_message'
		 and allows messages to be generated in multiple threads but
		 displayed from a single master thread (many GUI environments
		 ultimately rely upon a window message handler which runs in a
		 single thread).
	*/
public: // Member functions
	kdu_message() { mode_hex = false; }
	virtual ~kdu_message() { return; };
	virtual void put_text(const char *string) { return; }
	/* [BIND: callback]
	   [SYNOPSIS]
		 This function should be overridden in any meaningful messaging
		 class, providing an implementation of the text output function.
		 All ASCII or UTF-8 text is delivered through this function.
		 [//]
		 For foreign language bindings, e.g., Java, this function is regarded
		 as a callback.  When the native function is called, it will
		 automatically invoke the corresponding Java (or other foreign
		 language) function, supplying the text in the most natural form.
	  */
	virtual void put_text(const kdu_uint16 *string) { return; }
	/* [SYNOPSIS]
		 This form of the `put_text' function is called if the text to
		 be printed is a 16-bit unicode string.  To support fixed-width
		 unicode character strings, you have only to override this
		 function with a meaningful implementation.
		 [//]
		 In this case, you need to be prepared to accept a mixture of
		 calls to the ASCII/UTF-8 and Unicode versions of `put_text'.
	*/
	virtual void flush(bool end_of_message = false) { return; }
	/* [BIND: callback]
	   [SYNOPSIS]
		 This function should be overridden by any derived messaging
		 class, whose implementation requires `flush' events to be captured.
	   [ARG: end_of_message]
		 Has special meaning to the messaging objects passed in to
		 `kdu_customize_errors' and `kdu_customize_warnings', as explained
		 in connection with those functions.  As a general rule,
		 applications should provide a value of false for this argument
		 if they need to flush text which may be temporarily buffered in
		 a `kdu_message'-derived object.
	*/
	virtual void start_message() { return; }
	/* [BIND: callback]
	   [SYNOPSIS]
		 This function is invoked on the `kdu_message'-derived objects
		 passed in to `kdu_customize_errors' and `kdu_customize_warnings',
		 whenever a `kdu_error' or `kdu_warning' object (as appropriate)
		 is constructed.  In many cases, there may be no need to override
		 this function when deriving new classes from `kdu_message'.
		 An important exception, however, arises when a single messaging
		 object is to be shared by multiple threads of execution.  To
		 avoid message mangling or nasty race conditions, multi-threaded
		 applications should lock a mutex resource when `start_message'
		 is called, releasing it when `flush' is called with `end_of_message'
		 true.
	*/
	bool set_hex_mode(bool new_mode)
	{
		bool old_mode = mode_hex; mode_hex = new_mode; return old_mode;
	}
	/* [SYNOPSIS]
		 Changes the behaviour of integer output operators.  A false
		 argument means that integer values will be displayed as decimal
		 quantities, while a true argument means that  they will be displayed
		 as hexadecimal quantities.  The function returns the previous
		 value of the hexadecimal mode, so that it can be restored by the
		 caller.
	*/
	kdu_message &operator<<(const char *string)
	{
		put_text(string); return *this;
	}
	kdu_message &operator<<(char ch)
	{
		char text[2]; text[0] = ch; text[1] = '\0'; put_text(text); return *this;
	}
	kdu_message &operator<<(int val)
	{
		char text[80];
		sprintf(text, (mode_hex) ? "%x" : "%d", val);
		put_text(text); return *this;
	}
	kdu_message &operator<<(unsigned int val)
	{
		char text[80];
		sprintf(text, (mode_hex) ? "%x" : "%u", val);
		put_text(text); return *this;
	}
	kdu_message &operator<<(short int val)
	{
		return (*this) << (int)val;
	}
	kdu_message &operator<<(unsigned short int val)
	{
		return (*this) << (unsigned int)val;
	}
#ifdef KDU_LONG64
	kdu_message &operator<<(kdu_long val)
	{ // Conveniently prints in decimal with thousands separated by commas
		if (val < 0)
		{
			(*this) << '-'; val = -val;
		}
		kdu_long base = 1;
		while ((1000 * base) <= val) base *= 1000;
		int digits = (int)(val / base);   (*this) << digits;
		while (base > 1)
		{
			val -= base * digits;  base /= 1000;  digits = (int)(val / base);
			char text[4]; sprintf(text, "%03d", digits); (*this) << ',' << text;
		}
		return (*this);
	}
#endif
	kdu_message &operator<<(float val)
	{
		char text[80]; sprintf(text, "%f", val); put_text(text); return *this;
	}
	kdu_message &operator<<(double val)
	{
		char text[80]; sprintf(text, "%f", val); put_text(text); return *this;
	}
private: // Data
	bool mode_hex;
};

/*****************************************************************************/
/*                         kdu_thread_safe_message                           */
/*****************************************************************************/

class kdu_thread_safe_message : public kdu_message {
	/* [BIND: reference]
	   [SYNOPSIS]
		 Provides thread-safe handling of messages, for applications in
		 which multiple threads may need to write messages to the same
		 object.  This is done by locking a mutex inside the
		 `start_message' member function and unlocking it again in the
		 `flush' function (when called with `end_of_message'=true).  Between
		 these two calls, you may be sure that only one thread is
		 writing text to the object.
		 [//]
		 If your platform does not support multi-threading, this object
		 will have identical behaviour to `kdu_message'.
	*/
public: // Member functions
	kdu_thread_safe_message()
	{
		is_locked = false; mutex.create();
	}
	virtual ~kdu_thread_safe_message()
	{
		mutex.destroy();
	}
	virtual void flush(bool end_of_message = false)
	{
		if (end_of_message && is_locked)
		{
			is_locked = false; mutex.unlock();
		}
	}
	/* [BIND: callback]
	   [SYNOPSIS]
		 Unlocks the mutex locked by `start_message', so long as
		 `end_of_message' is true and the mutex was locked by a
		 previous call to `start_message'.
	*/
	virtual void start_message()
	{
		mutex.lock(); is_locked = true;
	}
	/* [BIND: callback]
	   [SYNOPSIS]
		 Locks a mutex so that no other thread can complete a call to
		 `start_message' until the present thread calls `flush' with
		 an `end_of_message' argument equal to true.
	*/
protected: // Data
	bool is_locked;
	kdu_mutex mutex;
};

/*****************************************************************************/
/*                             kdu_message_queue                             */
/*****************************************************************************/

class kdu_message_queue : public kdu_thread_safe_message {
	/* [BIND: reference]
	   [SYNOPSIS]
		 This object provides a very convenient base on which to construct
		 highly robust message handling systems, primarily aimed at the handling
		 of `kdu_error' and/or `kdu_warning' messages which might arrive on
		 different threads, but can only robustly be displayed within a single
		 specific thread.  This sort of situation comes up frequently when
		 implementing GUI-based applications on a variety of operating systems.
		 [//]
		 You can also use this object as a convenient way of buffering up
		 messages, even if you don't need queueing or multi-threading
		 capabilities specifically.
		 [//]
		 To derive from this object, you normally need only to override the
		 `pop_message' function.  If you are happy to manually pop messages
		 from the queue, you may not even need to derive a new object; this is
		 the case if all you want to do is use the object to buffer messages
		 produced by any source which delivers data to a `kdu_message' interface.
		 [//]
		 When using this object to buffer messages, you need to remember to
		 call `kdu_message::start_message' to start each message and
		 `kdu_message::flush' (with `end_of_message'=true) to finish each
		 message.  Otherwise, no message text will appear in the queue.
		 This is done automatically for you if you are using the
		 object to implement an error or warning service (to handle messages
		 generated through `kdu_error' or `kdu_warning').
	*/
public: // Member funcions
	kdu_message_queue()
	{
		throw_exceptions = false; exception_val = KDU_NULL_EXCEPTION;
		auto_pop = false; max_elts = 10; num_elts = 0;
		queue_head = queue_tail = NULL; active_msg = NULL;  popped_msg = NULL;
	}
	/* [SYNOPSIS]
		 Constructs a queue for storing messages.  By default, the queue
		 can hold a maximum of 10 messages, and the `flush' function
		 will neither throw exceptions nor automatically issue calls to
		 `pop_message' when a new message is complete.  To change this
		 behaviour, you should call the `configure' function at any point
		 after construction.
	*/
	virtual ~kdu_message_queue()
	{
		while ((queue_tail = queue_head) != NULL)
		{
			queue_head = queue_tail->next; delete queue_tail;
		}
		if (popped_msg != NULL) delete popped_msg;
	}
	void configure(int max_queued_messages, bool auto_pop,
		bool throw_exceptions,
		kdu_exception exception_val = KDU_ERROR_EXCEPTION)
	{
		this->max_elts = (max_queued_messages < 1) ? 1 : max_queued_messages;
		this->auto_pop = auto_pop; this->throw_exceptions = throw_exceptions;
		this->exception_val = exception_val;
	}
	/* [SYNOPSIS]
		 Use this function to configure the queue for anything other than
		 a simple buffer for `kdu_message'-delivered text.
	   [ARG: max_queued_messages]
		 Provides a limit on the number of messages which can be stored on
		 the internal queue, including the one which is currently being
		 generated.  This value must not be less than 1.  If the limit is
		 exceeded (e.g., because the `pop_message' function is called too
		 infrequently, at least from a context in which it can succeed),
		 the least recent messages are dropped.  If you never call
		 `configure', the default queue limit is 10.  For a message service
		 that you intend to pass to `kdu_customize_errors' or
		 `kdu_customize_warnings', it is recommended that the message queue
		 limit be at least as large as the number of threads in your system
		 which could possibly generate errors/warnings.  Of course, you may
		 pick a much larger upper bound, but don't pick something completely
		 ridiculous, since that could result in a lot of memory being
		 allocated if the system gets trapped in some error generating
		 loop.
	   [ARG: auto_pop]
		 If this argument is true, the `pop_message' function will be called
		 from within `flush' (with `end_of_message'=true) to pop the
		 recently completed message and any other messages from the queue,
		 in order.  This is exactly what you want when implementing a
		 message handling service for `kdu_error' and `kdu_warning'
		 delivered messages (the kind of service you would pass to
		 `kdu_customize_errors' or `kdu_customize_warnings'), but you do
		 then need to override `pop_message' in a custom derived class, to
		 decide whether and how to display popped message text.
	   [ARG: throw_exceptions]
		 If this argument is true, an integer exception will be thrown with
		 value `exception_val' at the end of a call to `flush' which has
		 `end_of_message'=true.  This is exactly what you want when
		 implementing an error message handler (something you would pass to
		 `kdu_customize_errors').
		 [//]
		 Unless you have good reason to do otherwise, you are strongly
		 recommended to use the default `exception_val' of
		 `KDU_ERROR_EXCEPTION'.
	*/
	KDU_EXPORT virtual void put_text(const char *string);
	/* [SYNOPSIS]
		 You should not neeed to override this function.  It interprets
		 all 8-bit characters strings as null-terminated UTF-8 strings
		 and allows intermixing with unicode text supplied via the
		 second (unicode) form of the function.
	*/
	KDU_EXPORT virtual void put_text(const kdu_uint16 *string);
	/* [SYNOPSIS]
		 You should never need to override this function.  It converts
		 any unicode text into multi-byte UTF-8 characters, allowing them
		 to be mixed directly with ASCII or UTF-8 strings which appear
		 via the first form of the `put_text' function.
	*/
	virtual void start_message()
	{
		kdu_thread_safe_message::start_message();
		if (active_msg != NULL) return; // Should not happen
		kdu_msg_queue_elt *elt = new kdu_msg_queue_elt;
		if (queue_tail == NULL)
		{
			queue_head = queue_tail = elt; num_elts = 1;
		}
		else if (num_elts < max_elts)
		{
			queue_tail = queue_tail->next = elt; num_elts++;
		}
		else
		{
			queue_tail = queue_tail->next = queue_head;
			queue_head = queue_head->next; queue_tail->next = NULL;
		}
		active_msg = queue_tail;  active_msg->len = 0;
	}
	/* [SYNOPSIS]
		 It is expected that all message text is provided via `put_text'
		 calls which are bracketed by calls to `start_message' and
		 `flush' (with the `end_of_message' argument true).
	*/
	virtual void flush(bool end_of_message = false)
	{
		if ((active_msg == NULL) || !end_of_message) return;
		active_msg = NULL;
		kdu_thread_safe_message::flush(true);
		if (auto_pop)
			while (pop_message() != NULL);
		if (throw_exceptions)
			throw exception_val;
	}
	/* [SYNOPSIS]
		 The internal implementation of this function should do enough for
		 you without any need to override it (although of course you can).
		 [//]
		 If `end_of_message' is false, the function does nothing at all.
		 [//]
		 If `end_of_message' is true, the function effectively adds all text
		 received since the last call to `start_message' onto the queue
		 as a new complete message.  The function then releases the
		 internal mutex by calling the overridden function
		 `kdu_thread_safe_message::flush'.  If the `configure' function
		 specified auto-popping, the `pop_message' is then called repeatedly
		 until it returns NULL -- this is normally the behaviour you would
		 want to ensure that a message is displayed to the user as soon as
		 possible.  Finally, if the `configure' function specified that
		 exceptions should be thrown, the relevant exception is thrown here.
	*/
	virtual const char *pop_message()
	{
		mutex.lock();
		if (popped_msg != NULL) { delete popped_msg; popped_msg = NULL; }
		if ((queue_head != NULL) && (queue_head != active_msg))
		{
			popped_msg = queue_head;
			if ((queue_head = queue_head->next) == NULL) queue_tail = NULL;
			popped_msg->next = NULL; num_elts--;
		}
		mutex.unlock();
		return (popped_msg == NULL) ? NULL : (popped_msg->buf);
	}
	/* [SYNOPSIS]
		 Overriding this function is most likely the only thing you need
		 to do to make a useful message handling object for
		 multi-threaded GUI-based environments.
		 [//]
		 The base function retrieves the first message from the queue
		 as a UTF-8 character string, returning NULL only if there is
		 no complete message on the queue.  If a non-NULL string is
		 returned, the queue is advanced, but you can be sure that the
		 internal buffer will not be overwritten, at least until this
		 function is called again.  You should generally call this
		 function from only one thread to avoid this risk, although the
		 internal implementation will at least work correctly when
		 invoked from any thread.  It is expected that the function will
		 be called from a context in which the internal mutex is not
		 locked.  In particular, you should not call the function from
		 any overrides of the `put_text' function, nor from any override
		 of `flush' at least until that override invokes the base
		 `kdu_thread_safe_message::flush' function.  However, you should
		 not normally need to concern yourself with these details because
		 none of those functions normally need to be overridden at all.
		 [//]
		 In a typical implementation, you provide an override of this
		 function alone, which first checks to see if it is being called
		 from the thread you in which you want to do all your message
		 displaying -- typically the one running a window message loop.
		 If not, the overriding function should return NULL after taking
		 steps to encourage the message displaying thread to call this
		 function itself at some point in the future.  If the function is
		 being called form the message displaying thread, it should invoke
		 the base function, whose behaviour is described in the previous
		 paragraph, rendering any returned UTF-8 string in an appropriate
		 way, and returning once the string is no longer required (e.g.,
		 when a user closes a window which is displaying the message).  It
		 is up to you whether or not you block the function until a user
		 closes a message window (not that your implementation has to use
		 message windows), but this is probably the most appropriate
		 behaviour for an error message handler at least.  In any case,
		 your overriding function should return NULL once it has no more
		 messages to display.
		 [//]
		 In a typical GUI-based application, the message displaying thread
		 is the thread which runs the message loop for the window system
		 and you define a custom message which causes this message
		 thread to invoke the present function repeatedly until it
		 returns NULL.  This custom message is delivered if this function
		 is invoked from any other thread.  In this way, you can be sure
		 that messages are displayed as soon as reasonably practicable
		 regardless of what thread they arrive on.
	*/
private: // Internal structures
	struct kdu_msg_queue_elt {
		kdu_msg_queue_elt()
		{
			len = 0; max_len = 10; buf = new char[11]; buf[0] = '\0'; next = NULL;
		}
		~kdu_msg_queue_elt()
		{
			if (buf != NULL) delete[] buf;
		}
		char *buf; // Contains null terminated string; never NULL.
		int len, max_len; // these do not include the null-terminator
		kdu_msg_queue_elt *next;
	};
private: // Data
	bool auto_pop;
	bool throw_exceptions;
	kdu_exception exception_val; // Value thrown by `flush'
	int max_elts; // Maximum allowed length of the list headed by `queue_head'
	int num_elts; // Number of elements in the list headed by `queue_head'
	kdu_msg_queue_elt *queue_head;
	kdu_msg_queue_elt *queue_tail;
	kdu_msg_queue_elt *active_msg; // Points to the `queue_tail' or else NULL
	kdu_msg_queue_elt *popped_msg; // Message most recently popped
};

/*****************************************************************************/
/*                            kdu_message_formatter                          */
/*****************************************************************************/

class kdu_message_formatter : public kdu_message {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Derived `kdu_message' object which formats the text it receives
	   into lines of a given maximum length (as supplied to the constructor)
	   and passes the lines to a separate `kdu_message'-derived object.  Every
	   attempt is made to avoid breaking words (space-delimited tokens) across
	   lines.  Tab characters are converted to a single space, unless they
	   appear immediately after the last new-line (or immediately after the
	   object is created).  In the latter case, each tab increments indentation
	   for the paragraph (currently by 4 spaces).  Indentation state is
	   cancelled at the next new-line character.
	   [//]
	   The present object provides a convenient means of automatically
	   upgrading a custom message handling service, constructed by deriving
	   from `kdu_message', with elementary formatting capabilities.  This
	   is achieved by supplying a pointer to your custom message handling
	   object as the `output' argument in the current object's constructor,
	   and then directing all text to the present object (or passing a
	   reference to the present object into one of the error/warning
	   configuration functions `kdu_customize_errors' or
	   `kdu_customize_warnings').
	   [//]
	   Unfortunately, the current implementation of this object does not
	   support unicode or UTF-8 (only ASCII).  This could be done in the
	   future without changing the form of any interface functions.  However,
	   it is unclear whether non-ASCII character sets should be formatted
	   using the simple rules of this object, since they are based on an
	   assumption of fixed pitch fonts.  In general, if your application
	   expects richer text content, you are recommended to write your
	   own message formatter, or do any formatting within the main
	   custom message handler, perhaps at the point when it is about to
	   generate output -- at that point, the font properties, window
	   dimensions and so forth, might be better known.  For simple
	   terminal-oriented messaging, however, the present function most
	   likely does everything you need, making it easy to get new
	   applications up and running.
	*/
public: // Member functions
	kdu_message_formatter(kdu_message *output, int max_line = 79)
	{
		/* [SYNOPSIS]
			 Identifies the `kdu_message'-derived object which is to receive
			 the formatted text, and the maximum line length to be used during
			 formatting.
		   [ARG: output]
			 If NULL, all text will be discarded.  If you supply a
			 `kdu_thread_safe_message' object (or derived object) here,
			 the entire combination will be thread safe, in the sense that
			 message text handling is protected against interference by
			 other threads, so long as the `start_message' function is used
			 to start writing a message and the `flush' function with
			 `end_of_message'=true is used to finish writing a message.
			 [//]
			 A natural way to build good error/warning handlers which are
			 robust is to implement your handler on top of
			 `kdu_thread_safe_message' or `kdu_message_queue' and
			 construct (usually) separate instances for errors and warnings.
			 Then construct a separate instance of the `kdu_message_formatter'
			 object for errors and warnings, passing them references to the
			 corresponding error/warning handlers, implemented as above.
			 Finally, pass references to the `kdu_message_formatter' objects
			 to `kdu_customize_errors' and `kdu_customize_warnings'.
		   [ARG: max_line]
			 Max characters in a formatted line of text.  The bound does not
			 include null terminators or terminal new-line characters, but it
			 does include any prevailing indentation, established by the use
			 of tab characters or through calls to `set_master_indent'.  Note
			 that the actual maximum line length may be smaller than the
			 bound supplied here, depending on the way in which internal
			 resources are allocated.
		*/
		if (max_line > 200) max_line = 200;
		dest = output; line_chars = max_line; num_chars = 0;
		max_indent = 40; indent = 0; master_indent = 0;
		no_output_since_newline = true;
	}
	~kdu_message_formatter() { if (dest != NULL) dest->flush(); }
	/* [SYNOPSIS]
		 Flushes any outstanding text to the output object.
	*/
	KDU_EXPORT void set_master_indent(int val);
	/* [SYNOPSIS]
		 This function alters the amount by which every line will be
		 indented from this point forward, in addition to any
		 paragraph-specific indents inserted by sending tab (`\t')
		 characters to the `put_text' function.
		 [//]
		 You should try to avoid calling this function except at the
		 beginning of a paragraph (i.e., immediately after a new-line
		 character, a call to `flush', or object construction).  If
		 you call the function in the middle of a paragraph, the `flush'
		 function will be invoked first.
	   [ARG: val]
		 Minimal indent (number of spaces) at the beginning of each
		 formatted line of text.  Additional indentation may be
		 introduced by tab characters in text supplied to the base
		 `streambuf' object. */
	KDU_EXPORT void put_text(const char *string);
	/* [SYNOPSIS]
		 Implements the `kdu_message::put_text' function, collecting
		 text as it appears and formatting it into lines with appropriate
		 indentation and line breaks.
		 [//]
		 Note that the current implementation supports only ASCII text
		 formatting (at least it assumes that each character corresponds
		 to a single byte).  If you want to support Unicode or
		 multi-byte UTF-8 strings, you are best off implementing your own
		 `kdu_message'-derived object from scratch, as explained in the
		 introductory comments to this `kdu_message_formatter' class.
	*/
	KDU_EXPORT void put_text(const kdu_uint16 *string)
	{
		kdu_message::put_text(string);
	}
	/* [SYNOPSIS]
		 We do not currently provide a text formatting service for
		 unicode strings -- they will get lost.  this function is
		 included only to avoid warning messages about the underlying
		 virtual function being hidden by the previous `put_text'
		 function.  In the future, we might give it a useful
		 implementation.
	*/
	KDU_EXPORT void flush(bool end_of_message = false);
	/* [SYNOPSIS]
		 Flushes the current line, as though the line length had been
		 exceeded, and invokes the output object's `kdu_message::flush'
		 function.
	*/
	void start_message()
	{
		if (dest == NULL) return;
		dest->start_message();
		flush();
	}
	/* [SYNOPSIS]
		 Passes the `start_message' call along to the output object,
		 which may wish to lock a mutex to guard multi-threaded access
		 to the text formatting facilities provided here.  The `flush'
		 function is then called to flush any outstanding text from
		 previous formatting operations.
	*/
private: // Data
	char line_buf[201];
	int line_chars; // Maximum number of characters per line. Must be <= 200.
	int num_chars; // Number of characters written to current line.
	int max_indent;
	int indent; // Indent to be applied until next new-line character.
	int master_indent; // Indent to be applied to all lines henceforth.
	bool no_output_since_newline;
	kdu_message *dest;
};

/*****************************************************************************/
/*                                kdu_error                                  */
/*****************************************************************************/

class kdu_error : public kdu_message {
	/* [SYNOPSIS]
		 Objects of this class may be created, written to and then
		 destroyed as a compact and powerful mechanism for generating error
		 messages and jumping out of the immediate execution context.  All
		 text is delivered to the `kdu_message'-derived object  supplied in
		 the most recent call to the `kdu_customize_errors' function.  The
		 `kdu_error' object's destructor plays a key role in completing
		 errors.  It first invokes the output object's `kdu_message::flush'
		 function with an `end_of_message' argument equal to true, and then
		 terminates the process through the system `exit' function.
		 [//]
		 To avoid process termination, the caller may supply a special
		 `kdu_message'-derived object to `kdu_customize_errors' which
		 overrides the `flush' function so as to throw an exception in the
		 event that its `flush' member is called with `end_of_message' equal
		 to true.  This condition is guaranteed not to occur until the
		 `kdu_error' object is destroyed.  For consistency, and especially
		 to ensure correct behaviour of Kakadu's multi-threaded processing
		 machinery, you should only throw exceptions of type `kdu_exception'.
		 [//]
		 The following code-fragment illustrates typical usage:
		 [>>]
		 { kdu_error e; if (error) { kdu_error e; e << "Oops! Don't do that."; }}
		 [//]
		 In view of the preceding discussion, it should be clear that
		 `kdu_error' objects should never be created on the heap.
		 [//]
		 As of Kakadu Version 4.5, the `kdu_error' object provides additional
		 constructors which may be used to customize the way messages are
		 displayed, providing for internationalizable text and/or allowing
		 compiled code to be stripped of numerous messages which are
		 meaningful only to the developer.
	*/
public:
	KDU_EXPORT kdu_error();
	/* [SYNOPSIS]
		 This original form of the constructor for the `kdu_error' object
		 should be used if you have no special requirements such as
		 custom lead-in's or internationalization.
	*/
	KDU_EXPORT kdu_error(const char *lead_in);
	/* [SYNOPSIS]
		 This form of the constructor modifies the lead-in message
		 which is automatically generated whenever a `kdu_error' object
		 is constructed.
		 [//]
		 The default constructor uses the lead-in string "Kakadu Error:\n".
		 If you use this form of the constructor, the supplied `lead_in'
		 will be used.  If you wish the lead-in text to appear on its
		 own separate line, you must explicitly supply the end-of-line
		 character.
	*/
	KDU_EXPORT kdu_error(const char *context, kdu_uint32 id);
	/* [SYNOPSIS]
		 This form of the constructor allows all for the creation of
		 internationalizable or otherwise customized messages.  The
		 `context' string and `id' are used together to form a unique
		 index against which customizable aspects of the message text
		 are registered using the `kdu_customize_text' function.  That
		 function can be used to register both the lead-in text and
		 a string to replace each instance of the special 3-character
		 PATTERN consisting of a "#" character surrounded by "<" and
		 ">" delimiters, which is supplied to `put_text'.
		 [//]
		 If nothing is registered against this `context'/`id' pair, a
		 special lead-in string is generated which commences with the
		 text: "Untranslated error -- Consult vendor for more information".
		 This lead-in is followed by details of the `context' string and
		 `id' value, followed by the untranslated text of the error message.
		 This information is sufficient for the full error message to
		 be reconstructed by the vendor who compiled the application.
		 [//]
		 All calls to `kdu_error' from within the Kakadu core system
		 invoke the constructor via a macro replacement strategy which
		 allows either simple messaging or full registered messaging
		 for internationalization.  See any of the source files to
		 understand how this is achieved.
	*/
	KDU_EXPORT ~kdu_error();
	/* [SYNOPSIS]
		 The destructor terminates the process through `exit', unless a
		 custom error handling `kdu_message'-derived object has been
		 installed using `kdu_customize_errors'.  In the latter case,
		 the error can be caught immediately before the call to `exit'
		 by overriding `kdu_message::flush' and checking for the
		 `end_of_message'=true condition.  The override may throw an
		 exception in interactive applications where processing must
		 be able to continue.  Because this behaviour is to be expected,
		 you should be careful only to destroy `kdu_error' objects from a
		 context in which both process exit and exception throwing are
		 reasonable behaviours. */
	KDU_EXPORT void put_text(const char *string);
	/* [SYNOPSIS]
		 Passes all text directly through to the `kdu_message'-derived
		 error message handler supplied to `kdu_customize_errors', if
		 not NULL.
	*/
	KDU_EXPORT void put_text(const kdu_uint16 *string)
	{
		if (handler != NULL) handler->put_text(string);
	}
	/* [SYNOPSIS]
		 Passes all unicode text directly through to the
		 `kdu_message'-derived error message handler supplied to
		 `kdu_customize_errors', if not NULL.
	*/
	void flush(bool end_of_message)
	{
		if (handler != NULL) handler->flush(false);
	}
	/* [SYNOPSIS]
		 Invokes the `kdu_message::flush' function associated with the
		 error message handler supplied to `kdu_customize_errors', if not
		 NULL, being careful to always set `end_of_message' to false in this
		 forwarded call.  This ensures that an `end_of_message' value of
		 true will uniquely identify the fact that the `kdu_error' object
		 is being destroyed.
	*/
private:
	kdu_message *handler;
	const char *ascii_text; // Non-NULL if ASCII replacement text exists
	const kdu_uint16 *unicode_text; // Non-NULL if Unicode replacements exist
};

/*****************************************************************************/
/*                               kdu_warning                                 */
/*****************************************************************************/

class kdu_warning : public kdu_message {
	/* [SYNOPSIS]
		 Objects of this class are used in a similar manner to `kdu_error'
		 objects, except that the destructor does not terminate the currently
		 executing process.  Note, however, that even warning handlers may
		 conceivably throw exceptions or terminate the process themselves.
		 The warning handler is configured using `kdu_customize_warnings'.
		 For this reason, `kdu_warning' objects should never be created on
		 the heap.
	*/
public:
	KDU_EXPORT kdu_warning();
	/* [SYNOPSIS]
		 This original form of the constructor for the `kdu_warnikng' object
		 should be used if you have no special requirements such as
		 custom lead-in's or internationalization.
	*/
	KDU_EXPORT kdu_warning(const char *lead_in);
	/* [SYNOPSIS]
		 This form of the constructor modifies the lead-in message
		 which is automatically generated whenever a `kdu_warning' object
		 is constructed.
		 [//]
		 The default constructor uses the lead-in string "Kakadu Warning:\n".
		 If you use this form of the constructor, the supplied `lead_in'
		 will be used.  If you wish the lead-in text to appear on its
		 own separate line, you must explicitly supply the end-of-line
		 character.
	*/
	KDU_EXPORT kdu_warning(const char *context, kdu_uint32 id);
	/* [SYNOPSIS]
		 This form of the constructor allows all for the creation of
		 internationalizable or otherwise customized messages.  The
		 `context' string and `id' are used together to form a unique
		 index against which customizable aspects of the message text
		 are registered using the `kdu_customize_text' function.  That
		 function can be used to register both the lead-in text and
		 a string to replace each instance of the special 3-character
		 PATTERN, consisting of a "#" character surrounded in "<" and ">"
		 delimiters, which is supplied to `put_text'.
		 [//]
		 If nothing is registered against this `context'/`id' pair,
		 no warning will be printed at all.  This may be used to hide
		 warnings which are meaningful only to developers from the
		 end users of an application.
		 [//]
		 All calls to `kdu_warning' from within the Kakadu core system
		 invoke the constructor via a macro replacement strategy which
		 allows either simple messaging or full registered messaging
		 for internationalization.  See any of the source files to
		 understand how this is achieved.
	*/
	KDU_EXPORT ~kdu_warning();
	/* [SYNOPSIS]
		 The destructor will invoke the `kdu_message::flush' function of
		 any warning handler installed using `kdu_customize_warnings',
		 supplying an `end_of_message' argument equal to true.  This
		 condition will not occur in any context other than the destruction
		 of the `kdu_warning' object.
	*/
	KDU_EXPORT void put_text(const char *string);
	/* [SYNOPSIS]
		 Passes all text directly through to the `kdu_message'-derived
		 warning message handler supplied to `kdu_customize_warnings', if
		 not NULL.
	*/
	KDU_EXPORT void put_text(const kdu_uint16 *string)
	{
		if (handler != NULL) handler->put_text(string);
	}
	/* [SYNOPSIS]
		 Passes all unicode text directly through to the
		 `kdu_message'-derived warning message handler supplied to
		 `kdu_customize_warnings', if not NULL.
	*/
	void flush(bool end_of_message)
	{
		if (handler != NULL) handler->flush(false);
	}
	/* [SYNOPSIS]
		 Invokes the `kdu_message::flush' function associated with the
		 warning message handler supplied to `kdu_customize_warnings', if not
		 NULL, being careful to always set `end_of_message' to false in this
		 forwarded call.  This ensures that an `end_of_message' value of
		 true will uniquely identify the fact that the `kdu_warning' object
		 is being destroyed.
	*/
private:
	kdu_message *handler;
	const char *ascii_text; // Non-NULL if ASCII replacement text exists
	const kdu_uint16 *unicode_text; // Non-NULL if Unicode replacements exist
};

/* ========================================================================= */
/*                     Messaging Short-Cut Functions                         */
/* ========================================================================= */

inline void
kdu_print_error(const char *message)
{
	kdu_error e; e << message;
}
/* [SYNOPSIS]
	 This function provides a compact means of generating simple
	 error messages through `kdu_error'.  It is particularly useful
	 for foreign language bindings which require all objects to reside
	 on the heap, since `kdu_error' objects should never be created
	 on the heap.
*/

inline void
kdu_print_warning(const char *message)
{
	kdu_warning w; w << message;
}
/* [SYNOPSIS]
	 This function provides a compact means of generating a simple
	 warning message through `kdu_warning'.  It is particularly useful
	 for foreign language bindings which require all objects to reside
	 on the heap, since `kdu_warning' objects should never be created
	 on the heap.
*/

#endif // KDU_MESSAGING_H
