#ifndef __record_h
#define __record_h

#include <include/libsig_comp.h>
#include <core/base/ebase.h>
#include <core/base/message.h>
#include <core/base/thread.h>
#include <core/base/eerror.h>

#include <set>

/**
 * \brief The DVBRecorder
 *
 * A class which can record a TS consisting of given pids. The
 * recorder runs in a seperate thread and can be controlled 
 * asynchronously. Internally this is done with a \c eMessagePump.
 * \todo Howto disable this warning?
 */
class eDVBRecorder: private eThread, eMainloop, public Object
{
	struct eDVBRecorderMessage
	{
		enum eCode
		{
			mOpen, mAddPID, mRemovePID, mRemoveAllPIDs, mClose, mStart, mStop, mExit
		} code;
		union
		{
			const char *filename;
			int pid;
		};
		eDVBRecorderMessage() { }
		eDVBRecorderMessage(eCode code): code(code) { }
		eDVBRecorderMessage(eCode code, const char *filename): code(code), filename(filename) { }
		eDVBRecorderMessage(eCode code, int pid): code(code), pid(pid) { }
	};
	
	struct pid_t
	{
		int pid;
		int fd;
		bool operator < (const pid_t &p) const
		{
			return pid < p.pid;
		}
	};
	
	std::set<pid_t> pids;
	
	int dvrfd;
	int outfd;

	eSocketNotifier *sn;
	
	eLock lock;
	
	void dataAvailable(int what);

	eFixedMessagePump<eDVBRecorderMessage> messagepump;
	void thread();
	void gotMessage(const eDVBRecorderMessage &msg);

	void s_open(const char *filename);
	void s_addPID(int pid);
	void s_removePID(int pid);
	void s_removeAllPIDs();
	void s_close();
	void s_start();
	void s_stop();
	void s_exit();
public:
		/// the constructor
	eDVBRecorder();
		/// the destructor
	~eDVBRecorder();

	/**
	 * \brief Opens a file
	 *
	 * This asynchronous call will open a new filename. It will close open files.
	 * \param filename Pointer to a filename. Existing files will be deleted. (life is cruel)
	 */
	void open(const char *filename)
	{
		messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mOpen, strcpy(new char[strlen(filename+1)], filename)));
	}
	/**
	 * \brief Adds a PID.
	 *
	 * This asynchronous call will add a PID to record. Recording doesn't start until using \c start().
	 * \sa eDVBRecorder::start
	 */
	void addPID(int pid)
	{
		messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mAddPID, pid));
	}
	
	/**
	 * \brief Removes a PID.
	 *
	 * This asynchrounus call will remove a PID.
	 */
	void removePID(int pid)
	{
		messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mRemovePID, pid));
	}
	
	/**
	 * \brief Removes a PID.
	 *
	 * This asynchrounus call will remove all pids.
	 */
	void removeAllPIDs()
	{
		messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mRemoveAllPIDs));
	}
	
	/**
	 * \brief Start recording.
	 *
	 * This asynchronous call will start the recording. You can stop it with \c stop().
	 * \sa eDVBRecorder::stop
	 */
	void start()
	{
		messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mStart));
	}
	
	/**
	 * \brief Stop recording.
	 *
	 * This asynchrounous call will stop the recording. You can restart it (and append data) with \c start again.
	 */
	void stop()
	{
		messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mStop));
	}
	
	/**
	 * \brief Closes recording file.
	 *
	 * This will close the recorded file.
	 */
	void close()
	{
		messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mClose));
	}
};

#endif
