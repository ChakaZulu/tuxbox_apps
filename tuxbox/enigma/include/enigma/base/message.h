#ifndef __core_base_message_h
#define __core_base_message_h

#include "ebase.h"

/**
 * \brief A generic messagepump.
 *
 * You can send and receive messages with this class. Internally a fifo is used,
 * so you can use them together with a \c eMainloop.
 */
class eMessagePump
{
	int fd[2];
public:
	eMessagePump();
	~eMessagePump();
	int send(const void *data, int len);
	int recv(void *data, int len); // blockierend
	int getInputFD() const;
	int getOutputFD() const;
};

/**
 * \brief A messagepump with fixed-length packets.
 *
 * Based on \ref eMessagePump, with this class you can send and receive fixed size messages.
 * Automatically creates a eSocketNotifier and gives you a callback.
 */
template<class T>
class eFixedMessagePump: private eMessagePump, public Object
{
	eSocketNotifier *sn;
	void do_recv(int)
	{
		T msg;
		recv(&msg, sizeof(msg));
		/*emit*/ recv_msg(msg);
	}
public:
	Signal1<void,const T&> recv_msg;
	void send(const T &msg)
	{
		eMessagePump::send(&msg, sizeof(msg));
	}
	eFixedMessagePump(eMainloop *context)
	{
		sn=new eSocketNotifier(context, getOutputFD(), eSocketNotifier::Read);
		CONNECT(sn->activated, eFixedMessagePump<T>::do_recv);
		sn->start();
	}
	~eFixedMessagePump()
	{
		delete sn;
	}
};

#endif
